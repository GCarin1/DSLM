#!/usr/bin/env python3
"""Write tests/golden/littlekedi_ref.json from the numpy oracle (needs numpy).

    python3 tools/m8/make_golden.py [--force]

The oracle (littlekedi_ref.py) matches llama.cpp greedy output with the repeat
penalty off; the C engine is compared against this file by the host tests.
"""
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "m8"))
import dslm  # noqa: E402
import littlekedi_ref as ref  # noqa: E402

STEPS = 48
PROMPTS = [
    "<|user|>What causes the seasons?<|end|><|assistant|>",
    "<|user|>Hello! Who are you?<|end|><|assistant|>",
    "<|user|>Write a short story about a cat.<|end|><|assistant|>",
    "<|system|>You are a helpful assistant.<|end|><|user|>Give me three tips to sleep better.<|end|><|assistant|>",
    "Once upon a time, there was a cat named",
]


def main():
    out = dslm.GOLDEN_DIR / "littlekedi_ref.json"
    if out.exists() and "--force" not in sys.argv:
        raise SystemExit(f"error: {out} exists; pass --force")
    dslm.ensure_littlekedi()
    hf = dslm.CACHE / "models" / "littlekedi-hf"
    entry = dslm.load_pins()["chat"]["littlekedi"]
    runs = []
    for p in PROMPTS:
        ids, tokens, text = ref.generate(str(hf), p, STEPS)
        runs.append({"prompt": p, "prompt_tokens": len(ids), "tokens": tokens, "text": text})
    doc = {"model": "littlekedi", "reference": "tools/m8/littlekedi_ref.py (numpy fp32; equals llama.cpp greedy, "
           "repeat penalty off)", "model_sha256": entry["files"]["model.safetensors"]["sha256"],
           "steps": STEPS, "runs": runs}
    out.write_text(json.dumps(doc, indent=1) + "\n", encoding="utf-8", newline="\n")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
