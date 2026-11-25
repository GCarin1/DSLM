# Manual test: missing-files

**Status:** not runnable yet — needs model loading (M3); the FAT-failure half is covered today by `hello-card.md` step 3. Proves `model-loading` criterion 3.

## Steps and expected result

1. Run with DLDI disabled (or no microSD card on hardware): `FAT mount failed` with the dlditool hint; nothing else runs.
2. Run with the card mounted but `/dslm/models/` empty: the screen names the missing file; inference does not start.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
