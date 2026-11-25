# Manual test: load-time

**Status:** not runnable yet — needs model loading (M3) and the reference R4 (M4). Proves `model-loading` criterion 4.

## Steps and expected result

1. On a real R4, run `dslm.nds` with stories260K staged. The loader times the full read with a DS hardware timer and writes `load_ms` to `/dslm/bench.txt`.
2. Copy `bench.txt` to `benchmarks/<YYYY-MM-DD>-<cart>.txt` with the cart, kernel and microSD card in the header (`emulated=no`).

Emulator timings are never recorded here as hardware results.

## Results

| Date | Cart / kernel | microSD | load_ms |
|------|---------------|---------|---------|
