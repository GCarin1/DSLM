"""The DS engine (src/engine/llama2.c), built for the host, must reproduce the
golden outputs token for token. Needs a C compiler: run with `make tools-test`."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

GOLDEN = dslm.GOLDEN_DIR / "stories260K.json"


class EngineHost(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.exe = pathlib.Path(cls.tmp.name) / "engine_main"
        subprocess.run(
            ["cc", "-O2", "-std=c99", "-Wall", "-Wextra", "-Werror", "-ffp-contract=off",
             f"-I{dslm.ROOT / 'src' / 'engine'}", "-o", str(cls.exe),
             str(dslm.ROOT / "tests" / "host" / "engine_main.c"),
             str(dslm.ROOT / "src" / "engine" / "llama2.c"), "-lm"],
            check=True)
        cls.golden = json.loads(GOLDEN.read_text(encoding="utf-8"))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_matches_golden_for_every_prompt(self):
        ckpt, tok = dslm.model_paths()
        steps = self.golden["steps"]
        for run in self.golden["runs"]:
            got = dslm.run_golden(self.exe, ckpt, tok, run["prompt"], steps)
            self.assertEqual(got["prompt_tokens"], run["prompt_tokens"], run["prompt"])
            self.assertEqual(got["tokens"], run["tokens"], run["prompt"])
            self.assertEqual(got["text"], run["text"], run["prompt"])

    def test_int8_matches_runq_golden_for_every_prompt(self):
        q, tok = dslm.ensure_q80()
        golden = json.loads((dslm.GOLDEN_DIR / "stories260K_q80.json").read_text(encoding="utf-8"))
        for run in golden["runs"]:
            got = dslm.run_golden(self.exe, q, tok, run["prompt"], golden["steps"])
            self.assertEqual(got["tokens"], run["tokens"], run["prompt"])
            self.assertEqual(got["text"], run["text"], run["prompt"])

    def test_stop_keeps_the_tokens_produced_so_far(self):
        ckpt, tok = dslm.model_paths()
        run = self.golden["runs"][1]
        out = subprocess.run([str(self.exe), str(ckpt), str(tok), "64", run["prompt"], "10"],
                             check=True, capture_output=True, text=True).stdout
        tokens = [int(t) for t in out.splitlines()[1].split()[1:]]
        self.assertEqual(len(tokens), 11)  # BOS + the 10 tokens seen before stopping
        self.assertEqual(tokens, run["tokens"][:11])

    def test_rejects_a_prompt_longer_than_the_buffer(self):
        ckpt, tok = dslm.model_paths()
        r = subprocess.run([str(self.exe), str(ckpt), str(tok), "4", "a" * 300], capture_output=True)
        self.assertNotEqual(r.returncode, 0)


if __name__ == "__main__":
    unittest.main()
