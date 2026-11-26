# Change 0021-graphical-ui-base-visual — Graphical UI base and visual identity: replace the libnds text console with a tiled-graphics renderer (custom pixel font, palette from the DSLM logo, LZ77 assets embedded in the .nds and decompressed to VRAM via grit), a splash screen with the logo, a 32x32 banner icon, and the existing five screens redrawn 1:1 in the new look; supersedes ADR 0006's console decision; must keep tok/s within 2%, binary growth within 64 KB and LittleKedi loadable

- **Status:** applied
- **Applied:** 2026-11-26
- **Date:** 2026-11-26
- **Owner:** Carini
- **Lane:** product (confident; signals: must)
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

Graphical UI base and visual identity: replace the libnds text console with a tiled-graphics renderer (custom pixel font, palette from the DSLM logo, LZ77 assets embedded in the .nds and decompressed to VRAM via grit), a splash screen with the logo, a 32x32 banner icon, and the existing five screens redrawn 1:1 in the new look; supersedes ADR 0006's console decision; must keep tok/s within 2%, binary growth within 64 KB and LittleKedi loadable

## What

Change 1 of 4 in the graphical interface update (then the custom QWERTY keyboard, the Home menu and motion, and the v1.3.0 release).

- `src/ui/gfx.c/.h` (new): mode 5 with one 8bpp bitmap per screen (VRAM A and C). Fills, rounded rectangles, indexed blits and the proportional pixel font, written with 16-bit read-modify-write.
- `src/ui/ui_gfx.c` (new, replaces `ui_console.c`): the same five screens and control map, redrawn as a title bar, body and hint chips on top and header and button panels below. Touch targets are rectangles. The body repaints only changed lines. The logo splash and loading panel show at boot and on model loads. The libnds keyboard moves to BG1, and a touch that opens the prompt screen is not typed.
- `src/ui/ui.h`: `ui_splash()` and `ui_text_lines()`. `UI_TEXT_ROWS` is 14. `src/main.c` uses them.
- `src/ui/textview.c/.h`: `tv_wrap_px()` wraps by pixel width with a per-byte advance table. `tv_wrap()` becomes its 1-per-byte case.
- Art: `assets/ui/font.txt` (original font) and `assets/ui/icons.txt` (brain icon). `tools/ui_assets.py` writes `src/ui/art_data.c/.h` (palette, font, icon), `assets/ui/gfx/splash.png` with `splash.grit` (grit LZ77 at build time), and `assets/ui/icon.png` (banner). `nds.mk` sets `GFXDIRS`, `GAME_ICON` and a banner title without the stale version.
- ADR 0011 supersedes ADR 0006. The ds-interface spec gains the graphics requirement and the speed and size budget.
- Tests: `tests/host/test_textview.c` (proportional wrap), `tests/hw/ui-graphics.md` (new), `tests/hw/ui-navigation.md` (rerun). Docs: `docs/building.md`, `docs/user-guide.md`, `CHANGELOG.md`.

## Scope boundaries

- No new screens and no control changes: the Home menu, the About screen and motion come in change 3.
- The libnds keyboard stays until change 2, and so does the hold-B stop (VBlank-latched input arrives with the keyboard).
- No engine, storage or file-format change. Token parity must stay identical.
- No real-hardware benchmark here: the R4 run of the graphical build lands with the v1.3.0 release.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (ini, textview with the new proportional-wrap tests, fxmath), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`). New criteria are proven by `tests/hw/ui-graphics.md`; criterion 4 was rerun in `tests/hw/ui-navigation.md`.
- [x] Budgets (melonDS, against v1.2.0 built from `c5aa8d7`): int8 256 steps 35,455 ms vs 36,659 ms; binary +10,240 bytes; LittleKedi parity 40/40 with a heap peak of 3,417,872 B; int8 parity 129/129.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=6fdf1697,69751f84,6e07ddd9,8a24ac83,e438f2d9,ad2293c8,d90b5a5b -->
