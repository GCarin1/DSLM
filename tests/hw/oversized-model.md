# Manual test: oversized-model

**Status:** runnable on melonDS since M3. Proves `model-loading` criterion 2.

## Setup

1. Point `checkpoint` in `/dslm/dslm.ini` at a model larger than the DS heap (stories15M, about 60 MB in fp32).

## Steps and expected result

1. Run `dslm.nds` in melonDS. The loader reads only the header, prints the required and available byte counts, and stops. No allocation is attempted and inference does not start.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M3 | refused before allocating: needs 16353673 bytes, 4038656 available (synthetic 14.2 MB checkpoint, dim 256) | yes |
