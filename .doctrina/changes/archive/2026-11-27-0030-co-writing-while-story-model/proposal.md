# Change 0030-co-writing-while-story-model — Co-writing: while a story model generates, Y hands the next tokens to the user; at each step the five most likely tokens are offered with their odds, Up/Down and A or a touch pick one, Y gives the pen back to the model and B stops; the waiting time is left out of the speed figures

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

Co-writing: while a story model generates, Y hands the next tokens to the user; at each step the five most likely tokens are offered with their odds, Up/Down and A or a touch pick one, Y gives the pen back to the model and B stops; the waiting time is left out of the speed figures

## What

Second change of v1.5 ("inside the model"). It builds on the step hook from change 0029.

- `src/main.c`:
  - Y during generation sets `cowrite`. The `peek_step` hook then calls `choose_token()`, which shows the top-5 as choices and waits (`UI_CHOOSE`): Up/Down, A or touch picks, Y gives the pen back, B stops.
  - The pick is returned to the engine to force. The wait is taken out of `ticks`.
  - `peek_texts()` and `gen_panel()` are factored out.
- `src/ui/ui_gfx.c`, `ui.h`:
  - Peek mode 3 shows the "Your turn" header, the choice rows (`ui_choices()`, a highlight shared with the peek rows through `odds_rows()`) and its own footer.
  - Events `UI_EV_COWRITE` and `UI_EV_PICK` with their sounds. The footer of peek modes 1-2 offers Y.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-cowrite.md`. `tests/hw/ui-peek.md` gets the new footer text.

## Scope boundaries

- Story models only (the chat engine has no step hook). No free typing of tokens outside the five offered.
- No engine change beyond using the 0029 hook's forcing.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test`, `make nds` with no warnings, and `doctrina verify`. Hook forcing is covered by `tools/tests/test_engine_host.py` (change 0029).
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/hw/ui-cowrite.md`.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=951f1b12,96c970bb,d90b5a5b -->
