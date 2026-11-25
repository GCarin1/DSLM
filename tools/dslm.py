#!/usr/bin/env python3
"""DSLM PC toolchain (spec: pc-toolchain; layouts: contracts/model-file-format).

    python tools/dslm.py fetch                  download the pinned model files
    python tools/dslm.py quantize               int8 copy of the model (llama2.c v2)
    python tools/dslm.py inspect [checkpoint]   header and DS memory budget
    python tools/dslm.py stage <dir> [--force]  write the /dslm card tree
    python tools/dslm.py golden [--kind K]      golden outputs: fp32, q80-runq, q80 (needs cc)
    python tools/dslm.py parity <tokens.txt>    compare a DS run with the golden
    python tools/dslm.py package [--force]      zip to unzip at the microSD root
    python tools/dslm.py bench <bench.txt> ...  import a hardware run into benchmarks/

Standard library only. golden compiles C, so run it in the Docker image.
"""
import argparse
import hashlib
import json
import pathlib
import shutil
import struct
import subprocess
import sys
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
CACHE = ROOT / ".cache"
GOLDEN_DIR = ROOT / "tests" / "golden"

DEFAULT_MODEL = "stories260K"
GOLDEN_STEPS = 128

HEADER = struct.Struct("<7i")
DS_RAM_BYTES = 4194304  # contracts/model-file-format, Budgets
DS_POINTER_BYTES = 4  # ARM9 is 32-bit

DEFAULT_INI = """[model]
checkpoint = /dslm/models/stories260K_q80.bin
tokenizer  = /dslm/models/tok512.bin

[sampling]
temperature = 0.0
topp        = 0.9
steps       = 256
seed        = 0
"""


def load_pins():
    return json.loads((TOOLS / "models.json").read_text(encoding="utf-8"))


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def fetch_pinned(url, dest, sha256, size=None):
    """Download `url` to `dest` unless a file with the right hash is there."""
    dest = pathlib.Path(dest)
    if dest.exists() and sha256_file(dest) == sha256:
        return dest
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name + ".part")
    with urllib.request.urlopen(url) as resp, open(tmp, "wb") as out:
        shutil.copyfileobj(resp, out)
    got = sha256_file(tmp)
    if got != sha256 or (size is not None and tmp.stat().st_size != size):
        tmp.unlink()
        raise SystemExit(f"error: {url}: SHA-256 {got} does not match the pin {sha256}")
    tmp.replace(dest)
    return dest


def model_paths(model=DEFAULT_MODEL):
    entry = load_pins()["models"][model]
    base = CACHE / "models"
    return base / entry["checkpoint"]["file"], base / entry["tokenizer"]["file"]


def fetch(model=DEFAULT_MODEL):
    entry = load_pins()["models"][model]
    paths = []
    for kind in ("checkpoint", "tokenizer"):
        f = entry[kind]
        paths.append(fetch_pinned(f["url"], CACHE / "models" / f["file"], f["sha256"], f["size"]))
    return paths


# ---- inspect -------------------------------------------------------------

def read_header(path):
    with open(path, "rb") as f:
        raw = f.read(HEADER.size)
    if len(raw) < HEADER.size:
        raise ValueError(f"{path}: too short for a llama2.c header")
    dim, hidden, layers, heads, kv_heads, vocab, seq_len = HEADER.unpack(raw)
    return {
        "dim": dim,
        "hidden_dim": hidden,
        "n_layers": layers,
        "n_heads": heads,
        "n_kv_heads": kv_heads,
        "vocab_size": abs(vocab),
        "shared_classifier": vocab > 0,
        "seq_len": seq_len,
    }


def weight_floats(h):
    """float32 count after the header, in the contract's order."""
    d, hd, L, V, S = h["dim"], h["hidden_dim"], h["n_layers"], h["vocab_size"], h["seq_len"]
    head = d // h["n_heads"]
    kv_dim = d * h["n_kv_heads"] // h["n_heads"]
    n = V * d                       # token_embedding_table
    n += L * d                      # rms_att_weight
    n += L * d * d                  # wq
    n += 2 * L * d * kv_dim         # wk, wv
    n += L * d * d                  # wo
    n += L * d                      # rms_ffn_weight
    n += 3 * L * d * hd             # w1, w2, w3
    n += d                          # rms_final_weight
    n += S * head                   # two legacy RoPE tables, S * head / 2 each
    if not h["shared_classifier"]:
        n += V * d                  # wcls
    return n


