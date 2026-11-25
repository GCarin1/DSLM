import pathlib
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

# Names that would mean a kernel or DLDI driver slipped in (ADR 0005).
FORBIDDEN = ("__rpg", "_DS_MENU.DAT", "_dsmenu", ".dldi", "R4.dat", "TTMenu")


class Package(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = pathlib.Path(self.tmp.name)
        self.nds = self.dir / "dslm.nds"
        self.nds.write_bytes(b"NDS")
        self.ckpt = self.dir / "stories260K.bin"
        self.ckpt.write_bytes(b"model")
        self.tok = self.dir / "tok512.bin"
        self.tok.write_bytes(b"tok")
        self.out = self.dir / "out" / "pkg.zip"

    def tearDown(self):
        self.tmp.cleanup()

    def build(self, out=None, force=False):
        return dslm.package(out or self.out, self.nds, self.ckpt, self.tok, force=force)

    def test_layout_unzips_at_the_card_root(self):
        _, names = self.build()
        self.assertEqual(names, sorted([
            "MANIFEST.txt", "README-DSLM.txt", "THIRD_PARTY.txt", "dslm.nds",
            "dslm/dslm.ini", "dslm/models/stories260K.bin", "dslm/models/tok512.bin"]))

    def test_model_files_are_copied_unchanged(self):
        self.build()
        with zipfile.ZipFile(self.out) as zf:
            self.assertEqual(zf.read("dslm/models/stories260K.bin"), b"model")
            self.assertEqual(zf.read("dslm.nds"), b"NDS")
            self.assertIn("checkpoint = /dslm/models/stories260K.bin", zf.read("dslm/dslm.ini").decode())

    def test_manifest_lists_sizes_and_hashes(self):
        self.build()
        with zipfile.ZipFile(self.out) as zf:
            lines = zf.read("MANIFEST.txt").decode().splitlines()
        rec = {l.split()[0]: l.split()[1:] for l in lines}
        self.assertEqual(rec["dslm/models/stories260K.bin"][0], "5")
        self.assertEqual(rec["dslm.nds"][1], dslm.hashlib.sha256(b"NDS").hexdigest())

    def test_licenses_and_readme_are_included(self):
        self.build()
        with zipfile.ZipFile(self.out) as zf:
            notice = zf.read("THIRD_PARTY.txt").decode()
            readme = zf.read("README-DSLM.txt").decode()
        self.assertIn("MIT License", notice)
        self.assertIn("karpathy/tinyllamas", notice)
        self.assertIn("ROOT of the flashcart's microSD card", readme)

    def test_no_kernel_or_dldi_files(self):
        _, names = self.build()
        for n in names:
            for bad in FORBIDDEN:
                self.assertNotIn(bad.lower(), n.lower())

    def test_builds_are_byte_identical(self):
        self.build()
        other = self.dir / "other.zip"
        self.build(out=other)
        self.assertEqual(self.out.read_bytes(), other.read_bytes())

    def test_existing_package_is_not_overwritten(self):
        self.build()
        with self.assertRaises(SystemExit):
            self.build()
        self.build(force=True)

    def test_missing_nds_is_an_error(self):
        self.nds.unlink()
        with self.assertRaises(SystemExit):
            self.build()


if __name__ == "__main__":
    unittest.main()
