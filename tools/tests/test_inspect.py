import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402


def write_checkpoint(path, dim=8, hidden=16, layers=2, heads=2, kv_heads=1, vocab=10, seq_len=4, extra=0):
    h = dict(dim=dim, hidden_dim=hidden, n_layers=layers, n_heads=heads, n_kv_heads=kv_heads,
             vocab_size=abs(vocab), shared_classifier=vocab > 0, seq_len=seq_len)
    n = dslm.weight_floats(h) + extra
    with open(path, "wb") as f:
        f.write(struct.pack("<7i", dim, hidden, layers, heads, kv_heads, vocab, seq_len))
        f.write(b"\0" * 4 * n)


def write_tokenizer(path, vocab=10):
    with open(path, "wb") as f:
        f.write(struct.pack("<i", 3))
        for i in range(vocab):
            piece = f"t{i}".encode()
            f.write(struct.pack("<fi", float(i), len(piece)) + piece)


class InspectSynthetic(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = pathlib.Path(self.tmp.name)
        self.tok = self.dir / "tok.bin"
        write_tokenizer(self.tok)

    def tearDown(self):
        self.tmp.cleanup()

    def test_header_fields(self):
        ckpt = self.dir / "m.bin"
        write_checkpoint(ckpt)
        h = dslm.read_header(ckpt)
        self.assertEqual((h["dim"], h["n_layers"], h["vocab_size"]), (8, 2, 10))
        self.assertTrue(h["shared_classifier"])

    def test_negative_vocab_means_separate_classifier(self):
        ckpt = self.dir / "m.bin"
        write_checkpoint(ckpt, vocab=-10)
        h, b = dslm.memory_budget(ckpt, self.tok)
        self.assertEqual(h["vocab_size"], 10)
        self.assertFalse(h["shared_classifier"])

    def test_budget_parts(self):
        ckpt = self.dir / "m.bin"
        write_checkpoint(ckpt)
        _, b = dslm.memory_budget(ckpt, self.tok)
        # kv_dim = 8 * 1 / 2 = 4; 2 caches * 2 layers * 4 positions * 4 floats
        self.assertEqual(b["kv_cache"], 2 * 2 * 4 * 4 * 4)
        self.assertEqual(b["run_state"], 4 * (4 * 8 + 2 * 16 + 2 * 4 + 10))
        self.assertEqual(b["total"], b["weights"] + b["kv_cache"] + b["run_state"] + b["tokenizer"])

    def test_size_mismatch_is_rejected(self):
        ckpt = self.dir / "m.bin"
        write_checkpoint(ckpt, extra=1)
        with self.assertRaises(ValueError):
            dslm.memory_budget(ckpt, self.tok)

    def test_truncated_header_is_rejected(self):
        ckpt = self.dir / "m.bin"
        ckpt.write_bytes(b"\0" * 10)
        with self.assertRaises(ValueError):
            dslm.read_header(ckpt)


class InspectStories260K(unittest.TestCase):
    """Needs `python tools/dslm.py fetch` (make tools-test runs it first)."""

    def test_header_and_budget_below_4mb(self):
        ckpt, tok = dslm.model_paths()
        if not ckpt.exists():
            self.fail(f"{ckpt} missing: run `python tools/dslm.py fetch`")
        h, b = dslm.memory_budget(ckpt, tok)
        self.assertEqual(
            (h["dim"], h["hidden_dim"], h["n_layers"], h["n_heads"], h["n_kv_heads"], h["vocab_size"], h["seq_len"]),
            (64, 172, 5, 8, 4, 512, 512))
        self.assertEqual(b["weights"], 1056512)
        self.assertLess(b["total"], dslm.DS_RAM_BYTES)


if __name__ == "__main__":
    unittest.main()