def tokenizer_ram(path, vocab_size):
    """Bytes run.c's Tokenizer holds on a 32-bit target."""
    with open(path, "rb") as f:
        f.read(4)  # max_token_length
        strings = 0
        for _ in range(vocab_size):
            raw = f.read(8)
            if len(raw) < 8:
                raise ValueError(f"{path}: truncated before token {_}")
            _score, length = struct.unpack("<fi", raw)
            f.seek(length, 1)
            strings += length + 1
    vocab_ptrs = vocab_size * DS_POINTER_BYTES
    scores = vocab_size * 4
    sorted_vocab = vocab_size * (DS_POINTER_BYTES + 4)
    byte_pieces = 512
    return vocab_ptrs + scores + strings + sorted_vocab + byte_pieces


def memory_budget(checkpoint, tokenizer):
    h = read_header(checkpoint)
    expected = HEADER.size + 4 * weight_floats(h)
    actual = pathlib.Path(checkpoint).stat().st_size
    if actual != expected:
        raise ValueError(f"{checkpoint}: {actual} bytes, header implies {expected}")
    d, hd, L, S = h["dim"], h["hidden_dim"], h["n_layers"], h["seq_len"]
    kv_dim = d * h["n_kv_heads"] // h["n_heads"]
    budget = {
        "weights": actual - HEADER.size,
        "kv_cache": 2 * L * S * kv_dim * 4,
        "run_state": 4 * (4 * d + 2 * hd + h["n_heads"] * S + h["vocab_size"]),
        "tokenizer": tokenizer_ram(tokenizer, h["vocab_size"]),
    }
    budget["total"] = sum(budget.values())
    return h, budget


def inspect(checkpoint=None, tokenizer=None):
    ckpt, tok = model_paths()
    checkpoint = pathlib.Path(checkpoint or ckpt)
    tokenizer = pathlib.Path(tokenizer or tok)
    h, b = memory_budget(checkpoint, tokenizer)
    print(f"checkpoint {checkpoint.name}  {checkpoint.stat().st_size} bytes  sha256 {sha256_file(checkpoint)}")
    for k, v in h.items():
        print(f"  {k:18} {v}")
    print("DS RAM needed (32-bit pointers, fp32):")
    for k, v in b.items():
        print(f"  {k:18} {v:>9} bytes")
    fits = b["total"] < DS_RAM_BYTES
    print(f"  {'budget':18} {DS_RAM_BYTES:>9} bytes  -> {'fits' if fits else 'DOES NOT FIT'}"
          f" ({100 * b['total'] / DS_RAM_BYTES:.1f}%, excluding code and stack)")
    return h, b


# ---- quantize (llama2.c v2 int8, "ak42") ---------------------------------

Q80_MAGIC = 0x616B3432
Q80_HEADER_BYTES = 256


def q80_path(model=DEFAULT_MODEL):
    return CACHE / "models" / f"{model}_q80.bin"


def pick_group_size(h, start=64):
    """Largest power of two <= start dividing dim and hidden_dim.

    runq.c's matmul only walks whole groups (j <= n - GS), so GS must divide
    both inner sizes it sees (dim and hidden_dim) or values are dropped.
    """
    gs = start
    while gs > 1 and (h["dim"] % gs or h["hidden_dim"] % gs):
        gs //= 2
    return gs


def _quantize_tensor(values, gs):
    """Q8_0 like export.py's quantize_q80: int8 = round(w / (max|w| / 127))."""
    from array import array
    q = bytearray(len(values))
    scales = array("f")
    for g in range(0, len(values), gs):
        group = values[g:g + gs]
        wmax = max(abs(v) for v in group)
        scale = wmax / 127.0
        scales.append(scale)
        for i, v in enumerate(group):
            qi = round(v / scale) if scale else 0
            q[g + i] = qi & 0xFF
    return bytes(q) + scales.tobytes()


