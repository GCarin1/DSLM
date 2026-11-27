# Change 0033-voice-mode-microphone-prompt — Voice mode with the microphone: X on the prompt screen switches the microphone on or off and shows a live level meter; blowing or speaking loudly for about a quarter of a second starts the generation, and how loud it was sets the temperature for that run (soft 0.2 to strong 1.2) without changing the saved settings; shift stays on the Aa key; blow detection and the loudness-to-temperature mapping are pure C tested on the host

- **Status:** applied
- **Applied:** 2026-11-27
- **Date:** 2026-11-27
- **Owner:** Carini
- **Lane:** product (uncertain)
- **Affects specs:** ds-interface

<!--
Optional, and usually absent. The closing docs gate reads COMMAND and FLAG
names out of the prose below and asks for documentation when it finds any.
It cannot tell a change from a mention: explaining an effect, or writing a
Scope boundaries line about what this deliberately does NOT touch, names
things just as loudly as changing them would.

When that happens, say so on the record instead of forcing the close:

- **Documented surface:** n/a — names two commands to explain an effect; alters neither

`none` reads the same as `n/a`, and a BARE one silences nothing — the
reason is the declaration.
-->

## Why

Voice mode with the microphone: X on the prompt screen switches the microphone on or off and shows a live level meter; blowing or speaking loudly for about a quarter of a second starts the generation, and how loud it was sets the temperature for that run (soft 0.2 to strong 1.2) without changing the saved settings; shift stays on the Aa key; blow detection and the loudness-to-temperature mapping are pure C tested on the host

## What

The v1.6 ("microphone") change. Rumble was dropped because the deciders' DSi has no slot 2.

- `src/ui/mic.c/.h` (new, pure C): the DC-removed level of 16-bit samples, the blow detector (40/127 for 15 frames, 90-frame cooldown, reports the peak) and the peak-to-temperature mapping (0.2-1.2). Tested by `tests/host/test_mic.c` (new, in `tests/host/Makefile`).
- `src/ui/ui_gfx.c`, `ui.h`:
  - `ui_mic()` starts `soundMicRecord()` (12-bit, 8 kHz, a 2 x 400-sample buffer) or stops it. The callback invalidates the cache before measuring.
  - The prompt header draws the meter. On the prompt screen X raises `UI_EV_MIC` (shift stays on `Aa`), and a detected blow raises `UI_EV_BLOW` with the peak.
- `src/main.c`: X toggles voice mode. A blow becomes GO with `voice_milli`, which sets the temperature for that run only (restored afterwards; the hint shows it). The microphone is switched off on GO and on the menu.
- ADR 0015 (amends ADR 0012's X). Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`, `tests/hw/ui-keyboard.md`. New runbook `tests/hw/ui-voice.md`.

## Scope boundaries

- No speech recognition and no voice recording kept on the card.
- No Rumble Pak (no slot 2 on the DSi).
- No engine change.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (with mic), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/host/test_mic.c` and `tests/hw/ui-voice.md` (melonDS noise input with the Mic hotkey).

## Open questions

None. The threshold of 40/127 will be checked on the DSi with the release run.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=bb33033d,75a3fe10,dfa98985,8f1b57c9,d90b5a5b -->
