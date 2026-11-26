# Manual test: ui-keyboard

**Status:** runnable on melonDS since change 0022. Proves `ds-interface`: the prompt can be typed with the D-pad and A alone or by touch on the QWERTY keyboard, and a short B tap stops generation (ADR 0012).

## Setup

Stage the card (`python tools/dslm.py stage .tools/card`) and build `dslm.nds`. Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Boot. Prompt screen: the keyboard fills the bottom screen below the buttons, with `q` highlighted in cyan.
2. Press B sixteen times: the prompt is empty.
3. X, Down, Right ×5, A: `H` is typed and the keyboard goes back to lower case. Up, Right ×2, A: `i`. Y: a space. Left ×7, A: `q` (the row wraps). The prompt reads `Hi q`.
4. Touch `?123`: the digits and symbols page. Touch `abc`: back to letters.
5. With the stylus lifted between taps, touch `q`, `w`, `e`, `a`, `s`: those letters are typed. Touch `DEL`: one is deleted.
6. START (or touch GO): generation starts. After about 3 s, tap B for about 0.1 s: the Story screen says "Stopped" and keeps the text.
7. Parity: `python tests/parity/run_melonds.py --steps 128 --model int8` passes (no engine change).

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0022 | 1-7 pass: `Hi q` by D-pad, the symbols page, `qweas` and DEL by touch, a 0.12 s B tap stopped at 31 tokens, int8 parity 129/129; `dslm.nds` 215,040 bytes (v1.2.0: 217,088) | yes |
