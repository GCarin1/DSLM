# Manual test: load-stories260k

**Status:** melonDS half runnable since M3; the real R4 half is M4. Proves `model-loading` criterion 1.

## Setup

1. Stage the card with `pc-toolchain`: `/dslm/models/stories260K.bin` and `/dslm/models/tok512.bin`.
2. Build `dslm.nds` (`docs/building.md`).

## Steps and expected result

1. melonDS with DLDI and "Sync SD to folder" (`docs/testing.md`): the screen lists both files, shows load progress, then reports the model loaded with its byte counts.
2. Repeat on a real R4 (`docs/r4-flashcart.md`): same result.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M3 | both files found, progress shown, loaded in 321 ms (emulated) | yes |
