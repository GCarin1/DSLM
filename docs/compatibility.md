# Compatibility

Where DSLM has been run, and how. Hardware results come from the DS's own timers (`emulated=no`); emulator results are indicative only. Add a row for every new cart or emulator you try (cart label domain, kernel, microSD), and import hardware runs with `python tools/dslm.py bench` (`tests/hw/runbook.md`).

## Flashcarts

| Cart | Kernel | microSD | DLDI | Result | Evidence |
|---|---|---|---|---|---|
| R4 clone booting `R4.dat`/`TTMenu.dat` (DSONEi SDHC / R4i-SDHC YSMenu package) — **reference** (ADR 0009) | YSMenu | 3.8 GB FAT32 partition (card reports 64 GB, probably counterfeit) | Auto-patched by YSMenu (`ttdldi.dat`) | ✅ Loads, generates, fp32 and int8 parity; stories260K and tiny1m | `benchmarks/2026-11-25-r4-ysmenu*.txt` |
| Original R4 (r4ds.com) / M3 Simply | Wood R4 1.62 | ≤ 2 GB FAT | `r4tf.dldi` | Not tested yet | — |
| DSTTi "DEMON"/timebomb clones | Pico-Launcher, AKMenu-Next, YSMenu | — | Kernel auto-patch | Not tested yet; avoid R4iMenu | — |

## Consoles

| Console | Mode | Result |
|---|---|---|
| Nintendo DS (model not recorded) | DS mode | ✅ Works (reference runs) |
| DS Lite | DS mode | Expected to work (same hardware); not tested yet |
| DSi / DSi XL | DS mode via the reference R4 | ✅ Works: the v1.3 development build (graphical interface, Home, history, peek, paths, book, voice) ran all four models, with int8 parity 129/129 on hardware (`benchmarks/2026-11-26-r4-ysmenu-dsi-v1-3-dev-*.txt`) |
| 3DS / 2DS | DS mode via flashcart | Not tested; DSi/3DS native mode is out of scope |

## Emulators

| Emulator | Setup | Result |
|---|---|---|
| melonDS 1.1 (Windows) | DLDI on, "Sync SD to folder" (`docs/testing.md`) | ✅ Works; unattended parity runs (`tests/parity/run_melonds.py`) |
| DeSmuME 0.9.13 (Windows) | Slot-1 R4 + FAT folder | Not supported in 1.0 runs: no command-line switch for Slot-1 R4 and the unattended launch failed (`test-harness` spec, Future) |
| no$gba | Debugger only | Not a test target |