def quantize_checkpoint(src, dst, group_size=None):
    """Write the int8 v2 checkpoint runq.c reads. Returns the group size."""
    from array import array
    h = read_header(src)
    raw = pathlib.Path(src).read_bytes()
    expected = HEADER.size + 4 * weight_floats(h)
    if len(raw) != expected:
        raise ValueError(f"{src}: {len(raw)} bytes, header implies {expected}")
    f = array("f")
    f.frombytes(raw[HEADER.size:])
    if sys.byteorder != "little":
        f.byteswap()

    d, hd, L, V, S = h["dim"], h["hidden_dim"], h["n_layers"], h["vocab_size"], h["seq_len"]
    head = d // h["n_heads"]
    kv = d * h["n_kv_heads"] // h["n_heads"]
    gs = group_size or pick_group_size(h)
    if d % gs or hd % gs:
        raise ValueError(f"group size {gs} must divide dim {d} and hidden_dim {hd}")

    pos = 0

    def take(n):
        nonlocal pos
        out = f[pos:pos + n]
        pos += n
        return out

    emb = take(V * d)
    rms_att = take(L * d)
    wq = [take(d * d) for _ in range(L)]
    wk = [take(d * kv) for _ in range(L)]
    wv = [take(d * kv) for _ in range(L)]
    wo = [take(d * d) for _ in range(L)]
    rms_ffn = take(L * d)
    w1 = [take(hd * d) for _ in range(L)]
    w2 = [take(d * hd) for _ in range(L)]
    w3 = [take(hd * d) for _ in range(L)]
    rms_final = take(d)
    take(S * head)  # legacy RoPE tables, not in v2
    wcls = None if h["shared_classifier"] else take(V * d)

    header = struct.pack("<Ii7iBi", Q80_MAGIC, 2, d, hd, L, h["n_heads"], h["n_kv_heads"], V, S,
                         1 if h["shared_classifier"] else 0, gs)
    out = bytearray(header.ljust(Q80_HEADER_BYTES, b"\0"))
    out += rms_att.tobytes() + rms_ffn.tobytes() + rms_final.tobytes()
    for t in [emb] + wq + wk + wv + wo + w1 + w2 + w3 + ([wcls] if wcls is not None else []):
        out += _quantize_tensor(t, gs)
    dst = pathlib.Path(dst)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(bytes(out))
    return gs


def read_q80_header(path):
    raw = pathlib.Path(path).read_bytes()[:41]
    magic, version, d, hd, L, nh, nkv, V, S, shared, gs = struct.unpack("<Ii7iBi", raw)
    if magic != Q80_MAGIC or version != 2:
        raise ValueError(f"{path}: not a llama2.c v2 int8 checkpoint")
    return {"dim": d, "hidden_dim": hd, "n_layers": L, "n_heads": nh, "n_kv_heads": nkv,
            "vocab_size": V, "seq_len": S, "shared_classifier": bool(shared), "group_size": gs}


