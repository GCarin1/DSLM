# ADR 0011 — Graphical bitmap UI with the DSLM palette, pixel font and logo splash; every action on buttons and touch

- **Status:** accepted
- **Date:** 2026-11-26
- **Deciders:** Carini (approved the plan, the Home menu, the QWERTY keyboard, embedded assets and the four-change split on 2026-11-26)
- **Supersedes:** 0006
- **Superseded by:** —
- **Evidence:** `src/ui/gfx.c`, `src/ui/ui_gfx.c`, `src/ui/art_data.c`, `tools/ui_assets.py`, `assets/ui/`, `nds.mk`
- **Landed:** 2026-11-26 — `src/ui/gfx.c`, `src/ui/ui_gfx.c`, `tests/hw/ui-graphics.md`

## Context

ADR 0006 built the interface on the libnds text console. It worked but looked like a program running in a terminal, with 32 fixed-width columns and ASCII boxes, and nothing in it matched the project's logo (`assets/dslm-logo.png`). It also deferred custom graphics and a custom keyboard to later, behind `src/ui/ui.h`. The constraints are tight. Main RAM is nearly full with LittleKedi (heap peak 3.42 MB against a budget of about 3.5 MB). Generation runs at 3 to 7 tokens per second and must not slow down. VRAM, on the other hand, is almost unused (656 KB, none of it used by the engines). This is the first of four changes: graphics base (this ADR), custom QWERTY keyboard with D-pad typing, Home menu and motion, then the release.

## Decision

- **Rendering.**
  - Each screen is one 8bpp bitmap background (mode 5, BG2): the main screen on VRAM bank A and the sub screen on bank C, at 48 KB so it clears the keyboard.
  - Drawing is software primitives (`src/ui/gfx.c`): fills, 2 px rounded rectangles, indexed images and text. VRAM takes no 8-bit writes, so single pixels are read-modify-written as 16-bit pairs.
- **Identity.**
  - A 256-colour palette shared by both screens. Entries 32-47 hold the UI colours taken from the logo: navy-black, neon cyan, and the DS shell's white and grey. Entries 64-111 hold the splash art. Entries 1-31 stay free for the libnds keyboard.
  - An original proportional pixel font: ASCII 32-126, 9 rows with 7 above the baseline, an 11 px line pitch, so 14 lines of about 40 characters on the top screen.
  - The splash, shown at boot and on model loads, is a crop of the logo with the tagline.
  - A 16x12 brain icon sits in the title bar, and the 32x32 banner icon carries the logo's brain.
- **Assets.**
  - The sources live in `assets/` (the logo, `assets/ui/font.txt`, `assets/ui/icons.txt`).
  - `tools/ui_assets.py` (Pillow, developer-only) writes the committed outputs: `src/ui/art_data.c/.h`, the indexed splash PNG and the banner icon.
  - grit converts the splash at build time into LZ77 data inside the .nds, and the BIOS decompresses it straight into VRAM. Nothing new goes on the microSD card.
- **Layout.**
  - The top screen has a title bar (logo icon, model, state pill), the word-wrapped body and a hint bar. In the hint bar, each `KEY:label` is drawn as a button chip.
  - The bottom screen has a header strip and one panel per screen. Every touch target is a rectangle hit-tested by the same constant that draws it.
  - The body repaints only the lines whose text changed, so a streamed token usually repaints one line.
- **Kept from ADR 0006.**
  - Every action has a button and a touch target, and the screen flow and control map are unchanged.
  - Generation polls input once per token.
  - Settings are saved to `/dslm/dslm.ini`.
  - The libnds keyboard stays on the prompt screen, moved to BG1 because mode 5 makes BG3 a bitmap. A touch that opened the prompt screen is ignored by the keyboard until the stylus lifts.
- **Budgets.** Generation speed may drop at most 2% against the console build, and the binary may grow at most 64 KB. LittleKedi must still load.

## Alternatives considered

1. **Tiled text layers (fixed-width font tiles plus a frame tileset).** Uses less VRAM and gives hardware scrolling. But it locks the font to 8 px cells, which is the same 32 columns as the console, and makes rounded panels and a proportional font awkward. VRAM is not the scarce resource.
2. **Assets on the microSD card (`/dslm/ui/`).** No binary growth, and it would allow themes. Rejected by the deciders: it is one more file that can go missing, and embedding costs only about 10 KB.
3. **Python-only conversion (own LZ77 encoder, no grit).** Avoids a build-time tool, but grit ships with BlocksDS (ADR 0004) and keeps the PNG as the build input.
4. **Replacing the keyboard in the same change.** Kept separate so each change is testable on its own. The libnds keyboard is replaced by the custom QWERTY keyboard in the next change.

## Consequences

**Positive**

- The interface matches the logo and no longer looks like a terminal: proportional text, more characters per line, rounded buttons with DS button chips, and the logo at boot and on the flashcart menu.
- Measured on melonDS: 256 int8 tokens took 35,455 ms against 36,659 ms on the console build (faster: the console's per-token printing cost more than the line repaint). The binary grew from 217,088 to 227,328 bytes (+10 KB). LittleKedi loads with a heap peak of 3,417,872 bytes, and token parity is unchanged.
- The ui.h boundary held: `main.c` only gained `ui_splash()` and `ui_text_lines()`.

**Negative**

- 14 body lines instead of 20; more text fits per line, but long stories scroll sooner.
- Non-ASCII characters draw as `?` (the font covers ASCII only).
- The libnds keyboard still looks like the old interface until the next change.

**Neutral**

- `tools/ui_assets.py` needs Pillow, but only when the art changes. `--check` reports stale outputs.
- The banner title drops the version number (it had gone stale at 1.1.0).

<!--
Once this ADR is accepted, do not edit it. To change the decision,
create a new ADR that supersedes this one and update the "Superseded by"
header above to point at the new ADR. Status transitions:
proposed -> accepted | rejected
accepted -> deprecated | superseded by NNNN
-->
