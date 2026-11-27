# Manual test: ui-sound

**Status:** runnable on melonDS since change 0024. Proves `ds-interface`: the interface plays its own synthesized sounds, and the Sound switch on the Settings screen silences them and is saved (ADR 0014).

## Setup

Stage the card (`python tools/dslm.py stage .tools/card`) and build `dslm.nds`. melonDS with audio enabled. For an objective check, record the speaker loopback (for example with the Python `soundcard` package) and look for the notes' pitches (a PSG square wave also has a strong 3rd harmonic).

## Steps and expected result

1. Boot. When the Home screen appears: a rising chime (784, 1047, 1319, 1568 Hz, then a held 2093 Hz).
2. D-pad on Home: a short blip per move. A on a card: a two-note "open" (1319 → 1976 Hz).
3. On the prompt: a click per typed key, a lower blip on delete, two quick notes on L/R, X or `?123`, and the "start" arpeggio on START.
4. Tap B during generation: a falling "stop". Let a generation finish: the "done" chime.
5. SELECT: the falling "back".
6. Home → Settings → the Sound row (Up from Temperature) → Right: the sounds go silent at once. START saves `sound = off` in `/dslm/dslm.ini`. Back without saving restores the saved choice.
7. Boot with `sound = off`: no chime and no clicks.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0024 | Loopback recording: the boot chime notes 784, 1047, 1568 and 2093 Hz at 0.1 s steps and the open sound 1319 → 1976 Hz on A (the 1000 Hz probe first showed the PSG rate is 8x the pitch, now corrected); step 6 saved `sound = off`; int8 parity 129/129, 256 steps 35,460 ms | yes |
| 2026-11-26 | DSi (DS mode), reference R4 / YSMenu | v1.3 development build | boot chime and interface sounds play on the DSi speaker; checked by the owner | no |
