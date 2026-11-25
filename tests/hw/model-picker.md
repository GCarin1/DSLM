# Manual test: model-picker

Proves the model picker on the Settings screen (`ds-interface`, `model-loading`).

## Setup

Put more than one checkpoint in `/dslm/models/` (for example `stories260K_q80.bin`, `stories260K.bin` and `tiny1m_q80.bin`, all sharing `tok512.bin`). For the failure path, add a valid checkpoint that does not fit in RAM (the fp32 `tiny1m` `model.bin`, 3.8 MB, renamed `tiny1m_fp32.bin`).

## Steps and expected result

1. SELECT opens Settings; Down three times reaches `Model`. Left/Right (or touching `[<]` / `[>]`) cycles through the checkpoints the loader accepts; the tokenizer and other files are not listed. The note line shows the file and its format.
2. START: the model is unloaded, the new one loads with the progress bar, the title bar shows its name, the note says `Loaded and saved`, and `/dslm/dslm.ini` holds the new `checkpoint`.
3. B, then START: the story comes from the new model; `tokens.txt` records it and matches the host build of the engine.
4. Pick the model that does not fit and press START: the top screen explains why (`Model does not fit: needs ... available`), the note says `Not loaded, kept previous model`, the previous model is loaded again, `dslm.ini` is unchanged, and generation still works.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-25 | melonDS 1.1 | 1.0.0 + picker | steps 1-3: switched to `tiny1m_q80`, saved, 129/129 tokens vs the host build | yes |
| 2026-11-25 | melonDS 1.1 | 1.0.0 + picker | step 4: `tiny1m_fp32` refused (needs 4,903,053 bytes, 3,992,896 free after unloading), previous model reloaded, ini unchanged, next story generated | yes |
