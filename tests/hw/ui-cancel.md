# Manual test: ui-cancel

**Status:** not runnable yet — needs the interface (M6). Proves `ds-interface` criterion 2.

## Steps and expected result

1. Start a generation with a high `steps` value in `/dslm/dslm.ini`.
2. Press B mid-generation. Generation stops within one token; the text produced so far stays on the top screen and the keyboard accepts a new prompt.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
