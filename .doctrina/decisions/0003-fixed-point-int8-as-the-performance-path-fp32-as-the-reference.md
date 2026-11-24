# ADR 0003 — Fixed-point int8 as the performance path, fp32 as the reference

- **Status:** proposed
- **Date:** 2026-11-24
- **Deciders:** Carini
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** n/a — no implementation yet
- **Landed:** —

## Context

The DS's ARM946E-S (ARMv5TE, ~67 MHz) has no floating-point unit. Every `float` add or multiply becomes a call to a software routine (`__aeabi_fadd`, `__aeabi_fmul`), tens of times slower than an integer operation. On the other hand, ARMv5TE has 16×16 DSP multiply-accumulate instructions (`SMULxy`/`SMLAxy`) and saturating arithmetic (`QADD`). The DS also has a memory-mapped hardware divider and square root. At the same time, the project needs a build whose output can be compared token by token with `llama2.c` on a PC (SC2).

## Decision

The project keeps two builds:

1. **fp32 reference** (`inference-engine`): a direct port of `run.c`, with emulated `float`. It is slow and exists to prove correctness and exact parity.
2. **int8 / fixed-point performance** (`fixed-point-inference`): int8 weights with per-group scales and fixed-point activations. Hot kernels live in ITCM, division and square root use the DS hardware, and `exp()` uses an approximation or a lookup table. Parity is checked against `runq.c` on the PC.

## Alternatives considered

1. **fp32 only.** Simple, but soft-float likely prevents reaching a few tokens per second (SC3).
2. **Fixed point only, no fp32 build.** Faster to deliver, but no exact oracle on the DS. Quantization error and porting bugs would be indistinguishable.
3. **Emulated fp16.** Gains nothing without hardware support.

## Consequences

**Positive**

- Separates "is it correct?" from "is it fast?". Each build has a clear acceptance criterion.

**Negative**

- Two code paths to maintain, and the int8 build is a rewrite of the forward pass.

**Neutral**

- The int8 checkpoint format is still pending in `contracts/model-file-format`.
