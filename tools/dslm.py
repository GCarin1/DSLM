#!/usr/bin/env python3
"""DSLM PC toolchain (spec: pc-toolchain; layouts: contracts/model-file-format).

    python tools/dslm.py fetch                  download the pinned model files
    python tools/dslm.py inspect [checkpoint]   header and DS memory budget
    python tools/dslm.py stage <dir> [--force]  write the /dslm card tree
    python tools/dslm.py golden [--force]       fp32 golden outputs (needs cc)
    python tools/dslm.py parity <tokens.txt>    compare a DS run with the golden
    python tools/dslm.py package [--force]      zip to unzip at the microSD root

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
checkpoint = /dslm/models/stories260K.bin
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


# ---- stage ---------------------------------------------------------------

def stage(target, checkpoint, tokenizer, force=False):
    """Write <target>/dslm/{dslm.ini, models/, manifest.txt}."""
    target = pathlib.Path(target)
    models = target / "dslm" / "models"
    models.mkdir(parents=True, exist_ok=True)
    staged = []
    for src in (pathlib.Path(checkpoint), pathlib.Path(tokenizer)):
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


def build_golden_binary(cc="cc"):
    pin = load_pins()["llama2c"]
    src_dir = CACHE / "llama2c" / pin["commit"]
    fetch_pinned(pin["run_c_url"], src_dir / "run.c", pin["run_c_sha256"])
    exe = CACHE / "golden"
    # No fast-math and no FMA contraction: plain IEEE fp32, the DS reference.
    cmd = [cc, "-O2", "-std=gnu11", "-ffp-contract=off", f"-I{src_dir}",
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


def golden(out_path=None, force=False, steps=GOLDEN_STEPS, model=DEFAULT_MODEL):
    out_path = pathlib.Path(out_path or GOLDEN_DIR / f"{model}.json")
    if out_path.exists() and not force:
        raise SystemExit(f"error: {out_path} exists; pass --force to overwrite it")
    checkpoint, tokenizer = fetch(model)
    exe = build_golden_binary()
    doc = {
        "model": model,
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
       dslm/models/stories260K.bin
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
        "  Author: Andrej Karpathy (karpathy/tinyllamas). License: MIT.\n\n"
        "llama2.c (the inference code dslm.nds is ported from)\n"
        f"  Source: https://github.com/karpathy/llama2.c (commit {pins['llama2c']['commit']})\n"
        "  License: MIT, text below. The same MIT terms cover the model files.\n\n"
        + license_text
    )


def package(out_path, nds_path, checkpoint, tokenizer, force=False):
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
        stage(card, checkpoint, tokenizer, force=True)
        files = {"dslm.nds": nds_path.read_bytes()}
        for p in sorted((card / "dslm").rglob("*")):
            if p.is_file() and p.name != "manifest.txt":
                files[p.relative_to(card).as_posix()] = p.read_bytes()

    manifest = "".join(
        f"{name} {len(data)} {hashlib.sha256(data).hexdigest()}\n" for name, data in sorted(files.items()))
    files["MANIFEST.txt"] = manifest.encode()
    files["README-DSLM.txt"] = PACKAGE_README.encode()
    files["THIRD_PARTY.txt"] = third_party_notice().encode()

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(out_path, "w") as zf:
        for name in sorted(files):
            _zip_entry(zf, name, files[name])
    return out_path, sorted(files)


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
    golden_path = pathlib.Path(golden_path or GOLDEN_DIR / f"{DEFAULT_MODEL}.json")
    ds = read_ds_tokens(tokens_path)
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
    p = sub.add_parser("inspect")
    p.add_argument("checkpoint", nargs="?")
    p.add_argument("tokenizer", nargs="?")
    p = sub.add_parser("stage")
    p.add_argument("target")
    p.add_argument("--force", action="store_true", help="also overwrite an existing dslm.ini")
    p = sub.add_parser("golden")
    p.add_argument("--force", action="store_true", help="overwrite an existing golden file")
    p.add_argument("--out")
    p = sub.add_parser("package")
    p.add_argument("--nds", default=str(ROOT / "dslm.nds"))
    p.add_argument("--out", default=str(ROOT / "build" / "dslm-package.zip"))
    p.add_argument("--force", action="store_true", help="overwrite an existing package")
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
    elif args.cmd == "stage":
        ckpt, tok = fetch()
        for line in stage(args.target, ckpt, tok, args.force):
            print(f"staged {line}")
    elif args.cmd == "golden":
        print(f"wrote {golden(args.out, args.force)}")
    elif args.cmd == "package":
        ckpt, tok = fetch()
        out, names = package(args.out, args.nds, ckpt, tok, args.force)
        print(f"wrote {out} ({out.stat().st_size} bytes)")
        for n in names:
            print(f"  {n}")
    elif args.cmd == "parity":
        return 0 if parity(args.tokens, args.golden) else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
