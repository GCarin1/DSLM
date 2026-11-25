"""The LittleKedi C engine (src/engine/mla.c + bpe.c), built for the host, must
reproduce the numpy oracle's golden outputs token for token. Needs a C compiler:
run with `make tools-test` in Docker."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

GOLDEN = dslm.GOLDEN_DIR / "littlekedi_ref.json"


class MlaHost(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.exe = pathlib.Path(cls.tmp.name) / "mla_main"
        src = dslm.ROOT / "src" / "engine"
        subprocess.run(["cc", "-O2", "-std=c99", "-Wall", "-Wextra", "-Werror", "-ffp-contract=off", f"-I{src}",
                        "-o", str(cls.exe), str(dslm.ROOT / "tests" / "host" / "mla_main.c"),
                        str(src / "mla.c"), str(src / "bpe.c"), "-lm"], check=True)
        cls.model, cls.tok = dslm.ensure_littlekedi()
        cls.golden = json.loads(GOLDEN.read_text(encoding="utf-8"))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_matches_the_oracle_for_every_prompt(self):
        for run in self.golden["runs"]:
            got = dslm.run_golden(self.exe, self.model, self.tok, run["prompt"], self.golden["steps"])
            self.assertEqual(got["prompt_tokens"], run["prompt_tokens"], run["prompt"])
            self.assertEqual(got["tokens"], run["tokens"], run["prompt"])
            self.assertEqual(got["text"], run["text"], run["prompt"])

    def test_chat_markers_are_single_tokens(self):
        got = dslm.run_golden(self.exe, self.model, self.tok, "<|user|>hi<|end|><|assistant|>", 1)
        self.assertEqual(got["tokens"][0], 8193)
        self.assertIn(8195, got["tokens"])
        self.assertEqual(got["tokens"][got["prompt_tokens"] - 1], 8194)


if __name__ == "__main__":
    unittest.main()
