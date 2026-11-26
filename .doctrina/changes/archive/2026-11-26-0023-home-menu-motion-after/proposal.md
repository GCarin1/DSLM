# Change 0023-home-menu-motion-after — Home menu and motion: after the splash a Home screen with Stories, Chat, Settings and About cards (D-pad + A or touch); choosing Chat loads the chat model and Stories loads the story model from Settings, with the existing load-and-fallback; the last mode is saved in dslm.ini and focused on the next boot; B returns to Home from the prompt; an About screen with the logo, version and models on the card; motion: fade transitions between screens, a pulsing brain while generating, and chat questions and answers drawn as bubbles

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

Home menu and motion: after the splash a Home screen with Stories, Chat, Settings and About cards (D-pad + A or touch); choosing Chat loads the chat model and Stories loads the story model from Settings, with the existing load-and-fallback; the last mode is saved in dslm.ini and focused on the next boot; B returns to Home from the prompt; an About screen with the logo, version and models on the card; motion: fade transitions between screens, a pulsing brain while generating, and chat questions and answers drawn as bubbles

## What

Change 3 of 4 in the graphical interface update.

- `src/storage/ini.c/.h`: `chat_checkpoint` and `[ui] mode = stories|chat` (parsed, defaulted, formatted), with tests in `tests/host/test_ini.c`. The contract `model-file-format` documents both.
- `src/main.c`:
  - The Home loop with Stories, Chat, Settings and About.
  - `enter_mode()` loads the mode's model with the existing fallback. `settle_mode()` makes the mode follow the kind that loaded, which handles old `dslm.ini` files. The loaded path goes in `ckpt_path` (used by `tokens.txt` and `bench.txt`).
  - Settings opens from Home and lists story models. `prompt_screen()` returns to Home on SELECT/MENU.
  - The About screen, and chat text without the `> ` marker.
- `src/ui/ui_gfx.c`, `ui.h`:
  - `ui_home()` (the logo on top, a 2x2 card grid with D-pad focus and touch) and `ui_about()`.
  - `ui_set_chat()` bubble layout, with per-line bubble shapes in the line cache.
  - `ui_pulse()` palette animation in the VBlank interrupt, and `ui_fade_out/in()` brightness fades.
  - The MENU/Menu buttons, and a 2x pulsing brain on the Generating panel.
  - `gfx_blit_scaled()`.
- Art: the card icons (book, bubble, gear, info) and the pulse colour in `assets/ui/icons.txt` and `tools/ui_assets.py`. The brain icons use `COL_PULSE`.
- `tests/parity/run_melonds.py`: boots in Stories mode and presses START on the Home card, then on the prompt.
- ADR 0013. Docs: `docs/user-guide.md`, `docs/littlekedi.md`, `README.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-home.md`.

## Scope boundaries

- No engine or model-format change: token parity must be unchanged.
- No pixel-smooth scrolling and no history on the card (Future in the spec).
- No version bump and no hardware benchmark (change 4, the v1.3.0 release).

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (ini with mode and chat model, textview, keyboard, fxmath), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/hw/ui-home.md` (melonDS walkthrough, persistence across a reboot, parity through Home) and `tests/host/test_ini.c`.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=6ea92469,ba515dc9,fb0cd8ab,9f31edc0,035f2bcf,01fe4bf1,53c1e00b,e50cbe2f,d90b5a5b -->
