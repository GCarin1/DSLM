# Manual test: load-time

**Status:** run on the reference R4 (M4). Proves `model-loading` criterion 4.

## Steps and expected result

1. On a real R4, run `dslm.nds` with stories260K staged. The loader times the full read with a DS hardware timer and writes `load_ms` to `/dslm/bench.txt`.
2. Copy `bench.txt` to `benchmarks/<YYYY-MM-DD>-<cart>.txt` with the cart, kernel and microSD card in the header (`emulated=no`).

Emulator timings are never recorded here as hardware results.

## Results

| Date | Cart / kernel | microSD | load_ms |
|------|---------------|---------|---------|
| 2026-11-25 | R4 clone / YSMenu | 3.8 GB FAT32 partition | 249 (`benchmarks/2026-11-25-r4-ysmenu.txt`) |
