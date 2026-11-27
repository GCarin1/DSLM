# Manual test: ui-paths

**Status:** runnable on melonDS since change 0031. Proves `ds-interface` and `inference-engine`: a finished story can go on with three alternative continuations to choose from, and continuing from the KV cache stays coherent whichever path is chosen.

## Setup

Stage the card, set `steps = 64` in `dslm.ini` (to leave room in the context), and build `dslm.nds`. A story model loaded. Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Stories, START. When the story is done, the Story screen has a fourth button: "What happens next? (X)". There is no such button in chat mode or when the context has no room for 24 more tokens.
2. X: "Writing path 1 of 3..." up to 3 of 3 (B cancels). Then three cards with the continuations. A path that goes on inside the last word starts with "...".
3. Down, A (the second card, so not the last one written): its text is appended on the top screen, and three new paths follow that continue it coherently (the cache was fed again).
4. Touch a card: the same, by touch.
5. B (Finish): back to the Story screen with the longer story. Its speed figure is that of the original generation, and a new history entry holds the extended story.
6. Parity is unchanged (`python tests/parity/run_melonds.py --steps 256 --model int8`).

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-27 | melonDS 1.1 | change 0031 | 1-5 pass: picked the 2nd path ("...'s mom asked her to put the ball in her bed. The ball was stu"), and the next paths went on coherently ("ck in the blocks..."); a touch picked the 3rd path in a second run; 65 → 89 tokens with the speed shown as 8.7 tokens/s; 6: int8 parity 129/129 (35,553 ms) | yes |
