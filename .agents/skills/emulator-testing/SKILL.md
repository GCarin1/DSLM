---
name: emulator-testing
description: "Run and check DSLM builds in melonDS (primary), DeSmuME (second opinion) and no$gba (debugger) with a virtual SD card. Use when: The task runs dslm.nds in an emulator, sets up the virtual SD card, automates a parity run, or interprets an emulator timing."
---
<!-- doctrina:skill-export — generated from .doctrina/skills/emulator-testing.md by `doctrina index rebuild`; edit that file, not this one. -->

# Skill — emulator-testing

## When to use this skill

- First run of any new build (always emulator before hardware).
- Setting up or debugging the virtual SD card for DLDI.
- Writing or running the headless parity check against golden outputs.
- Someone quotes a tokens-per-second figure from an emulator.

## Procedure

1. Stage the card directory with `pc-toolchain`.
2. melonDS: Config → Emu settings → DLDI → enable, 1 GB image, **Sync SD to folder** = staged directory. Copy files only while melonDS is closed. No BIOS dumps needed in DS mode.
3. Run with temperature 0. Read the token log and `/dslm/bench.txt` back from the folder and compare with the golden output. Report the first diverging index.
4. Second opinion: `desmume-cli --slot1 R4 --preload-rom --slot1-fat-dir ./card dslm.nds`. If FAT fails on an old DeSmuME, patch with `r4tf.dldi` or use MPCF.
5. Use no$gba only for breakpoints and cycle counts.

## Anti-patterns

- Publishing emulator tok/s as a hardware result. Label it `emulated=yes`.
- Timing with the host clock instead of DS hardware timers.
- Editing the sync folder while melonDS is running. Changes get lost or corrupt the image.
- Treating an emulator pass as proof the R4 works. Cache, wait states and DLDI differ.

## Related material

- `docs/testing.md`
- `.doctrina/specs/test-harness/spec.md`, `.doctrina/specs/pc-toolchain/spec.md`
