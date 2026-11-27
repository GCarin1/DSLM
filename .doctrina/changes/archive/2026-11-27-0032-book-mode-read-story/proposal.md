# Change 0032-book-mode-read-story — Book mode: read a story or a saved history entry with the DS held sideways like a book, the text rotated by the background hardware and laid out on light pages across both screens with page numbers; START on the Story screen or A in History opens it, A, R or a touch turn the page forward, Y or L back, B or START close it

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

Book mode: read a story or a saved history entry with the DS held sideways like a book, the text rotated by the background hardware and laid out on light pages across both screens with page numbers; START on the Story screen or A in History opens it, A, R or a touch turn the page forward, Y or L back, B or START close it

## What

Last change of v1.5 ("inside the model").

- `src/ui/gfx.c/.h`:
  - Surfaces carry their drawable size, and every clip uses it.
  - `gfx_book()` sets both BG2 affine matrices (pa 0, pb 1, pc -1, pd 0, y0 255) so a 192x256 page reads upright with the DS held sideways (top screen on the left), or restores the identity matrix.
- `src/ui/ui_gfx.c`, `ui.h`:
  - `ui_book_open()` wraps the text at 168 px, 20 lines per page, and returns the spreads. Its line table is on the heap while open.
  - `ui_book_show()` draws two pages: a light paper card, title, rule, dark text and page number. `ui_book_close()` restores the screens.
  - The `UI_BOOK` input maps A/R/Right/Down/touch to next, Y/L/Left/Up to previous, and B/START to close.
  - The Story panel gets a "Book (START)" button. A in History raises `UI_EV_BOOK`.
- `src/main.c`: `book_screen()` with fades, opened from the Story screen and from History (titled with the entry's date). Returning to History fades in again.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-book.md`.

## Scope boundaries

- One orientation (top screen on the left). No bubbles in book mode: the chat text reads as plain text.
- No engine change.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test`, `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/hw/ui-book.md`.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=262583bf,25cf0d73,d90b5a5b -->
