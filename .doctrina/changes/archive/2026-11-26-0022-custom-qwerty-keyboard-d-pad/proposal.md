# Change 0022-custom-qwerty-keyboard-d-pad — Custom QWERTY keyboard with D-pad and touch typing: replace the libnds keyboard on the prompt screen with a keyboard drawn in the DSLM style, navigable with the D-pad (A types the focused key, B deletes, Y types a space, X toggles shift/symbols, L/R example prompts, START generates, SELECT settings) and by touch; input sampled in the VBlank interrupt and latched so a short B tap stops generation; keyboard layout and focus movement are pure C tested on the host

- **Status:** applied
- **Applied:** 2026-11-26
- **Date:** 2026-11-26
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

Custom QWERTY keyboard with D-pad and touch typing: replace the libnds keyboard on the prompt screen with a keyboard drawn in the DSLM style, navigable with the D-pad (A types the focused key, B deletes, Y types a space, X toggles shift/symbols, L/R example prompts, START generates, SELECT settings) and by touch; input sampled in the VBlank interrupt and latched so a short B tap stops generation; keyboard layout and focus movement are pure C tested on the host

## What

Change 2 of 4 in the graphical interface update.

- `src/ui/keyboard.c/.h` (new, pure C): 34-key QWERTY layout in four rows below y 76, lower/upper/symbol pages, one-shot shift, D-pad focus movement (wrapping rows and columns, the nearest key by centre), hit-testing.
- `src/ui/ui_gfx.c`: draws the keyboard in the ADR 0011 style with a cyan focus. The libnds keyboard and its carry-over guard are gone. Input is sampled in the VBlank interrupt and latched (presses, repeats, touch-down position), and `ui_poll()` drains the latches. On the prompt: D-pad focus, A types, B deletes, Y space, X shift, START go. During generation a latched B or STOP tap stops.
- `src/main.c`: welcome texts and hints. `src/ui/gfx.c/.h`: comments (the palette entries and VRAM freed by the libnds keyboard).
- ADR 0012. The ds-interface delta covers D-pad and touch typing and the tap-to-stop requirement.
- Tests: `tests/host/test_keyboard.c` (new; in `tests/host/Makefile`), `tests/hw/ui-keyboard.md` (new), `ui-navigation.md` and `ui-cancel.md` updated. Docs: `docs/user-guide.md` (control table), the package README in `tools/dslm.py`, `CHANGELOG.md`.

## Scope boundaries

- No Home menu, About screen or motion (change 3). No engine, storage or format change.
- Story and Settings controls are unchanged. Only the prompt screen's map changes (A types; Up/Down no longer scroll there).

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (ini, textview, keyboard, fxmath), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/host/test_keyboard.c` and `tests/hw/ui-keyboard.md` (melonDS: D-pad typing, touch typing, a 0.12 s B tap stops, int8 parity 129/129).

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=5f47a075,b1258bc4,dee43d19,2abdf210,24f4ebb2,d90b5a5b -->
