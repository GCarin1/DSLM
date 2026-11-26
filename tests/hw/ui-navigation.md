# Manual test: ui-navigation

**Status:** runnable on melonDS since M6. Proves `ds-interface`: every action is reachable by a button and by touch (control map in `docs/user-guide.md`).

## Setup

Stage the card (`python tools/dslm.py stage .tools/card`), build `dslm.nds`. In melonDS, map keys for the DS buttons (Config > Input and hotkeys); the automated run used X=A, Z=B, A=Y, S=X, Q=L, W=R, Enter=START, Backspace=SELECT and the arrow keys.

## Steps and expected result

1. Boot. Prompt screen: welcome text on top; `Once upon a time` with the cursor, the `<` `>` `SET` `GO` buttons and the keyboard below.
2. R, then L: the prompt cycles through the example prompts. Touch `<` / `>`: same.
3. Touch DEL twice, then a letter: two characters are deleted and the letter is typed. D-pad Down then A: the key below the highlight is typed.
4. SELECT: Settings. Down twice, Left six times: Steps goes from 128 to 32. START: "Saved to /dslm/dslm.ini", and the file on the card holds `steps = 32`. B: back to the prompt.
5. START: Generating screen with the progress bar and STOP box; then the Story screen with "Done".
6. Y: generates again; press B: the Story screen says "Stopped" and keeps the text so far.
7. A: back to the prompt with the same prompt and the last story on top. Touch GO: generates. Touch STOP: stops.
8. With a story longer than 14 lines (steps 512), Up/Down scroll the top screen.

## Results

| Date | Target | Build | Steps passed | emulated |
|------|--------|-------|--------------|----------|
| 2026-11-25 | melonDS 1.1 | M6 | 1-7 (input posted to the melonDS window; captures checked) | yes |
| 2026-11-25 | melonDS 1.1 (fast-forward) | M6 | 8: steps 512 stopped at 289 tokens (23 lines); the view follows the end while streaming, Up reaches the first line, Down goes back | yes |
| 2026-11-26 | melonDS 1.1 | change 0022 (own keyboard) | 2-3 and 6 rerun: examples, DEL and letters by touch, D-pad + A typing, B tap stops (see `ui-keyboard.md`) | yes |
| 2026-11-26 | melonDS 1.1 | change 0021 (graphical UI) | 1-7 pass with the new touch rectangles (examples, `<`/`>`, Bksp, Settings `-`/`+`, Save, GO, STOP, Same again, hold B, New prompt, SET, Back) | yes |
