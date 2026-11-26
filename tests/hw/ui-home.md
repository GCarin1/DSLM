# Manual test: ui-home

**Status:** runnable on melonDS since change 0023. Proves `ds-interface`: the Home screen opens every mode and screen by D-pad and touch, Chat loads its model by itself, chat answers show as bubbles, and the mode survives a reboot (ADR 0013).

## Setup

Stage the card (`python tools/dslm.py stage .tools/card`) and build `dslm.nds`. Start from a `dslm.ini` without `[ui]` (as staged). Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Boot. After the splash: the logo on top. Below are the Stories (`stories260K_q80`), Chat (`littlekedi_q80`), Settings and About cards, with Stories highlighted.
2. Right: Chat is highlighted. Left, A: the prompt of the story model (fade in). SELECT: back to Home.
3. Down, A: Settings, whose Model row lists only story models. B: Home. Right, A: About shows the model, its context and the free RAM. B: Home.
4. Up, A (on Chat): the splash with "Loading littlekedi_q80", then the chat prompt with the first example question. START: the question appears in a cyan bubble on the right and the answer in a dark bubble on the left, and the brain on the bottom screen pulses. SELECT: Home, with Chat highlighted.
5. Close melonDS. `dslm.ini` now holds `chat_checkpoint = ...` and `[ui] mode = chat`. Boot again: LittleKedi loads and Chat is highlighted.
6. Touch About, then its Back. Touch Stories: the story model loads, and `dslm.ini` holds `mode = stories`.
7. Parity through Home: `python tests/parity/run_melonds.py --steps 256 --model int8` and `--model littlekedi` pass (the runner presses START on the Home card, then on the prompt).

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0023 | 1-6 pass (captures checked; keys and touch posted to the window) | yes |
| 2026-11-26 | melonDS 1.1 | change 0023 | 7: int8 parity 129/129, 256 steps 35,503 ms (change 0021: 35,455; v1.2.0: 36,659); LittleKedi parity 40/40, heap peak 3,420,968 B; `dslm.nds` 219,136 bytes | yes |
