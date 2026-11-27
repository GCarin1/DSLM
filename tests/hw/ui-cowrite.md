# Manual test: ui-cowrite

**Status:** runnable on melonDS since change 0030. Proves `ds-interface`: during a story generation the user can take over the choice of the next tokens among the model's five most likely, give the pen back, or stop.

## Setup

Stage the card and build `dslm.nds`. A story model loaded. Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Stories, START. The footer reads "X: peek   Y: write together   B: stop".
2. Y: at the next step the header says "Your turn: pick the next token", and five rows offer the candidates with their odds, the first highlighted.
3. Down twice, A: the third candidate is appended to the text, and the next five are offered.
4. Touch a row: that candidate is used.
5. Y: the model writes again by itself (the brain comes back). B: it stops, and the Story screen keeps the co-written text.
6. The Story screen's tokens/s leaves out the time spent choosing.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-27 | melonDS 1.1 | change 0030 | 1-6 pass: "She lo" + picked "st" (third) → "She lost", touch picked " her", Y gave the pen back ("…toys and her mommy…"), B stopped at 49 tokens; 8.9 tokens/s reported (waits excluded) | yes |
