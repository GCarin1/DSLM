#!/usr/bin/env python3
"""Convert LittleKedi (Hugging Face: model.safetensors, config.json,
tokenizer.json) into DSLM's MLA checkpoint and tokenizer files.

    python tools/m8/convert_littlekedi.py <hf dir> <out dir>

Writes <out>/littlekedi.bin and <out>/littlekedi.tok. Standard library only.

littlekedi.bin, little-endian:
  header, 128 bytes: char magic[4] = "MLA1", int32 version = 1,
    int32 dim, n_layers, n_dense_layers, n_heads, qk_nope, qk_rope, v_head,
    kv_rank, dense_hidden, n_experts, n_active, moe_hidden, vocab, max_seq,
    float32 norm_eps, rope_theta, route_scale, zero padding
  float32 tensors, row-major [out][in]:
    per layer: attn_norm[dim], wq[nh*(nope+rope)][dim], wkv_a[(kv+rope)][dim],
      kv_norm[kv], wkv_b[nh*(nope+v)][kv], wo[dim][nh*v], ffn_norm[dim], then
      dense layers: w1[hd][dim], w2[dim][hd], w3[hd][dim]
      MoE layers: gate_w[ne][dim], gate_b[ne],
        expert e = 0..ne-1: gate[mh][dim], up[mh][dim], down[dim][mh],
        shared: w1[mh][dim], w2[dim][mh], w3[mh][dim]
    norm[dim], embed[vocab][dim] (also the output layer)

littlekedi.tok, little-endian:
  char magic[4] = "BPE1", int32 vocab, n_merges, n_special
  per token id: int32 len, raw bytes (GPT-2 byte-level decoded)
  per special: int32 id, int32 len, text bytes
  per merge, by rank: int32 left, right, result
"""
import json
import pathlib
import struct
import sys
from array import array


def load_safetensors(path):
    raw = pathlib.Path(path).read_bytes()
    n = struct.unpack("<Q", raw[:8])[0]
    hdr = json.loads(raw[8:8 + n])
    hdr.pop("__metadata__", None)
    out = {}
    for name, info in hdr.items():
        if info["dtype"] != "F32":
            raise SystemExit(f"error: {name} is {info['dtype']}, expected F32")
        a, b = info["data_offsets"]
        arr = array("f")
        arr.frombytes(raw[8 + n + a:8 + n + b])
        out[name] = (arr, info["shape"])
    return out


def transpose(arr, rows, cols):
    """[rows][cols] -> [cols][rows]"""
    out = array("f", bytes(4 * rows * cols))
    for r in range(rows):
        base = r * cols
        for c in range(cols):
            out[c * rows + r] = arr[base + c]
    return out


def bytes_to_unicode():
    bs = list(range(ord("!"), ord("~") + 1)) + list(range(ord("¡"), ord("¬") + 1)) + list(range(ord("®"), ord("ÿ") + 1))
    cs = bs[:]
    n = 0
    for b in range(256):
        if b not in bs:
            bs.append(b)
            cs.append(256 + n)
            n += 1
    return {chr(c): b for b, c in zip(bs, cs)}


