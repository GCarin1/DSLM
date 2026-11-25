import io
import json
import pathlib
import sys
import tempfile
import unittest
from contextlib import redirect_stdout

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402


class Parity(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = pathlib.Path(self.tmp.name)
        self.golden = self.dir / "g.json"
        self.golden.write_text(json.dumps({"runs": [{"prompt": "Hi", "tokens": [1, 5, 6, 7, 8]}]}))

    def tearDown(self):
        self.tmp.cleanup()

    def run_parity(self, tokens, prompt="Hi"):
        path = self.dir / "tokens.txt"
        path.write_text(f"prompt={prompt}\nsteps=4\ntokens={' '.join(map(str, tokens))}\ngen_ms=1\n")
        out = io.StringIO()
        with redirect_stdout(out):
            ok = dslm.parity(path, self.golden)
        return ok, out.getvalue()

    def test_identical_prefix_passes(self):
        ok, out = self.run_parity([1, 5, 6])
        self.assertTrue(ok)
        self.assertIn("3 tokens identical", out)

    def test_first_divergence_is_reported(self):
        ok, out = self.run_parity([1, 5, 9, 7])
        self.assertFalse(ok)
        self.assertIn("index 2: ds=9 golden=6", out)

    def test_longer_ds_run_compares_the_golden_length(self):
        ok, out = self.run_parity([1, 5, 6, 7, 8, 9])
        self.assertTrue(ok)
        self.assertIn("compared the prefix", out)

    def test_unknown_prompt_is_an_error(self):
        with self.assertRaises(SystemExit):
            self.run_parity([1], prompt="Other")


if __name__ == "__main__":
    unittest.main()
