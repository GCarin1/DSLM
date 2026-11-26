# Change 0025-history-save-every-finished — History: save every finished story or chat answer to /dslm/history on the microSD with its date from the DS clock, model, prompt and text; a History card on the Home screen lists them newest first, shows the selected one on the top screen (chat in bubbles), scrolls it with L/R and deletes an entry with Y pressed twice; the file format and its parser are pure C tested on the host

- **Status:** applied
- **Applied:** 2026-11-26
- **Date:** 2026-11-26
- **Owner:** Carini
- **Lane:** chore (confident; signals: format)
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

History: save every finished story or chat answer to /dslm/history on the microSD with its date from the DS clock, model, prompt and text; a History card on the Home screen lists them newest first, shows the selected one on the top screen (chat in bubbles), scrolls it with L/R and deletes an entry with Y pressed twice; the file format and its parser are pure C tested on the host

## What

First change of v1.4 ("personal and practical").

- `src/storage/history.c/.h` (new, pure C): the history file format (`dslm-history=1` header, a blank line, the text), with `history_format()`, `history_parse()` (CRLF tolerant, the question clamped to the text, prompt newlines dropped) and the `H0001.TXT` names. Tested by `tests/host/test_history.c` (new, in `tests/host/Makefile`).
- `src/storage/sdcard.c/.h`: `sd_read_upto()` reads the head of a file of any length.
- `src/main.c`:
  - `save_history()` after each generation, with the date from the DS clock (`time()`/`localtime()`).
  - `history_screen()`: list, top-screen preview, L/R page scroll, double-Y delete. Its 8 KB list is allocated only while the screen is open.
  - The History card on Home.
- `src/ui/ui_gfx.c`, `ui.h`: Home is a 2x3 grid of 120x48 cards (icon left, title and subtitle right) with wrapping `card_move()`. `ui_history()` is the list panel with rows, Delete and Back. New events `UI_EV_PAGE_UP/DOWN` and `UI_EV_DELETE` with their sounds. The clock icon is in `assets/ui/icons.txt`.
- The contract `model-file-format` documents `/dslm/history`. Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-history.md`.

## Scope boundaries

- No reuse of a saved prompt and no export beyond the text files themselves.
- No cap on the number of files (the list shows the newest 64).
- No engine change: token parity must be unchanged.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (ini, history, textview, keyboard, sfx, fxmath), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/host/test_history.c` and `tests/hw/ui-history.md` (melonDS: save, list, preview, delete; parity unchanged).

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=a27ff3be,08c63c67,6e038036,f4c4b69e,44adb2dd,d90b5a5b -->
