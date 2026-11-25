# Tasks — Change 0002-m1-toolchain-card-blocksds

- [x] Add the BlocksDS `Makefile`, `src/` layout and `.gitignore` for build output
- [x] Implement `src/storage/ini` (pure C, no libnds) with contract defaults
- [x] Add host tests `tests/host/test_ini.c` and a `tests/host/run.sh` runner
- [x] Implement `src/storage/sdcard` (FAT mount, whole-file read, DLDI hint on failure)
- [x] Define `src/ui/ui.h` and the libnds console backend `src/ui/ui_console.c`
- [x] Write `src/main.c`: mount card, read settings, print them, fail visibly
- [x] Write `docs/building.md` and the manual test `tests/hw/hello-card.md`
- [x] Wire the host test into `verify.json` once a host compiler is available
