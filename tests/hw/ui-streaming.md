# Manual test: ui-streaming

**Status:** runnable on melonDS since M6. Proves `ds-interface` criterion 1.

## Steps and expected result

1. Launch `dslm.nds` with stories260K staged. The bottom screen shows the on-screen keyboard.
2. Type `Once upon a time` and confirm. Text appears on the top screen one token at a time, not all at once at the end.
3. While it runs, the bottom screen shows that the model is working and the token count; the keyboard does not accept a new prompt.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M6 | prompt `Once upon a time` (default example; typing checked in `ui-navigation.md`): mid-run capture shows `11/32` and the partial text; the keyboard is hidden while generating | yes |
