import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

TWO_RUNS = """version=1.0.0
build=fp32
emulated=unknown
tok_per_s=1.59
heap_peak_bytes=2000000

version=1.0.0
build=int8
emulated=unknown
load_ms=124
tok_per_s=6.55
heap_peak_bytes=1200000

"""


class Bench(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = pathlib.Path(self.tmp.name)
        self.bench = self.dir / "bench.txt"
        self.bench.write_text(TWO_RUNS)

    def tearDown(self):
        self.tmp.cleanup()

    def test_blocks_are_split_on_blank_lines(self):
        blocks = dslm.read_bench_blocks(self.bench)
        self.assertEqual([b["build"] for b in blocks], ["fp32", "int8"])

    def test_import_takes_the_last_run_and_adds_the_header(self):
        out = self.dir / "out.txt"
        path, run = dslm.import_bench(self.bench, "R4 clone", "YSMenu", "3.8 GB FAT32", "no", out)
        text = path.read_text()
        self.assertTrue(text.startswith("cart=R4 clone\nkernel=YSMenu\nmicrosd=3.8 GB FAT32\ncommit="))
        self.assertIn("build=int8\n", text)
        self.assertIn("emulated=no\n", text)
        self.assertNotIn("emulated=unknown", text)  # the import decides it
        self.assertEqual(run["heap_peak_bytes"], "1200000")

    def test_default_name_uses_date_cart_and_build(self):
        dslm.BENCH_DIR, saved = self.dir, dslm.BENCH_DIR
        try:
            path, _ = dslm.import_bench(self.bench, "R4 Clone!", "YSMenu", "x", "no", date="2026-11-25")
        finally:
            dslm.BENCH_DIR = saved
        self.assertEqual(path.name, "2026-11-25-r4-clone-int8.txt")

    def test_emulator_runs_need_an_explicit_destination(self):
        with self.assertRaises(SystemExit):
            dslm.import_bench(self.bench, "melonDS", "-", "-", "yes")

    def test_existing_file_is_not_overwritten(self):
        out = self.dir / "out.txt"
        dslm.import_bench(self.bench, "c", "k", "m", "no", out)
        with self.assertRaises(SystemExit):
            dslm.import_bench(self.bench, "c", "k", "m", "no", out)


if __name__ == "__main__":
    unittest.main()
