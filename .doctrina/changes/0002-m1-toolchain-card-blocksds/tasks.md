# Tasks — Change 0002-m1-toolchain-card-blocksds

- [ ] Add the BlocksDS `Makefile`, `src/` layout and `.gitignore` for build output
- [ ] Implement `src/storage/ini` (pure C, no libnds) with contract defaults
- [ ] Add host tests `tests/host/test_ini.c` and a `tests/host/run.sh` runner
- [ ] Implement `src/storage/sdcard` (FAT mount, whole-file read, DLDI hint on failure)
- [ ] Define `src/ui/ui.h` and the libnds console backend `src/ui/ui_console.c`
- [ ] Write `src/main.c`: mount card, read settings, print them, fail visibly
- [ ] Write `docs/building.md` and the manual test `tests/hw/hello-card.md`
- [ ] Wire the host test into `verify.json` once a host compiler is available
