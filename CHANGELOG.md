# Changelog

All notable changes to DSLM are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/). Each entry links to the Doctrina change that delivered it (`.doctrina/changes/archive/`).

## [1.0.0] - 2026-11-25

First release: a tiny Llama 2 language model that runs 100% offline on a Nintendo DS launched from an R4 flashcart.

### Added

- **Fixed-point int8 engine** (default): integer-only forward pass, int8 weights, matmul in ITCM; 6.55 tokens/s on a real DS with an R4, 4.1x the fp32 build (change 0010, ADR 0008).
- **fp32 reference engine**: line-for-line port of `llama2.c`'s `run.c`, token-for-token identical to the PC (change 0004, ADR 0001).
- **Interface**: touch keyboard, five screens (Loading, Prompt, Generating, Story, Settings), word-wrapped streaming, stop with B or STOP, example prompts on L/R, settings saved to `/dslm/dslm.ini`, tokens per second on the story screen; every action on buttons and on touch (changes 0005 and 0010, ADR 0006).
- **Model loading** from the microSD card through DLDI: format detection (fp32 or int8), size check, RAM budget before allocating, progress bar, clear errors for missing files and models that do not fit (changes 0002, 0004, 0007).
- **int8 model format**: `llama2.c` v2 (`ak42`), quantized from the pinned fp32 model with group size 4 (change 0007, ADR 0007).
- **Run records**: `/dslm/tokens.txt` after every generation and `/dslm/bench.txt` with timings and heap peak (changes 0004 and 0013).
- **PC toolchain** (`tools/dslm.py`, Python standard library only): fetch with pinned SHA-256, inspect and memory budget, quantize, stage a card, golden outputs, parity check, install package, benchmark import (changes 0003, 0006, 0007, 0013).
- **Install package**: one zip to unzip at the microSD root, with the model, README, manifest and licenses (changes 0006 and 0011).
- **Build and test environment**: Docker image on a pinned BlocksDS digest; host unit tests; parity tests against `run.c`, `runq.c` and the engine's host build; unattended melonDS runs; a binary check that the int8 path calls no soft-float routine (changes 0002, 0010, 0013).
- **Release workflow**: GitHub Actions builds, tests and publishes the release assets on a version tag (change 0014).
- MIT license (change 0011).

### Changed

- The default model is the int8 `stories260K_q80.bin`; the fp32 `stories260K.bin` ships next to it as the reference (change 0007, ADR 0007).
- Reference hardware is the R4 clone with the YSMenu kernel used for the hardware runs (ADR 0009, supersedes ADR 0005).

### Fixed

- Short button presses right after a screen change were lost while the keyboard slid in or out; the slide animation is off (change 0005).
- The title bar ran the model name into the state (`stories260K.bin[settings]`); it now keeps a space and drops the extension (change 0005).
- Generated text was cut in the middle of words on the 32-column screen; it is word-wrapped (change 0005).
- The settings printout at boot went missing in the M3 build; restored (change 0004).
- A normal end of the M1 build was shown as `ERROR`; it now ends cleanly (change 0002).
- With the group size `export.py` would pick (64), the int8 `w2` matmul would have silently used 128 of 172 inputs; the quantizer picks a group size that divides both inner sizes (change 0007).

### Hardware results

| Build | Load | Time per token | Tokens/s | Parity | File |
|---|---|---|---|---|---|
| int8 fixed point | 124 ms | 152.7 ms | 6.55 | 257/257 vs host build | `benchmarks/2026-11-25-r4-ysmenu-int8.txt` |
| fp32 reference | 249 ms | 627.5 ms | 1.59 | 153/153 vs `run.c` | `benchmarks/2026-11-25-r4-ysmenu.txt` |

[1.0.0]: https://github.com/GCarin1/DSLM/releases/tag/v1.0.0
