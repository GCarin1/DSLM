"""The fixed-point LittleKedi engine (src/engine/mlq_fixed.arm.c), host build:
exact against its own golden, bounded drift against the fp32 engine, and a
repeat penalty that breaks loops. Needs a C compiler: run with `make tools-test`."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

# Teacher-forced on the fp32 golden sequences. Measured: 226/235 = 0.962 agreement,
# mean relative logit error 0.024 (llama.cpp's Q8_0 diverges from F16 on the same prompts).
MIN_AGREEMENT = 0.93
MAX_REL_ERROR = 0.04


class MlqHost(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        d = pathlib.Path(cls.tmp.name)
        cls.exe, cls.drift = d / "mlq_main", d / "mlq_drift"
        dslm.build_host("tests/host/mlq_main.c", cls.exe, dslm.MLQ_SOURCES)
        dslm.build_host("tests/host/mlq_drift.c", cls.drift, dslm.MLQ_SOURCES)
        cls.fp32, _ = dslm.ensure_littlekedi()
        cls.model, cls.tok = dslm.ensure_littlekedi_q()
        cls.ref = json.loads((dslm.GOLDEN_DIR / "littlekedi_ref.json").read_text(encoding="utf-8"))
        cls.golden = json.loads((dslm.GOLDEN_DIR / "littlekedi_q80.json").read_text(encoding="utf-8"))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_ds_default_runs_match_their_golden(self):
        for g in self.golden["runs"]:
            got = dslm.run_golden(self.exe, self.model, self.tok, g["prompt"], self.golden["steps"], dslm.MLQ_GREEDY)
            self.assertEqual(got["tokens"], g["tokens"], g["prompt"])

    def test_sampled_matches_its_golden(self):
        for g in self.golden["sampled"]:
            got = dslm.run_golden(self.exe, self.model, self.tok, g["prompt"], self.golden["sampled_steps"],
                                  dslm.MLQ_SAMPLED)
            self.assertEqual(got["tokens"], g["tokens"], g["prompt"])

    def test_the_first_chat_example_is_a_golden_prompt(self):
        main_c = (dslm.ROOT / "src" / "main.c").read_text(encoding="utf-8")
        self.assertIn('{"What causes the seasons?"', main_c)
        self.assertEqual(self.golden["runs"][0]["prompt"], "<|user|>What causes the seasons?<|end|><|assistant|>")

    def test_the_file_names_its_tokenizer(self):
        raw = self.model.read_bytes()[:128]
        self.assertEqual(raw[:4], b"MLQ1")
        self.assertEqual(raw[80:128].rstrip(bytes(1)), b"littlekedi.tok")

    def test_drift_against_fp32_is_bounded(self):
        agree = total = 0
        errors = []
        for r in self.ref["runs"]:
            out = subprocess.run([str(self.drift), str(self.fp32), str(self.model), *map(str, r["tokens"])],
                                 check=True, capture_output=True, text=True).stdout.split()
            agree += int(out[0])
            total += int(out[1])
            errors.append(float(out[2]))
        self.assertGreaterEqual(agree / total, MIN_AGREEMENT, f"{agree}/{total}")
        self.assertLessEqual(sum(errors) / len(errors), MAX_REL_ERROR, errors)

    def test_repeat_penalty_breaks_the_loop(self):
        prompt = "<|user|>Write a short story about a cat.<|end|><|assistant|>"
        plain = dslm.run_golden(self.exe, self.model, self.tok, prompt, 48)
        penalised = dslm.run_golden(self.exe, self.model, self.tok, prompt, 48, (0, 0, 1.15, 1))
        self.assertIn("cat's cat's cat's", plain["text"])
        self.assertNotIn("cat's cat's cat's", penalised["text"])


if __name__ == "__main__":
    unittest.main()