def q80_bytes(h, gs):
    """File size runq.c expects for a v2 checkpoint with this header."""
    d, hd, L, V = h["dim"], h["hidden_dim"], h["n_layers"], h["vocab_size"]
    kv = d * h["n_kv_heads"] // h["n_heads"]

    def qt(n):
        return n + 4 * (n // gs)

    n = Q80_HEADER_BYTES + 4 * (2 * L * d + d) + qt(V * d)
    n += L * (qt(d * d) + 2 * qt(d * kv) + qt(d * d) + 3 * qt(d * hd))
    if not h["shared_classifier"]:
        n += qt(V * d)
    return n


def ensure_q80(model=DEFAULT_MODEL):
    """Fetch the fp32 model and (re)build its int8 copy when missing or stale."""
    ckpt, tok = fetch(model)
    q = q80_path(model)
    if not q.exists() or q.stat().st_mtime < ckpt.stat().st_mtime:
        quantize_checkpoint(ckpt, q)
    return q, tok


# ---- stage ---------------------------------------------------------------

def stage(target, checkpoint, tokenizer, force=False, extra=()):
    """Write <target>/dslm/{dslm.ini, models/, manifest.txt}.

    `extra` lists more checkpoints to copy next to the default one."""
    target = pathlib.Path(target)
    models = target / "dslm" / "models"
    models.mkdir(parents=True, exist_ok=True)
    staged = []
    for src in [pathlib.Path(checkpoint), *map(pathlib.Path, extra), pathlib.Path(tokenizer)]:
        dst = models / src.name
        shutil.copyfile(src, dst)
        staged.append(dst)
    ini = target / "dslm" / "dslm.ini"
    if force or not ini.exists():
        ini.write_text(DEFAULT_INI, encoding="utf-8", newline="\n")
    lines = [f"/dslm/models/{p.name} {p.stat().st_size} {sha256_file(p)}" for p in staged]
    (target / "dslm" / "manifest.txt").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    return lines


# ---- golden --------------------------------------------------------------

def read_prompts(path=TOOLS / "prompts.txt"):
    prompts = []
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        prompts.append("" if line.strip() == "<empty>" else line)
    return prompts


def build_golden_binary(cc="cc", quantized=False):
    pin = load_pins()["llama2c"]
    src_dir = CACHE / "llama2c" / pin["commit"]
    fetch_pinned(pin["run_c_url"], src_dir / "run.c", pin["run_c_sha256"])
    fetch_pinned(pin["runq_c_url"], src_dir / "runq.c", pin["runq_c_sha256"])
    exe = CACHE / ("goldenq" if quantized else "golden")
    # No fast-math and no FMA contraction: plain IEEE fp32, the DS reference.
    source = ['-DLLAMA2C_SOURCE="runq.c"'] if quantized else []
    cmd = [cc, "-O2", "-std=gnu11", "-ffp-contract=off", f"-I{src_dir}", *source,
           "-o", str(exe), str(TOOLS / "golden" / "golden.c"), "-lm"]
    subprocess.run(cmd, check=True)
    return exe


def run_golden(exe, checkpoint, tokenizer, prompt, steps):
    out = subprocess.run([str(exe), str(checkpoint), str(tokenizer), str(steps), prompt],
                         check=True, capture_output=True, text=True).stdout
    rec = {}
    for line in out.splitlines():
        tag, _, rest = line.partition(" ")
        rec[tag] = rest
    tokens = [int(t) for t in rec["T"].split()]
    return {
        "prompt": prompt,
        "prompt_tokens": int(rec["P"]),
        "tokens": tokens,
        "text": bytes.fromhex(rec["S"]).decode("utf-8", errors="replace"),
    }


ENGINE_SOURCES = ["src/engine/llama2.c", "src/engine/llama2_fixed.arm.c", "src/engine/fxmath.arm.c"]

# kind -> (golden file suffix, checkpoint, reference that produces it)
GOLDEN_KINDS = {
    "fp32": ("", "fp32", "run.c"),
    "q80-runq": ("_q80_runq", "q80", "runq.c"),
    "q80": ("_q80", "q80", "dslm engine (host build)"),
}


def build_engine_binary(cc="cc"):
    """The DS engine built for the host: the exact reference for int8 DS runs."""
    exe = CACHE / "engine_main"
    exe.parent.mkdir(parents=True, exist_ok=True)
    cmd = [cc, "-O2", "-std=c99", "-Wall", "-Wextra", "-Werror", "-ffp-contract=off",
           f"-I{ROOT / 'src' / 'engine'}", "-o", str(exe), str(ROOT / "tests" / "host" / "engine_main.c"),
           *[str(ROOT / s) for s in ENGINE_SOURCES], "-lm"]
    subprocess.run(cmd, check=True)
    return exe


def golden(out_path=None, force=False, steps=GOLDEN_STEPS, model=DEFAULT_MODEL, kind="fp32"):
    suffix, ckpt_kind, reference = GOLDEN_KINDS[kind]
    name = f"{model}{suffix}"
    out_path = pathlib.Path(out_path or GOLDEN_DIR / f"{name}.json")
    if out_path.exists() and not force:
        raise SystemExit(f"error: {out_path} exists; pass --force to overwrite it")
    checkpoint, tokenizer = ensure_q80(model) if ckpt_kind == "q80" else fetch(model)
    if kind == "q80":
        exe = build_engine_binary()
    else:
        exe = build_golden_binary(quantized=kind == "q80-runq")
    doc = {
        "model": name,
        "reference": reference,
        "llama2c_commit": load_pins()["llama2c"]["commit"],
        "checkpoint_sha256": sha256_file(checkpoint),
        "tokenizer_sha256": sha256_file(tokenizer),
        "temperature": 0.0,
        "steps": steps,
        "runs": [run_golden(exe, checkpoint, tokenizer, p, steps) for p in read_prompts()],
    }
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(doc, indent=1) + "\n", encoding="utf-8", newline="\n")
    return out_path


