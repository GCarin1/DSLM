# Manual test: benchmark-int8

**Status:** runnable on the reference R4 since M5a (int8 matmul); the fully fixed-point build is M5b. Proves `fixed-point-inference` criterion 1.

## Steps and expected result

1. Install the package (`make package`, unzip at the microSD root). The default `dslm.ini` uses `stories260K_q80.bin`.
2. Generate from `Once upon a time` with `steps = 128` and let it finish.
3. Copy `/dslm/tokens.txt` back; record `load_ms`, `gen_ms` and ms per position in `benchmarks/<YYYY-MM-DD>-<cart>.txt` with `build=int8`, `emulated=no`, and compare with the fp32 baseline (627.5 ms per position, `benchmarks/2026-11-25-r4-ysmenu.txt`).

## Results

| Date | Cart / kernel | Build | Steps | gen_ms | ms per position | vs fp32 |
|------|---------------|-------|-------|--------|-----------------|---------|
