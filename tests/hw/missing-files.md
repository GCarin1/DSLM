# Manual test: missing-files

**Status:** runnable on melonDS since M3. Proves `model-loading` criterion 3.

## Steps and expected result

1. Run with DLDI disabled (or no microSD card on hardware): `FAT mount failed` with the dlditool hint; nothing else runs.
2. Run with the card mounted but `/dslm/models/` empty: the screen names the missing file; inference does not start.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M3 | step 1: `FAT mount failed` + dlditool hint (see `hello-card.md`); step 2: `/dslm/models/nope.bin: file not found`, nothing runs | yes |