# ---- package -------------------------------------------------------------

PACKAGE_README = """DSLM - a tiny Llama 2 language model for the Nintendo DS
==========================================================

DSLM writes short children's stories in English, entirely offline. The
model (stories260K, about 1 MB) is in this package; the DS never needs
the internet.

What you need
-------------
- A Nintendo DS or DS Lite.
- An R4 (or any DLDI-compatible) flashcart whose kernel is already
  installed on its microSD card. Kernels and DLDI drivers are NOT part
  of this package.

Install
-------
1. Unzip this package to the ROOT of the flashcart's microSD card.
   You should see dslm.nds and a folder named dslm next to the kernel
   files. Keep the dslm folder exactly as it is:
       dslm/dslm.ini
       dslm/models/stories260K_q80.bin   (int8, the fast default)
       dslm/models/stories260K.bin       (fp32 reference, slower)
       dslm/models/tok512.bin
   (dslm.nds may also go in another folder your kernel lists, but the
   dslm folder must stay at the root.)
2. Put the card back in the flashcart, boot the DS and launch dslm.nds
   from the kernel menu.

Controls
--------
Every action works with buttons or with the stylus.
  Prompt:      type on the touch keyboard, B deletes, L/R example
               prompts, START or GO generates, SELECT opens settings.
  Generating:  hold B or touch STOP to stop and keep the text.
  Story:       Up/Down scroll, A new prompt, Y same prompt again.
  Settings:    Up/Down choose, Left/Right change, START saves, B back.
Each token takes about half a second; a 256-token story takes minutes.

Troubleshooting
---------------
- "FAT mount failed": your kernel did not patch the DLDI driver. Patch
  dslm.nds with dlditool and the driver for your cart, then retry.
- "... file not found": the dslm folder is not at the card root, or a
  file is missing. Unzip the package again.
- "Model does not fit": dslm.ini points at a model that is too big for
  the DS's 4 MB of RAM.

Files in this package
---------------------
MANIFEST.txt lists every file with its size and SHA-256.
LICENSE.txt is DSLM's license (MIT).
THIRD_PARTY.txt has the licenses of the model and of llama2.c.
"""


def _zip_entry(zf, name, data):
    import zipfile
    info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o644 << 16
    zf.writestr(info, data)


def third_party_notice(model=DEFAULT_MODEL):
    pins = load_pins()
    entry = pins["models"][model]
    license_text = (ROOT / "third_party" / "llama2.c" / "LICENSE").read_text(encoding="utf-8")
    return (
        "Third-party components\n"
        "======================\n\n"
        f"Model {model} ({entry['checkpoint']['file']}, {entry['tokenizer']['file']})\n"
        f"  Source: {entry['checkpoint']['url']}\n"
        f"          {entry['tokenizer']['url']}\n"
        "  Author: Andrej Karpathy (karpathy/tinyllamas). License: MIT.\n"
        f"  {model}_q80.bin is the same model quantized to int8 by DSLM's tools/dslm.py.\n\n"
        "llama2.c (the inference code dslm.nds is ported from)\n"
        f"  Source: https://github.com/karpathy/llama2.c (commit {pins['llama2c']['commit']})\n"
        "  License: MIT, text below. The same MIT terms cover the model files.\n\n"
        + license_text
    )


