# Spec — test-harness

**Capability:** test-harness
**Status:** active
**Implementation:** implemented — bench.txt with heap peak (`src/main.c`), import into benchmarks/ (`tools/dslm.py bench`), unattended melonDS parity (`tests/parity/run_melonds.py`), golden and parity tools; the v1.0 hardware bench run is pending
**Realizes:** SC1, SC2, SC3, SC6
**Depends on:** pc-toolchain, inference-engine
**Last updated:** 2026-11-24
**Version:** 0.2.0

## Purpose

How DSLM is tested and measured, emulator first and real hardware second. melonDS, with DLDI enabled and the SD card synced to a folder, is the primary development target. Headless melonDS runs compare the DS output token by token with the `pc-toolchain` golden outputs. DeSmuME, with its R4 Slot-1 emulation and a FAT folder, is a second opinion. The hardware runbook covers a real DS / DS Lite with an R4. Emulator timings are indicative only, because cache behavior, main-RAM wait states and DLDI latency differ from real hardware. Every number published is measured on the DS itself with its hardware timers (SC6).

## Requirements (EARS)

### Ubiquitous

- The harness shall measure model load time, tokens per second and peak heap with DS hardware timers and the allocator, never with the host clock.
- The harness shall write each run's results to `/dslm/bench.txt` on the card, in a line-oriented format the PC scripts can parse.

### Event-driven

- When a parity run finishes in headless melonDS, the harness shall compare the generated token ids with the golden output for the same prompt and report the first diverging index, if any.
- When a hardware run finishes, the operator shall copy `/dslm/bench.txt` into `benchmarks/<date>-<cart>.txt` in the repository, naming the cart and kernel used.
- When a benchmark run is imported, the harness shall add the cart, kernel, microSD and commit and refuse to write an emulated run into benchmarks/.

### State-driven

- While a build is under test in an emulator, the harness shall label every timing it reports as emulated.

### Unwanted-behavior (must-not)

- The harness shall not publish a tokens-per-second figure measured in an emulator as a hardware result.

### Optional

- Where a debugger is needed, the harness may use no$gba for breakpoints and cycle counts; its SD flow is not a test target.

## Acceptance criteria

1. [unverified] A real R4 run produces a `benchmarks/` file with load time, tokens per second and peak heap — verified by `tests/hw/runbook.md`.
2. [verified] An unattended melonDS run of the fp32 and of the int8 build reports zero divergence against their golden outputs — verified by `tests/parity/run_melonds.py`
3. [verified] bench.txt blocks are parsed and imported with the header, and emulator runs are refused — verified by `tools/tests/test_bench.py`

## Maturity

**MVP (committed):**

- melonDS parity runs, DeSmuME smoke run, manual hardware runbook with recorded benchmarks.

**Future (aspirational, not committed):**

- DeSmuME as a second-opinion emulator (Slot-1 R4 with a FAT folder): the Windows build has no command-line switch for it and failed to open in an unattended run; melonDS plus real-hardware runs cover the need for v1.0.

- Running the melonDS parity suite in GitHub Actions on every push.
- A benchmark table covering at least three R4 clone families.

## Out of scope for this spec

- Producing the golden outputs themselves (`pc-toolchain`).
