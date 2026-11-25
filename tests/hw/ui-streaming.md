# Manual test: ui-streaming

**Status:** not runnable yet — needs the interface (M6). Proves `ds-interface` criterion 1.

## Steps and expected result

1. Launch `dslm.nds` with stories260K staged. The bottom screen shows the on-screen keyboard.
2. Type `Once upon a time` and confirm. Text appears on the top screen one token at a time, not all at once at the end.
3. While it runs, the bottom screen shows that the model is working and the token count; the keyboard does not accept a new prompt.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