def package(out_path, nds_path, checkpoint, tokenizer, force=False, extra=()):
    """Write a zip to unzip at the microSD root: dslm.nds + /dslm + docs."""
    import tempfile
    import zipfile

    out_path = pathlib.Path(out_path)
    nds_path = pathlib.Path(nds_path)
    if out_path.exists() and not force:
        raise SystemExit(f"error: {out_path} exists; pass --force to overwrite it")
    if not nds_path.is_file():
        raise SystemExit(f"error: {nds_path} not found; build it first (make nds)")

    with tempfile.TemporaryDirectory() as tmp:
        card = pathlib.Path(tmp) / "card"
        stage(card, checkpoint, tokenizer, force=True, extra=extra)
        files = {"dslm.nds": nds_path.read_bytes()}
        for p in sorted((card / "dslm").rglob("*")):
            if p.is_file() and p.name != "manifest.txt":
                files[p.relative_to(card).as_posix()] = p.read_bytes()

    manifest = "".join(
        f"{name} {len(data)} {hashlib.sha256(data).hexdigest()}\n" for name, data in sorted(files.items()))
    files["MANIFEST.txt"] = manifest.encode()
    files["README-DSLM.txt"] = PACKAGE_README.encode()
    files["THIRD_PARTY.txt"] = third_party_notice().encode()
    files["LICENSE.txt"] = (ROOT / "LICENSE").read_bytes()

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(out_path, "w") as zf:
        for name in sorted(files):
            _zip_entry(zf, name, files[name])
    return out_path, sorted(files)


# ---- bench ---------------------------------------------------------------

BENCH_DIR = ROOT / "benchmarks"


def read_bench_blocks(path):
    """Blocks of /dslm/bench.txt (key=value lines, blank line between runs)."""
    blocks, cur = [], {}
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip():
            if cur:
                blocks.append(cur)
                cur = {}
            continue
        key, sep, value = line.partition("=")
        if sep:
            cur[key] = value
    if cur:
        blocks.append(cur)
    return blocks


def _slug(text):
    return "-".join("".join(c.lower() if c.isalnum() else " " for c in text).split()) or "cart"


def git_commit():
    try:
        return subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, check=True,
                              capture_output=True, text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def import_bench(bench_path, cart, kernel, microsd, emulated, out_path=None, date=None, force=False):
    """Write the last run of bench.txt as a benchmarks/ file with the header from benchmarks/README.md."""
    import datetime
    blocks = read_bench_blocks(bench_path)
    if not blocks:
        raise SystemExit(f"error: {bench_path} has no runs")
    if emulated not in ("yes", "no"):
        raise SystemExit("error: --emulated must be yes or no")
    run = dict(blocks[-1])
    run["emulated"] = emulated
    if out_path is None:
        if emulated == "yes":
            raise SystemExit("error: emulator runs never go to benchmarks/ (spec test-harness); pass --out")
        date = date or datetime.date.today().isoformat()
        out_path = BENCH_DIR / f"{date}-{_slug(cart)}-{run.get('build', 'run')}.txt"
    out_path = pathlib.Path(out_path)
    if out_path.exists() and not force:
        raise SystemExit(f"error: {out_path} exists; pass --force to overwrite it")
    header = {"cart": cart, "kernel": kernel, "microsd": microsd, "commit": git_commit()}
    text = "".join(f"{k}={v}\n" for k, v in header.items()) + "".join(f"{k}={v}\n" for k, v in run.items())
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(text, encoding="utf-8", newline="\n")
    return out_path, run


# ---- parity --------------------------------------------------------------

def read_ds_tokens(path):
    """Parse /dslm/tokens.txt (key=value lines) written by the DS skeleton."""
    rec = {}
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        key, sep, value = line.partition("=")
        if sep:
            rec[key] = value
    rec["tokens"] = [int(t) for t in rec.get("tokens", "").split()]
    return rec


def compare_tokens(ds, golden_tokens):
    """(compared, first diverging index or None)."""
    n = min(len(ds), len(golden_tokens))
    for i in range(n):
        if ds[i] != golden_tokens[i]:
            return n, i
    return n, None