def convert_model(hf, out):
    cfg = json.loads((hf / "config.json").read_text())
    w = load_safetensors(hf / "model.safetensors")
    nh, nope, rope, vd = cfg["n_heads"], cfg["qk_nope_head_dim"], cfg["qk_rope_head_dim"], cfg["v_head_dim"]
    dim, kv, ne, mh = cfg["dim"], cfg["kv_lora_rank"], cfg["n_routed_experts"], cfg["moe_hidden_dim"]
    if cfg.get("q_lora_rank", 0) != 0 or not cfg.get("tie_embeddings", False) or cfg.get("n_shared_experts", 1) != 1:
        raise SystemExit("error: only q_lora_rank 0, tied embeddings and one shared expert are supported")
    ints = [dim, cfg["n_layers"], cfg["n_dense_layers"], nh, nope, rope, vd, kv, cfg["dense_hidden_dim"],
            ne, cfg["n_activated_experts"], mh, cfg["vocab_size"], cfg["max_seq_len"]]
    header = b"MLA1" + struct.pack("<i", 1) + struct.pack(f"<{len(ints)}i", *ints)
    header += struct.pack("<3f", cfg["norm_eps"], cfg["rope_theta"], cfg["route_scale"])
    blob = bytearray(header.ljust(128, b"\0"))

    def put(name, expect):
        arr, shape = w[name]
        if list(shape) != list(expect):
            raise SystemExit(f"error: {name} has shape {shape}, expected {expect}")
        blob.extend(arr.tobytes())

    for l in range(cfg["n_layers"]):
        p = f"layers.{l}."
        put(p + "attn_norm.weight", [dim])
        put(p + "attn.wq.weight", [nh * (nope + rope), dim])
        put(p + "attn.wkv_a.weight", [kv + rope, dim])
        put(p + "attn.kv_norm.weight", [kv])
        put(p + "attn.wkv_b.weight", [nh * (nope + vd), kv])
        put(p + "attn.wo.weight", [dim, nh * vd])
        put(p + "ffn_norm.weight", [dim])
        if l < cfg["n_dense_layers"]:
            hd = cfg["dense_hidden_dim"]
            put(p + "ffn.w1.weight", [hd, dim])
            put(p + "ffn.w2.weight", [dim, hd])
            put(p + "ffn.w3.weight", [hd, dim])
            continue
        put(p + "ffn.gate.weight", [ne, dim])
        put(p + "ffn.gate.bias", [ne])
        g, gs = w[p + "ffn.experts.w_gate"]
        u, us = w[p + "ffn.experts.w_up"]
        d, ds = w[p + "ffn.experts.w_down"]
        assert gs == [ne, dim, mh] and us == [ne, dim, mh] and ds == [ne, mh, dim]
        for e in range(ne):  # stored [in][out]; written [out][in] like every other matrix
            blob.extend(transpose(g[e * dim * mh:(e + 1) * dim * mh], dim, mh).tobytes())
            blob.extend(transpose(u[e * dim * mh:(e + 1) * dim * mh], dim, mh).tobytes())
            blob.extend(transpose(d[e * mh * dim:(e + 1) * mh * dim], mh, dim).tobytes())
        put(p + "ffn.shared.w1.weight", [mh, dim])
        put(p + "ffn.shared.w2.weight", [dim, mh])
        put(p + "ffn.shared.w3.weight", [mh, dim])
    put("norm.weight", [dim])
    put("embed.weight", [cfg["vocab_size"], dim])
    (out / "littlekedi.bin").write_bytes(bytes(blob))
    return len(blob)


def convert_tokenizer(hf, out):
    t = json.loads((hf / "tokenizer.json").read_text(encoding="utf-8"))
    u2b = bytes_to_unicode()
    vocab = t["model"]["vocab"]
    special = {a["id"]: a["content"] for a in t["added_tokens"]}
    n_vocab = max(max(vocab.values()), max(special)) + 1
    toks = [b""] * n_vocab
    for s, i in vocab.items():
        toks[i] = special[i].encode() if i in special else bytes(u2b[c] for c in s)
    for i, s in special.items():
        toks[i] = s.encode()
    merges = [m.split(" ") if isinstance(m, str) else m for m in t["model"]["merges"]]
    blob = bytearray(b"BPE1" + struct.pack("<3i", n_vocab, len(merges), len(special)))
    for b in toks:
        blob += struct.pack("<i", len(b)) + b
    for i, s in sorted(special.items()):
        blob += struct.pack("<2i", i, len(s.encode())) + s.encode()
    for a, b in merges:
        blob += struct.pack("<3i", vocab[a], vocab[b], vocab[a + b])
    (out / "littlekedi.tok").write_bytes(bytes(blob))
    return n_vocab, len(merges), len(special)


