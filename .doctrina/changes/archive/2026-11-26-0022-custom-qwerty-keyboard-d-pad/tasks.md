# Tasks — Change 0022-custom-qwerty-keyboard-d-pad

- [x] `src/ui/keyboard.c/.h`: layout, pages, one-shot shift, D-pad focus, hit-test; host tests in `tests/host/test_keyboard.c`
- [x] `ui_gfx.c`: draw the keyboard with focus, remove the libnds keyboard, VBlank-latched input, prompt control map, tap-to-stop
- [x] `main.c` hints and welcome texts; user guide, package README, runbooks and CHANGELOG
- [x] melonDS: D-pad and touch typing, B tap stop, int8 parity (`tests/hw/ui-keyboard.md`)
- [x] ADR 0012; ds-interface delta and the Maturity prose merge
