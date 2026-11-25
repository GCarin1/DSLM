#!/usr/bin/env python3
"""fp32 reference for LittleKedi (DeepSeek-V3-style MLA + MoE), numpy only.

It is the oracle for the C engine: it reads model.safetensors, config.json
and tokenizer.json from the Hugging Face repo and generates greedily.

    python3 tools/m8/littlekedi_ref.py <dir> "<prompt>" [steps] [--rope interleaved|half]
"""
import json
import re
import struct
import sys

import numpy as np

F16_WEIGHTS = "--f16" in sys.argv
F16_ACTS = "--f16-acts" in sys.argv  # mimic llama.cpp CPU f16 matmuls (activations rounded)


def mm(w, x):
    if F16_ACTS:
        x = x.astype(np.float16).astype(np.float32)
    return w @ x


def vm(x, w):
    if F16_ACTS:
        x = x.astype(np.float16).astype(np.float32)
    return x @ w


# ---- weights --------------------------------------------------------------

def load_safetensors(path):
    raw = open(path, "rb").read()
    n = struct.unpack("<Q", raw[:8])[0]
    hdr = json.loads(raw[8:8 + n])
    hdr.pop("__metadata__", None)
    base = 8 + n
    out = {}
    for name, info in hdr.items():
        assert info["dtype"] == "F32", name
        a, b = info["data_offsets"]
        arr = np.frombuffer(raw[base + a:base + b], dtype=np.float32).reshape(info["shape"])
        if F16_WEIGHTS and arr.ndim >= 2:  # mimic an F16 GGUF: matrices in f16, vectors in f32
            arr = arr.astype(np.float16).astype(np.float32)
        out[name] = arr
    return out


# ---- GPT-2 byte-level BPE -------------------------------------------------

def bytes_to_unicode():
    bs = list(range(ord("!"), ord("~") + 1)) + list(range(ord("¡"), ord("¬") + 1)) + list(range(ord("®"), ord("ÿ") + 1))
    cs = bs[:]
    n = 0
    for b in range(256):
        if b not in bs:
            bs.append(b)
            cs.append(256 + n)
            n += 1
    return {b: chr(c) for b, c in zip(bs, cs)}


# GPT-2 pre-tokenizer; [^\W\d_] stands in for \p{L}, \d for \p{N}.
PRETOK = re.compile(r"""'s|'t|'re|'ve|'m|'ll|'d| ?[^\W\d_]+| ?\d+| ?(?:[^\s\w]|_)+|\s+(?!\S)|\s+""")


class Tokenizer:
    def __init__(self, path):
        t = json.load(open(path, encoding="utf-8"))
        self.vocab = dict(t["model"]["vocab"])
        merges = t["model"]["merges"]
        merges = [tuple(m.split(" ")) if isinstance(m, str) else tuple(m) for m in merges]
        self.ranks = {m: i for i, m in enumerate(merges)}
        self.special = {a["content"]: a["id"] for a in t["added_tokens"]}
        self.b2u = bytes_to_unicode()
        self.u2b = {v: k for k, v in self.b2u.items()}
        self.id2tok = {i: s for s, i in self.vocab.items()}
        for s, i in self.special.items():
            self.id2tok[i] = s

    def bpe(self, word):
        parts = list(word)
        while len(parts) > 1:
            best = min(((self.ranks.get((parts[i], parts[i + 1]), 1 << 30), i) for i in range(len(parts) - 1)))
            if best[0] == 1 << 30:
                break
            i = best[1]
            parts[i:i + 2] = [parts[i] + parts[i + 1]]
        return [self.vocab[p] for p in parts]

    def encode(self, text):
        ids = []
        pattern = "(" + "|".join(re.escape(s) for s in sorted(self.special, key=len, reverse=True)) + ")"
        for chunk in re.split(pattern, text):
            if not chunk:
                continue
            if chunk in self.special:
                ids.append(self.special[chunk])
                continue
            for word in PRETOK.findall(chunk):
                ids += self.bpe("".join(self.b2u[b] for b in word.encode("utf-8")))
        return ids

    def decode(self, ids):
        out = bytearray()
        for i in ids:
            s = self.id2tok[i]
            if i in self.special.values():
                continue
            out += bytes(self.u2b[c] for c in s)
        return out.decode("utf-8", errors="replace")


# ---- model ----------------------------------------------------------------

def rms(x, w, eps):
    return w * (x / np.sqrt(np.mean(x * x) + eps))


def silu(x):
    return x / (1.0 + np.exp(-x))


