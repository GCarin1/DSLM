# Manual test: benchmark-fp32

**Status:** run on the reference R4 (M4). Proves `inference-engine` criterion 3.

## Steps and expected result

1. Run the fp32 build on the R4 with `steps = 128` and the default prompt.
2. Record `gen_ms` and the per-position time from the bottom screen and `tokens.txt` (later `bench.txt`), measured with the DS hardware timers, into `benchmarks/<YYYY-MM-DD>-<cart>.txt` with `build=fp32` and `emulated=no`.

This is the baseline the fixed-point build is compared against. melonDS numbers are never recorded here.

## Results

| Date | Cart / kernel | Steps | gen_ms | ms per position |
|------|---------------|-------|--------|-----------------|
| 2026-11-25 | R4 clone / YSMenu | 256 (stopped at 152 positions) | 95380 | 627.5 (`benchmarks/2026-11-25-r4-ysmenu.txt`) |
