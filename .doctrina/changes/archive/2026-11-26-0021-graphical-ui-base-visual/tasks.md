# Tasks — Change 0021-graphical-ui-base-visual

- [x] Art sources and generator: `assets/ui/font.txt`, `assets/ui/icons.txt`, `tools/ui_assets.py`, and the generated `src/ui/art_data.c/.h`, splash and banner icon
- [x] `tv_wrap_px()` proportional wrap, with host tests in `tests/host/test_textview.c`
- [x] `src/ui/gfx.c/.h`: bitmap screens, palette, fills, rounded rectangles, blits, pixel-font text
- [x] `src/ui/ui_gfx.c` replaces `ui_console.c`: five screens, rectangle touch targets, line-diff body repaint, splash and loading panel, keyboard on BG1 with touch carry-over guard
- [x] `ui.h`/`main.c`: `ui_splash()` and `ui_text_lines()`; `nds.mk`: grit `GFXDIRS`, banner icon and title
- [x] melonDS: screens captured, `ui-navigation` rerun, int8 and LittleKedi parity, speed and size against v1.2.0 (`tests/hw/ui-graphics.md`)
- [x] ADR 0011 (supersedes 0006); ds-interface delta plus the Purpose/Maturity prose merge; docs and CHANGELOG
