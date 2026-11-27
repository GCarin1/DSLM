# Manual test: ui-voice

**Status:** runnable on melonDS since change 0033. Proves `ds-interface`: voice mode measures the microphone, a sustained loud sound starts the generation, and its loudness sets that run's temperature (ADR 0015).

## Setup

Stage the card and build `dslm.nds`. In melonDS, set the microphone input to white noise (Config > Input and hotkeys > Microphone, or `InputType = 2` under `[Mic]`) and map the "Mic" hotkey (`HK_Mic` under `[Instance0.Keyboard]`). Hold it to blow. On hardware, blow on the microphone.

## Steps and expected result

1. Stories. X: the header shows "MIC" and a level meter with a grey threshold mark, and the hint reads "blow:go START:go X:mic off".
2. A short sound (under a quarter of a second) does nothing.
3. A sustained loud sound: the meter turns red, the run starts, and the hint shows "voice:temp N.N" (1.2 for the loudest). The text differs from the temperature-0 story.
4. After the run, voice mode is off and `dslm.ini` keeps its temperature. X again turns it back on. SELECT (menu) switches it off.
5. Shift still works on the `Aa` key (D-pad or touch).

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-27 | melonDS 1.1 | change 0033 | 1-4 pass with white noise and the Mic hotkey: a 0.12 s tap did not trigger, a 1 s blow started the run at "voice:temp 1.2" ("…a little bird named Lily…"), stopped by B; the saved temperature was unchanged. Without a held hotkey melonDS feeds silence (meter at 0) | yes |
| 2026-11-26 | DSi (DS mode), reference R4 / YSMenu | v1.3 development build | voice mode meter and blow trigger work on the real microphone; checked by the owner | no |
