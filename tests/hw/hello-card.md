# Manual test: hello-card (M1)

Verifies that `dslm.nds` mounts the card and prints the settings from `/dslm/dslm.ini`.

## Setup

1. Build `dslm.nds` (`docs/building.md`).
2. Stage the card folder as in `docs/testing.md` (melonDS "Sync SD to folder"), with this `/dslm/dslm.ini`:

```ini
[model]
checkpoint = /dslm/models/stories260K.bin
tokenizer  = /dslm/models/tok512.bin

[sampling]
temperature = 0.5
topp        = 0.8
steps       = 64
seed        = 7
```

## Steps and expected result

1. Run `dslm.nds` in melonDS with DLDI enabled. The top screen shows `FAT mounted`, `read /dslm/dslm.ini`, and the values above (`temp x1000: 500`, `steps: 64`, `seed: 7`).
2. Delete `dslm.ini` and run again. The screen shows `file not found, using defaults` and `steps: 256`.
3. Run with DLDI disabled. The screen shows `FAT mount failed` with the dlditool hint, and nothing else runs.

Record the emulator and version used. Emulator results are labelled `emulated=yes`.

## Results

| Date | Emulator | Build | Step 1 | Step 2 | Step 3 | emulated |
|------|----------|-------|--------|--------|--------|----------|
| 2026-11-25 | melonDS 1.1 (Windows x86_64, portable) | BlocksDS `slim-latest` (digest in `docs/building.md`) | pass | pass | pass | yes |
| 2026-11-25 | melonDS 1.1 | M3 skeleton (settings print before loading) | pass | pass | pass | yes |

Not yet run on a real R4 (milestone M4).
