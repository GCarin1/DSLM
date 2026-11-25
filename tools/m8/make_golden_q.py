#!/usr/bin/env python3
"""Write tests/golden/littlekedi_q80.json: the fixed-point engine's own output
(host build), greedy and sampled, for the prompts of littlekedi_ref.json. The DS
build must reproduce it bit for bit. Needs a C compiler (run under Docker).

    python3 tools/m8/make_golden_q.py [--force]
"""
import json
import pathlib
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import dslm  # noqa: E402


def main():
    out = dslm.GOLDEN_DIR / "littlekedi_q80.json"
    if out.exists() and "--force" not in sys.argv:
        raise SystemExit(f"error: {out} exists; pass --force")
    ref = json.loads((dslm.GOLDEN_DIR / "littlekedi_ref.json").read_text(encoding="utf-8"))
    model, tok = dslm.ensure_littlekedi_q()
    with tempfile.TemporaryDirectory() as tmp:
        exe = pathlib.Path(tmp) / "mlq_main"
        dslm.build_host("tests/host/mlq_main.c", exe, dslm.MLQ_SOURCES)
        greedy = [dslm.run_golden(exe, model, tok, r["prompt"], ref["steps"]) for r in ref["runs"]]
        sampled = [dslm.run_golden(exe, model, tok, r["prompt"], ref["steps"], dslm.MLQ_SAMPLED) for r in ref["runs"]]
    doc = {"model": "littlekedi (MLQ1 int8)", "reference": "dslm fixed-point engine (host build)",
           "model_sha256": ref["model_sha256"], "steps": ref["steps"],
           "sampled_with": dict(zip(["temperature", "topp", "repeat_penalty", "seed"], dslm.MLQ_SAMPLED)),
           "greedy": greedy, "sampled": sampled}
    out.write_text(json.dumps(doc, indent=1) + "\n", encoding="utf-8", newline="\n")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
