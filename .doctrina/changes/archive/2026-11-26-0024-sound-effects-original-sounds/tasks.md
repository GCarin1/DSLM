# Tasks — Change 0024-sound-effects-original-sounds

- [x] `src/ui/sfx.c/.h`: original note sequences and the frame sequencer; host tests in `tests/host/test_sfx.c`
- [x] `ui_gfx.c`: PSG voice in the VBlank interrupt (rate 8x pitch), event sounds in `ui_poll()`, the Sound row in Settings
- [x] `main.c`: boot, done and error sounds; Sound toggle with preview; `dslm.ini` `[ui] sound` with tests and the contract
- [x] melonDS: loopback recording of the chime and open sound, Sound off saved, int8 parity and timing (`tests/hw/ui-sound.md`)
- [x] ADR 0014; ds-interface delta and the Maturity prose merge; docs and CHANGELOG
