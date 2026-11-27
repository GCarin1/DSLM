# Manual test: ui-peek

**Status:** runnable on melonDS since change 0029. Proves `ds-interface` and `inference-engine`: the Generating screen can show the five most likely next tokens of a story model, the step hook never changes the generated tokens, and the visualizer's cost is known.

## Setup

Stage the card and build `dslm.nds`. A story model loaded (Stories mode). Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Stories, START. The footer reads "B: stop    X: peek inside the model".
2. X: STOP shrinks, the header says "next token odds", and five rows show each candidate in quotes with a bar and a percentage. The token actually picked is white and bright cyan. A newline shows as `/`, and the end of the story as "(the end)".
3. X again: the brain and the big STOP come back. The choice is saved as `[ui] peek` in `dslm.ini` at the end of the run, if it changed.
4. Chat mode: no X hint and no visualizer (the chat engine has no step hook).
5. Parity with `peek = on`: `python tests/parity/run_melonds.py --steps 256 --model int8` passes (the hook only reads the logits). Compare `gen_ms` with `peek = off`.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-27 | melonDS 1.1 | change 0029 | 1-3 pass (e.g. `" to"` 68%, `" n"` 8%, `" very"` 4%); 5: int8 parity 129/129 with peek on; 256 steps 36,418 ms with peek on vs 35,459 ms off (+2.7%, the opt-in cost of the top-5 and the drawing) | yes |
