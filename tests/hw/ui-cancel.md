# Manual test: ui-cancel

**Status:** runnable on melonDS since M6. Proves `ds-interface` criterion 2.

## Steps and expected result

1. Start a generation with a high `steps` value in `/dslm/dslm.ini`.
2. Hold B (or touch STOP) mid-generation. Generation stops within one token; the text produced so far stays on the top screen, the Story screen says "Stopped", and A returns to the prompt for a new one.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M6 | hold B: "Stopped: 11 tokens", text kept, `tokens.txt` has `stopped=1` and a golden prefix; touch STOP: same | yes |
