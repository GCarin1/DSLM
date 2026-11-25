# Manual test: load-stories260k

**Status:** not runnable yet — needs model loading (M3). Proves `model-loading` criterion 1.

## Setup

1. Stage the card with `pc-toolchain`: `/dslm/models/stories260K.bin` and `/dslm/models/tok512.bin`.
2. Build `dslm.nds` (`docs/building.md`).

## Steps and expected result

1. melonDS with DLDI and "Sync SD to folder" (`docs/testing.md`): the screen lists both files, shows load progress, then reports the model loaded with its byte counts.
2. Repeat on a real R4 (`docs/r4-flashcart.md`): same result.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
