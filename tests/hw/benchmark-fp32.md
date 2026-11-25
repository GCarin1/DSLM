# Manual test: benchmark-fp32

**Status:** not runnable yet — needs the reference R4 and `/dslm/bench.txt` (M4). Proves `inference-engine` criterion 3.

## Steps and expected result

1. Run the fp32 build on the R4 with `steps = 128` and the default prompt.
2. Record `gen_ms` and the per-position time from the bottom screen and `tokens.txt` (later `bench.txt`), measured with the DS hardware timers, into `benchmarks/<YYYY-MM-DD>-<cart>.txt` with `build=fp32` and `emulated=no`.

This is the baseline the fixed-point build is compared against. melonDS numbers are never recorded here.

## Results

| Date | Cart / kernel | Steps | gen_ms | ms per position |
|------|---------------|-------|--------|-----------------|
