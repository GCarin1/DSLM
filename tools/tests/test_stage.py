import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402


class Stage(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = pathlib.Path(self.tmp.name)
        self.ckpt = self.dir / "src" / "stories260K.bin"
        self.tok = self.dir / "src" / "tok512.bin"
        self.ckpt.parent.mkdir()
        self.ckpt.write_bytes(b"model")
        self.tok.write_bytes(b"tok")
        self.card = self.dir / "card"

    def tearDown(self):
        self.tmp.cleanup()

    def test_tree_matches_contract(self):
        dslm.stage(self.card, self.ckpt, self.tok)
        for rel in ("dslm/dslm.ini", "dslm/manifest.txt",
                    "dslm/models/stories260K.bin", "dslm/models/tok512.bin"):
            self.assertTrue((self.card / rel).is_file(), rel)

    def test_default_ini_points_at_staged_files(self):
        dslm.stage(self.card, self.ckpt, self.tok)
        ini = (self.card / "dslm" / "dslm.ini").read_text()
        self.assertIn("checkpoint = /dslm/models/stories260K_q80.bin", ini)
        self.assertIn("tokenizer  = /dslm/models/tok512.bin", ini)
        self.assertIn("temperature = 0.0", ini)

    def test_manifest_records_size_and_sha256(self):
        dslm.stage(self.card, self.ckpt, self.tok)
        lines = (self.card / "dslm" / "manifest.txt").read_text().splitlines()
        path, size, sha = lines[0].split()
        self.assertEqual(path, "/dslm/models/stories260K.bin")
        self.assertEqual(int(size), 5)
        self.assertEqual(sha, dslm.sha256_file(self.ckpt))

    def test_extra_checkpoints_are_staged(self):
        fp32 = self.dir / "src" / "other.bin"
        fp32.write_bytes(b"fp32")
        lines = dslm.stage(self.card, self.ckpt, self.tok, extra=[fp32])
        self.assertTrue((self.card / "dslm" / "models" / "other.bin").is_file())
        self.assertEqual(len(lines), 3)

    def test_existing_ini_is_kept_unless_forced(self):
        ini = self.card / "dslm" / "dslm.ini"
        ini.parent.mkdir(parents=True)
        ini.write_text("[sampling]\nsteps = 9\n")
        dslm.stage(self.card, self.ckpt, self.tok)
        self.assertIn("steps = 9", ini.read_text())
        dslm.stage(self.card, self.ckpt, self.tok, force=True)
        self.assertIn("steps       = 256", ini.read_text())


if __name__ == "__main__":
    unittest.main()
