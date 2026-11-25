#!/usr/bin/env python3
"""Write tests/golden/littlekedi_q80.json: the fixed-point engine's own output
(host build) for the prompts of littlekedi_ref.json. "runs" use the DS defaults
(temperature 0, repeat penalty 1.15) and are what `dslm.py parity` compares a DS
run with; "sampled" checks the sampler. Needs a C compiler (run under Docker).

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
        runs = [dslm.run_golden(exe, model, tok, r["prompt"], dslm.MLQ_GREEDY_STEPS, dslm.MLQ_GREEDY)
                for r in ref["runs"]]
        sampled = [dslm.run_golden(exe, model, tok, r["prompt"], ref["steps"], dslm.MLQ_SAMPLED) for r in ref["runs"]]
    keys = ["temperature", "topp", "repeat_penalty", "seed"]
    doc = {"model": "littlekedi (MLQ1 int8)", "reference": "dslm fixed-point engine (host build)",
           "model_sha256": ref["model_sha256"], "steps": dslm.MLQ_GREEDY_STEPS,
           "runs_with": dict(zip(keys, dslm.MLQ_GREEDY)), "runs": runs,
           "sampled_steps": ref["steps"], "sampled_with": dict(zip(keys, dslm.MLQ_SAMPLED)), "sampled": sampled}
    out.write_text(json.dumps(doc, indent=1) + "\n", encoding="utf-8", newline="\n")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