def parity(tokens_path, golden_path=None):
    ds = read_ds_tokens(tokens_path)
    if not golden_path:
        # The DS records the checkpoint it ran; older runs did not (fp32 then).
        stem = pathlib.PurePosixPath(ds.get("checkpoint", f"{DEFAULT_MODEL}.bin")).stem
        golden_path = GOLDEN_DIR / f"{stem}.json"
    golden_path = pathlib.Path(golden_path)
    doc = json.loads(golden_path.read_text(encoding="utf-8"))
    runs = [r for r in doc["runs"] if r["prompt"] == ds.get("prompt")]
    if not runs:
        raise SystemExit(f"error: no golden run for prompt {ds.get('prompt')!r}")
    gold = runs[0]["tokens"]
    n, bad = compare_tokens(ds["tokens"], gold)
    if bad is not None:
        print(f"DIVERGED at index {bad}: ds={ds['tokens'][bad]} golden={gold[bad]} ({n} compared)")
        return False
    if len(ds["tokens"]) > len(gold):
        print(f"note: the DS ran {len(ds['tokens'])} tokens, the golden has {len(gold)}; compared the prefix")
    print(f"parity ok: {n} tokens identical (prompt {ds['prompt']!r})")
    return True


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("fetch")
    sub.add_parser("quantize")
    p = sub.add_parser("inspect")
    p.add_argument("checkpoint", nargs="?")
    p.add_argument("tokenizer", nargs="?")
    p = sub.add_parser("stage")
    p.add_argument("target")
    p.add_argument("--force", action="store_true", help="also overwrite an existing dslm.ini")
    p = sub.add_parser("golden")
    p.add_argument("--force", action="store_true", help="overwrite an existing golden file")
    p.add_argument("--out")
    p.add_argument("--kind", choices=sorted(GOLDEN_KINDS), default="fp32",
                   help="fp32 (run.c), q80-runq (runq.c, quality reference) or q80 (DS engine, exact DS reference)")
    p = sub.add_parser("package")
    p.add_argument("--nds", default=str(ROOT / "dslm.nds"))
    p.add_argument("--out", default=str(ROOT / "build" / "dslm-package.zip"))
    p.add_argument("--force", action="store_true", help="overwrite an existing package")
    p = sub.add_parser("bench", help="import the last run of /dslm/bench.txt into benchmarks/")
    p.add_argument("bench", help="path to bench.txt (e.g. E:/dslm/bench.txt)")
    p.add_argument("--cart", required=True)
    p.add_argument("--kernel", required=True)
    p.add_argument("--microsd", required=True)
    p.add_argument("--emulated", required=True, choices=["yes", "no"])
    p.add_argument("--out")
    p.add_argument("--force", action="store_true")
    p = sub.add_parser("parity")
    p.add_argument("tokens", help="path to tokens.txt written by the DS (e.g. .tools/card/dslm/tokens.txt)")
    p.add_argument("--golden")
    args = ap.parse_args(argv)

    if args.cmd == "fetch":
        for path in fetch():
            print(f"ok {path.relative_to(ROOT)}  {sha256_file(path)}")
    elif args.cmd == "inspect":
        if not args.checkpoint:
            fetch()
        inspect(args.checkpoint, args.tokenizer)
    elif args.cmd == "quantize":
        q, _ = ensure_q80()
        ckpt, _ = model_paths()
        print(f"fp32 {ckpt.relative_to(ROOT)}  {ckpt.stat().st_size} bytes")
        print(f"int8 {q.relative_to(ROOT)}  {q.stat().st_size} bytes  group_size {read_q80_header(q)['group_size']}"
              f"  sha256 {sha256_file(q)}")
    elif args.cmd == "stage":
        ckpt, _ = fetch()
        q, tok = ensure_q80()
        for line in stage(args.target, q, tok, args.force, extra=[ckpt]):
            print(f"staged {line}")
    elif args.cmd == "golden":
        print(f"wrote {golden(args.out, args.force, kind=args.kind)}")
    elif args.cmd == "package":
        ckpt, _ = fetch()
        q, tok = ensure_q80()
        out, names = package(args.out, args.nds, q, tok, args.force, extra=[ckpt])
        print(f"wrote {out} ({out.stat().st_size} bytes)")
        for n in names:
            print(f"  {n}")
    elif args.cmd == "bench":
        out, run = import_bench(args.bench, args.cart, args.kernel, args.microsd, args.emulated,
                                args.out, force=args.force)
        print(f"wrote {out}: {run.get('build')} {run.get('tok_per_s')} tokens/s, "
              f"load {run.get('load_ms')} ms, heap peak {run.get('heap_peak_bytes')} bytes")
    elif args.cmd == "parity":
        return 0 if parity(args.tokens, args.golden) else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
