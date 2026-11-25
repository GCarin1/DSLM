"""Needs a C compiler and the fetched model: run with `make tools-test` in Docker."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

STEPS = 32


class Golden(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.dir = pathlib.Path(cls.tmp.name)
        cls.first = dslm.golden(cls.dir / "a.json", steps=STEPS)
        cls.second = dslm.golden(cls.dir / "b.json", steps=STEPS)
        cls.doc = json.loads(cls.first.read_text())

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_two_builds_are_identical(self):
        self.assertEqual(self.first.read_bytes(), self.second.read_bytes())

    def test_pins_are_recorded(self):
        pins = dslm.load_pins()
        self.assertEqual(self.doc["llama2c_commit"], pins["llama2c"]["commit"])
        self.assertEqual(self.doc["checkpoint_sha256"], pins["models"]["stories260K"]["checkpoint"]["sha256"])
        self.assertEqual(self.doc["temperature"], 0.0)

    def test_every_prompt_has_a_run(self):
        self.assertEqual([r["prompt"] for r in self.doc["runs"]], dslm.read_prompts())

    def test_runs_start_with_bos_and_generate(self):
        for run in self.doc["runs"]:
            self.assertEqual(run["tokens"][0], 1)
            self.assertGreater(len(run["tokens"]), run["prompt_tokens"])
            self.assertLessEqual(len(run["tokens"]), STEPS + 1)
            self.assertTrue(run["text"])

    def test_text_matches_unmodified_run_c(self):
        """golden.c mirrors run.c's generate(); the stock binary must print the same text."""
        pin = dslm.load_pins()["llama2c"]
        src = dslm.CACHE / "llama2c" / pin["commit"] / "run.c"
        exe = self.dir / "run"
        subprocess.run(["cc", "-O2", "-std=gnu11", "-ffp-contract=off", "-o", str(exe), str(src), "-lm"],
                       check=True)
        ckpt, tok = dslm.model_paths()
        for run in self.doc["runs"]:
            out = subprocess.run([str(exe), str(ckpt), "-z", str(tok), "-t", "0", "-n", str(STEPS),
                                  "-i", run["prompt"]], check=True, capture_output=True).stdout
            self.assertEqual(out.decode("utf-8", errors="replace"), run["text"] + "\n", run["prompt"])

    def test_existing_golden_is_not_overwritten(self):
        with self.assertRaises(SystemExit):
            dslm.golden(self.first, steps=STEPS)


if __name__ == "__main__":
    unittest.main()
