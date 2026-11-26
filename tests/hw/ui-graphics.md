# Manual test: ui-graphics

**Status:** runnable on melonDS since change 0021. Proves `ds-interface`: the screens are bitmap graphics in the DSLM palette and font, the splash shows the logo, and the graphical interface costs no generation speed and stays within its binary budget (ADR 0011).

## Setup

Stage the card (`python tools/dslm.py stage .tools/card`) and build `dslm.nds`. For the budget comparison, also build the last console release (v1.2.0, commit `c5aa8d7`) from a `git archive` copy. Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Boot. The top screen shows the splash: the DS from the logo with the brain, and "RUN LLMs ON NINTENDO DS" under it. The bottom shows "Loading", the model name, a cyan progress bar and the boot log.
2. After loading: the title bar has the brain icon, "DSLM", the model name and a green `ready` pill. The body has proportional text, and the hint bar has button chips (`START`, `L/R`, `SEL`). The bottom has the prompt field with a cyan cursor, the `<` `>` `SET` `GO START` buttons and the keyboard.
3. START: the pill turns amber (`generating`). The text streams line by line, and the bottom shows the tokens, the bar, ms per token and the red STOP button with a `B` chip.
4. Story screen: a green `Done` header (or amber `Stopped`), the time and tokens/s, and three buttons with `A`, `Y` and `SELECT` chips.
5. SELECT: Settings rows with `-`/`+` (or `<`/`>` on Model). The selected row is highlighted with a cyan bar. Save and Back.
6. On Settings, touch Back and hold the stylus for half a second: the prompt screen appears and nothing is typed.
7. Budget: run `python tests/parity/run_melonds.py --steps 256 --model int8` with each build and compare `gen_ms` in `/dslm/bench.txt`. Compare the size of the two `dslm.nds` files. Run `--model littlekedi` with the new build: parity ok.
8. Check the flashcart menu (or `ndstool -i`): the banner shows the 32x32 brain icon, "DSLM" and "Run LLMs on Nintendo DS".

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0021 | 1-6 pass (captures checked; touch and keys posted to the window) | yes |
| 2026-11-26 | melonDS 1.1 | change 0021 vs v1.2.0 | 7: int8 256 steps 35,455 ms vs 36,659 ms (7.22 vs 6.98 tok/s, not slower); `dslm.nds` 227,328 vs 217,088 bytes (+10,240); int8 parity 129/129; LittleKedi parity 40/40, heap peak 3,417,872 B | yes |

Emulator timings compare two builds on the same machine only. They are not hardware results (`benchmarks/` holds those). The R4 run of the graphical build is recorded with the v1.3.0 release.
