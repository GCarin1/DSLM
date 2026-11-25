# Hardware runbook: one benchmark run on the reference R4

Proves `test-harness` criterion 3: a real R4 run produces a `benchmarks/` file with load time, tokens per second and peak heap. Reference hardware: ADR 0009.

## Steps

1. Build and install: `make package`, unzip `build/dslm-*.zip` at the root of the R4 microSD card (kernel already there).
2. On the DS: launch `dslm.nds` from the kernel menu, generate from `Once upon a time` (steps 128 or 256) and let it finish. Each run appends a block to `/dslm/bench.txt` (contract `model-file-format`).
3. Back on the PC, import the last run:

```bash
python tools/dslm.py bench E:/dslm/bench.txt --emulated no \
    --cart "R4 clone (R4.dat/TTMenu.dat, DSONEi SDHC / R4i-SDHC YSMenu package)" \
    --kernel "YSMenu" --microsd "3.8 GB FAT32 partition"
python tools/dslm.py parity E:/dslm/tokens.txt
```

`bench` writes `benchmarks/<date>-<cart>-<build>.txt` with the cart, kernel, microSD and commit header, and refuses emulator runs (`--emulated yes`) unless an explicit `--out` is given, so emulator numbers never land in `benchmarks/`.

## Results

| Date | Cart / kernel | Build | tok/s | load_ms | heap_peak_bytes | File |
|------|---------------|-------|-------|---------|-----------------|------|
| 2026-11-25 | R4 clone / YSMenu | int8 (1.0.0) | 6.87 | 124 | 1,372,492 | `benchmarks/2026-11-25-r4-ysmenu-v1.0.0-int8.txt` (parity 197/197) |
