# Spec — test-harness

**Capability:** test-harness
**Status:** active
**Implementation:** planned — current phase is documentation and planning only; no code yet
**Realizes:** SC1, SC2, SC3, SC6
**Depends on:** pc-toolchain, inference-engine
**Last updated:** 2026-11-24
**Version:** 0.1.0

## Purpose

How DSLM is tested and measured, emulator first and real hardware second. melonDS, with DLDI enabled and the SD card synced to a folder, is the primary development target. Headless melonDS runs compare the DS output token by token with the `pc-toolchain` golden outputs. DeSmuME, with its R4 Slot-1 emulation and a FAT folder, is a second opinion. The hardware runbook covers a real DS / DS Lite with an R4. Emulator timings are indicative only, because cache behavior, main-RAM wait states and DLDI latency differ from real hardware. Every number published is measured on the DS itself with its hardware timers (SC6).

## Requirements (EARS)

### Ubiquitous

- The harness shall measure model load time, tokens per second and peak heap with DS hardware timers and the allocator, never with the host clock.
- The harness shall write each run's results to `/dslm/bench.txt` on the card, in a line-oriented format the PC scripts can parse.

### Event-driven

- When a parity run finishes in headless melonDS, the harness shall compare the generated token ids with the golden output for the same prompt and report the first diverging index, if any.
- When a hardware run finishes, the operator shall copy `/dslm/bench.txt` into `benchmarks/<date>-<cart>.txt` in the repository, naming the cart and kernel used.

### State-driven

- While a build is under test in an emulator, the harness shall label every timing it reports as emulated.

### Unwanted-behavior (must-not)

- The harness shall not publish a tokens-per-second figure measured in an emulator as a hardware result.

### Optional

- Where a debugger is needed, the harness may use no$gba for breakpoints and cycle counts; its SD flow is not a test target.

## Acceptance criteria

1. [unverified] A headless melonDS run of the fp32 build reports zero divergence against the stories260K golden output — verified by `tests/parity/run_melonds.py`.
2. [unverified] The same build runs in DeSmuME with Slot-1 set to R4 and a FAT folder — verified by `tests/emu/desmume.md`.
3. [unverified] A real R4 run produces a `benchmarks/` file with load time, tokens per second and peak heap — verified by `tests/hw/runbook.md`.

## Maturity

**MVP (committed):**

- melonDS parity runs, DeSmuME smoke run, manual hardware runbook with recorded benchmarks.

**Future (aspirational, not committed):**

- Running the melonDS parity suite in GitHub Actions on every push.
- A benchmark table covering at least three R4 clone families.

## Out of scope for this spec

- Producing the golden outputs themselves (`pc-toolchain`).