class Model:
    def __init__(self, d, rope="interleaved"):
        cfg = json.load(open(f"{d}/config.json"))
        self.c = cfg
        self.w = load_safetensors(f"{d}/model.safetensors")
        self.rope_mode = rope
        self.nh = cfg["n_heads"]
        self.nope, self.rd, self.vd = cfg["qk_nope_head_dim"], cfg["qk_rope_head_dim"], cfg["v_head_dim"]
        self.kvr = cfg["kv_lora_rank"]
        self.eps = cfg["norm_eps"]
        self.scale = (self.nope + self.rd) ** -0.5
        if "--scale48" in sys.argv:  # experiment: 1/sqrt(kv_lora_rank + rope dim)
            self.scale = (self.kvr + self.rd) ** -0.5
        self.cache = [[] for _ in range(cfg["n_layers"])]
        i = np.arange(0, self.rd, 2, dtype=np.float64)
        self.freqs = 1.0 / (cfg["rope_theta"] ** (i / self.rd))

    def rope(self, v, pos):
        ang = pos * self.freqs
        c, s = np.cos(ang).astype(np.float32), np.sin(ang).astype(np.float32)
        out = v.copy()
        if self.rope_mode == "interleaved":
            a, b = v[..., 0::2], v[..., 1::2]
            out[..., 0::2], out[..., 1::2] = a * c - b * s, a * s + b * c
        else:
            h = self.rd // 2
            a, b = v[..., :h], v[..., h:]
            out[..., :h], out[..., h:] = a * c - b * s, a * s + b * c
        return out

    def attn(self, l, x, pos):
        w, p = self.w, f"layers.{l}.attn."
        q = mm(w[p + "wq.weight"], x).reshape(self.nh, self.nope + self.rd)
        q_nope, q_pe = q[:, :self.nope], self.rope(q[:, self.nope:], pos)
        kv = mm(w[p + "wkv_a.weight"], x)
        lat = rms(kv[:self.kvr], w[p + "kv_norm.weight"], self.eps)
        k_pe = self.rope(kv[self.kvr:], pos)
        self.cache[l].append((lat, k_pe))
        wkv_b = w[p + "wkv_b.weight"].reshape(self.nh, self.nope + self.vd, self.kvr)
        wk, wv = wkv_b[:, :self.nope, :], wkv_b[:, self.nope:, :]
        lats = np.stack([c[0] for c in self.cache[l]])   # (T, kvr)
        kpes = np.stack([c[1] for c in self.cache[l]])   # (T, rd)
        out = np.zeros((self.nh, self.vd), dtype=np.float32)
        for h in range(self.nh):
            q_lat = q_nope[h] @ wk[h]                      # absorbed: (kvr,)
            s = (lats @ q_lat + kpes @ q_pe[h]) * self.scale
            s = np.exp(s - s.max())
            s /= s.sum()
            out[h] = wv[h] @ (s @ lats)
        return mm(w[p + "wo.weight"], out.reshape(-1))

    def ffn(self, l, x):
        w, p = self.w, f"layers.{l}.ffn."
        if l < self.c["n_dense_layers"]:
            return mm(w[p + "w2.weight"], silu(mm(w[p + "w1.weight"], x)) * mm(w[p + "w3.weight"], x))
        scores = 1.0 / (1.0 + np.exp(-mm(w[p + "gate.weight"], x)))
        choice = scores + w[p + "gate.bias"]
        idx = np.argsort(-choice, kind="stable")[: self.c["n_activated_experts"]]
        wts = scores[idx] / scores[idx].sum() * self.c["route_scale"]
        y = np.zeros_like(x)
        for e, wt in zip(idx, wts):
            h = silu(vm(x, w[p + "experts.w_gate"][e])) * vm(x, w[p + "experts.w_up"][e])
            y += wt * vm(h, w[p + "experts.w_down"][e])
        sh = mm(w[p + "shared.w2.weight"], silu(mm(w[p + "shared.w1.weight"], x)) * mm(w[p + "shared.w3.weight"], x))
        return y + sh

    def forward(self, tok, pos):
        w = self.w
        x = w["embed.weight"][tok].copy()
        for l in range(self.c["n_layers"]):
            x = x + self.attn(l, rms(x, w[f"layers.{l}.attn_norm.weight"], self.eps), pos)
            x = x + self.ffn(l, rms(x, w[f"layers.{l}.ffn_norm.weight"], self.eps))
        x = rms(x, w["norm.weight"], self.eps)
        return mm(w["embed.weight"], x)


def generate(d, prompt, steps=40, rope="interleaved"):
    tk = Tokenizer(f"{d}/tokenizer.json")
    m = Model(d, rope)
    ids = tk.encode(prompt)
    out = list(ids)
    end = tk.special.get("<|end|>")
    for pos in range(len(ids) + steps - 1):
        logits = m.forward(out[pos], pos)
        if pos >= len(ids) - 1:
            nxt = int(np.argmax(logits))
            if nxt in (end, tk.special.get("<|endoftext|>")):
                break
            out.append(nxt)
    return ids, out, tk.decode(out[len(ids):])


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    rope = "half" if "--rope" in sys.argv and sys.argv[sys.argv.index("--rope") + 1] == "half" else "interleaved"
    args = [a for a in args if a not in ("half", "interleaved")]
    ids, out, text = generate(args[0], args[1], int(args[2]) if len(args) > 2 else 40, rope)
    print("prompt ids", ids)
    print("text", repr(text))