def mla_tensors(c):
    """(name, rows, cols) of every tensor in file order; cols == 0 marks a vector."""
    dim, nh, nope, rope, vd, kv = c["dim"], c["nh"], c["nope"], c["rope"], c["vd"], c["kv"]
    out = []
    for l in range(c["layers"]):
        out += [("attn_norm", dim, 0), ("wq", nh * (nope + rope), dim), ("wkv_a", kv + rope, dim),
                ("kv_norm", kv, 0), ("wkv_b", nh * (nope + vd), kv), ("wo", dim, nh * vd), ("ffn_norm", dim, 0)]
        if l < c["dense"]:
            hd = c["dense_hidden"]
            out += [("w1", hd, dim), ("w2", dim, hd), ("w3", hd, dim)]
        else:
            ne, mh = c["experts"], c["moe_hidden"]
            out += [("gate_w", ne, dim), ("gate_b", ne, 0)]
            for _ in range(ne):
                out += [("e_gate", mh, dim), ("e_up", mh, dim), ("e_down", dim, mh)]
            out += [("s_w1", mh, dim), ("s_w2", dim, mh), ("s_w3", mh, dim)]
    out += [("norm", dim, 0), ("embed", c["vocab"], dim)]
    return out


FP32_MATRICES = ("gate_w",)


def read_mla_header(raw):
    if raw[:4] not in (b"MLA1", b"MLQ1"):
        raise SystemExit("error: not an MLA1/MLQ1 file")
    v = struct.unpack("<15i", raw[4:64])
    keys = ["version", "dim", "layers", "dense", "nh", "nope", "rope", "vd", "kv", "dense_hidden", "experts",
            "active", "moe_hidden", "vocab", "max_seq"]
    return dict(zip(keys, v))


def quantize_mla(src, dst, group_size=None):
    """MLA1 (fp32) -> MLQ1: vectors and the expert router stay fp32, other matrices become int8 + one
    fp32 scale per group (the router decides top-k between near-equal scores, so it keeps full precision).

    The group size is written at header offset 76; it must divide every matrix's input size."""
    sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
    import dslm
    raw = pathlib.Path(src).read_bytes()
    c = read_mla_header(raw)
    tensors = mla_tensors(c)
    inner = sorted({cols for _, _, cols in tensors if cols})
    gs = group_size or 64
    while gs > 1 and any(n % gs for n in inner):
        gs //= 2
    if any(n % gs for n in inner):
        raise SystemExit(f"error: group size {gs} does not divide {inner}")
    header = bytearray(raw[:128])
    header[:4] = b"MLQ1"
    header[76:80] = struct.pack("<i", gs)
    out = bytearray(header)
    pos = 128
    for name, rows, cols in tensors:
        n = rows * (cols or 1)
        vals = array("f")
        vals.frombytes(raw[pos:pos + 4 * n])
        pos += 4 * n
        out += vals.tobytes() if cols == 0 or name in FP32_MATRICES else dslm._quantize_tensor(vals, gs)
    if pos != len(raw):
        raise SystemExit(f"error: {src} has {len(raw) - pos} unexpected trailing bytes")
    pathlib.Path(dst).write_bytes(bytes(out))
    return gs, len(out)


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--quantize":
        gs, size = quantize_mla(sys.argv[2], sys.argv[3])
        print(f"wrote {sys.argv[3]} ({size} bytes, group size {gs})")
        return
    hf, out = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    size = convert_model(hf, out)
    nv, nm, ns = convert_tokenizer(hf, out)
    print(f"wrote {out / 'littlekedi.bin'} ({size} bytes) and {out / 'littlekedi.tok'} "
          f"(vocab {nv}, merges {nm}, special {ns})")


if __name__ == "__main__":
    main()
