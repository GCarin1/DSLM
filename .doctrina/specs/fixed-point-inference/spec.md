# Spec — fixed-point-inference

**Capability:** fixed-point-inference
**Status:** active
**Implementation:** planned — current phase is documentation and planning only; no code yet
**Realizes:** SC3
**Depends on:** inference-engine, model-loading
**Last updated:** 2026-11-24
**Version:** 0.1.0

## Purpose

The performance build. It replaces software-emulated `float` with integer arithmetic: int8 weights with per-group scales (like `llama2.c`'s `runq.c`) and fixed-point activations. The goal is the SC3 tokens-per-second target on real hardware. The ARM946E-S is ARMv5TE and has 16-bit DSP multiply-accumulate instructions (`SMULxy`/`SMLAxy`). The DS also has a memory-mapped hardware divider and square root, which serve RMSNorm and softmax. `exp()` (softmax, SiLU) needs an approximation or a lookup table (ADR 0003).

## Requirements (EARS)

### Ubiquitous

- The fixed-point build shall run the forward pass with integer arithmetic only, using int8 weights with per-group scales and fixed-point activations.
- The fixed-point build shall load the quantized checkpoint format produced by `pc-toolchain`, as defined in `contracts/model-file-format`.

### Event-driven

- When generation finishes, the fixed-point build shall show the measured tokens per second on screen.

### State-driven

- While the fixed-point build is generating, the matrix-multiply inner loop shall run from ITCM.

### Unwanted-behavior (must-not)

- The fixed-point build shall not call software floating-point routines inside the per-token forward pass.

### Optional

- Where the quantized output drifts from the fp32 reference, the build may print the first diverging token index in a debug mode.

## Acceptance criteria

1. [unverified] The time per token measured on a real R4 is recorded and compared with the fp32 baseline — verified by `tests/hw/benchmark-int8.md`.
2. [unverified] No software-float symbol (`__aeabi_fmul`, `__aeabi_fadd`, ...) appears in the forward pass call map — verified by `tests/build/no-softfloat-in-forward.md`.
3. [unverified] At temperature 0, the first generated tokens match the PC int8 golden output (`runq.c` running the same quantized checkpoint) — verified by `tests/parity/int8-stories260k.md`.
4. [unverified] The int8 checkpoint the `pc-toolchain` quantizer generates is read by the reference `runq.c` — verified by `tools/tests/test_quantize.py`.

## Maturity

**MVP (committed):**

- int8 matmul with per-group scales, fixed-point RMSNorm/softmax/SiLU, tokens/s measurement.

**Future (aspirational, not committed):**

- ARM assembly kernels and using the ARM7 as a second core to split the work.

## Out of scope for this spec

- Quantization-aware training.
