# Change 0026-personalization-ds-firmware-greet — Personalization from the DS firmware: greet the user by the name stored in the DS settings on the Home screen and in the welcome text, put the name into one story example, and a Color row on the Settings screen that switches the interface accent between the DSLM cyan and the user's favourite colour from the DS settings, saved in dslm.ini; the colour table, shade derivation and name conversion are pure C tested on the host

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

Personalization from the DS firmware: greet the user by the name stored in the DS settings on the Home screen and in the welcome text, put the name into one story example, and a Color row on the Settings screen that switches the interface accent between the DSLM cyan and the user's favourite colour from the DS settings, saved in dslm.ini; the colour table, shade derivation and name conversion are pure C tested on the host

## What

Second change of v1.4 ("personal and practical").

- `src/ui/theme.c/.h` (new, pure C): the 16 DS favourite colours (approximations, in menu order), dark and light shades of an accent, and the UTF-16 firmware name as trimmed ASCII. Tested by `tests/host/test_theme.c` (new, in `tests/host/Makefile`).
- `src/ui/ui_gfx.c`, `ui.h`:
  - `ui_accent()` rewrites the cyan family and the pulse colour in both palettes from `PersonalData->theme`, or restores the logo cyan.
  - `ui_user_name()` reads `PersonalData->name`. The Home header greets "Hi, <name>!".
  - Settings has 6 rows with the Colour row; the note and buttons moved down.
- `src/main.c`: the welcome text starts with the greeting, story example 3 becomes "<name> wanted to", and the Colour row previews and is restored when you leave without saving.
- `src/storage/ini.c/.h`: `[ui] accent = dslm|ds` (default `dslm`), with tests. The contract `model-file-format` documents it.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-personal.md`.

## Scope boundaries

- The logo art (splash) keeps its own colours. Status colours (green, amber, red) do not follow the accent.
- No birthday or language use yet. The first story example stays fixed for parity.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (ini, history, textview, keyboard, sfx, theme, fxmath), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/host/test_theme.c` and `tests/hw/ui-personal.md`.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=eec2b6fa,4074668b,e9165b99,723a4565,d90b5a5b -->
