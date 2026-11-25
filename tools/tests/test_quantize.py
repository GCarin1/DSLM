"""int8 quantizer (llama2.c v2, "ak42"). The runq.c tests need a C compiler and
the fetched model: run with `make tools-test` in Docker."""
import json
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest
from array import array

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402
from test_inspect import write_checkpoint  # noqa: E402


class QuantizeSynthetic(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = pathlib.Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_group_size_divides_dim_and_hidden(self):
        self.assertEqual(dslm.pick_group_size({"dim": 64, "hidden_dim": 172}), 4)
        self.assertEqual(dslm.pick_group_size({"dim": 288, "hidden_dim": 768}), 32)
        self.assertEqual(dslm.pick_group_size({"dim": 64, "hidden_dim": 128}), 64)

    def test_header_and_size_match_runq_layout(self):
        src, dst = self.dir / "m.bin", self.dir / "q.bin"
        write_checkpoint(src, dim=8, hidden=12, layers=2, heads=2, kv_heads=1, vocab=10, seq_len=4)
        gs = dslm.quantize_checkpoint(src, dst)
        self.assertEqual(gs, 4)
        h = dslm.read_q80_header(dst)
        self.assertEqual((h["dim"], h["hidden_dim"], h["vocab_size"], h["group_size"]), (8, 12, 10, 4))
        self.assertTrue(h["shared_classifier"])
        self.assertEqual(dst.stat().st_size, dslm.q80_bytes(h, gs))

    def test_values_round_trip_within_half_a_step(self):
        # One tensor through the Q8_0 rule: |w - q*s| <= s/2 and |q| <= 127.
        vals = array("f", [0.5, -1.25, 3.0, 0.0, 1e-3, -2e-3, 7.5, -7.5])
        raw = dslm._quantize_tensor(vals, 4)
        q = struct.unpack("8b", raw[:8])
        s = struct.unpack("2f", raw[8:])
        for i, v in enumerate(vals):
            scale = s[i // 4]
            self.assertLessEqual(abs(q[i]), 127)
            self.assertLessEqual(abs(v - q[i] * scale), scale / 2 + 1e-9)

    def test_all_zero_group_stays_zero(self):
        raw = dslm._quantize_tensor(array("f", [0.0] * 4), 4)
        self.assertEqual(raw[:4], b"\0\0\0\0")
        self.assertEqual(struct.unpack("f", raw[4:]), (0.0,))

    def test_quantizing_twice_is_identical(self):
        src, a, b = self.dir / "m.bin", self.dir / "a.bin", self.dir / "b.bin"
        write_checkpoint(src, dim=8, hidden=12, layers=1, heads=2, kv_heads=2, vocab=-6, seq_len=4)
        dslm.quantize_checkpoint(src, a)
        dslm.quantize_checkpoint(src, b)
        self.assertEqual(a.read_bytes(), b.read_bytes())
        self.assertFalse(dslm.read_q80_header(a)["shared_classifier"])

    def test_group_size_that_does_not_divide_is_rejected(self):
        src = self.dir / "m.bin"
        write_checkpoint(src, dim=8, hidden=12)
        with self.assertRaises(ValueError):
            dslm.quantize_checkpoint(src, self.dir / "q.bin", group_size=8)


class QuantizeStories260K(unittest.TestCase):
    """The reference runq.c reads the int8 checkpoint the quantizer generates."""

    @classmethod
    def setUpClass(cls):
        cls.q, cls.tok = dslm.ensure_q80()
        cls.tmp = tempfile.TemporaryDirectory()
        cls.dir = pathlib.Path(cls.tmp.name)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_header_and_size(self):
        h = dslm.read_q80_header(self.q)
        self.assertEqual(h["group_size"], 4)
        self.assertEqual(self.q.stat().st_size, dslm.q80_bytes(h, 4))

    def test_runq_reads_it_and_generates(self):
        exe = dslm.build_golden_binary(quantized=True)
        run = dslm.run_golden(exe, self.q, self.tok, "Once upon a time", 32)
        self.assertEqual(run["tokens"][0], 1)
        self.assertEqual(len(run["tokens"]), 33)
        self.assertTrue(run["text"].startswith("Once upon a time"))

    def test_q80_golden_builds_twice_identically(self):
        a = dslm.golden(self.dir / "a.json", steps=16, kind="q80-runq")
        b = dslm.golden(self.dir / "b.json", steps=16, kind="q80-runq")
        self.assertEqual(a.read_bytes(), b.read_bytes())
        self.assertEqual(json.loads(a.read_text())["reference"], "runq.c")


if __name__ == "__main__":
    unittest.main()
