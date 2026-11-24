# Spec — inference-engine

**Capability:** inference-engine
**Status:** active
**Implementation:** planned — current phase is documentation and planning only; no code yet
**Realizes:** SC1, SC2
**Depends on:** model-loading
**Last updated:** 2026-11-24
**Version:** 0.1.0

## Purpose

A port of the core of `llama2.c`'s `run.c` to the Nintendo DS ARM9. It covers the Transformer forward pass (RMSNorm, attention with RoPE and KV cache, SwiGLU FFN), BPE tokenizer encode/decode and the sampler (argmax, temperature, top-p). This is the **fp32 reference build**. It is slow, because the ARM946E-S has no FPU and every `float` operation is emulated in software, but it exists to prove correctness: at temperature 0 it must produce exactly the same tokens as `llama2.c` on a PC (ADR 0001, ADR 0003).

## Requirements (EARS)

### Ubiquitous

- The engine shall implement the same forward pass, tokenizer and sampler as `run.c` from `llama2.c`, using 32-bit floats.
- The engine shall be plain C with no dependency beyond the toolchain's C library and libnds.

### Event-driven

- When a prompt is submitted, the engine shall encode it with the BPE tokenizer and generate tokens until it reaches the configured step limit or the BOS token.
- When a token is produced, the engine shall hand its decoded text to the caller immediately, so the interface can stream it.

### State-driven

- While temperature is 0, the engine shall pick the argmax token at every step, so the output is deterministic.

### Unwanted-behavior (must-not)

- The engine shall not allocate memory during token generation; every buffer shall be allocated once, after loading.
- The engine shall not depend on `mmap()`, threads or any operating-system service.

### Optional

- Where a seed is given, the engine may use it for the sampler's random number generator, so sampled runs can be reproduced.

## Acceptance criteria

1. [unverified] With `stories260K`, temperature 0 and the skeleton's fixed prompt, the token sequence on the DS is identical to the PC golden output — verified by `tests/parity/fp32-stories260k.md`.
2. [unverified] The same `.nds` generates text on a real R4 with no network — verified by `tests/hw/offline-run.md`.
3. [unverified] The fp32 build's time per token is measured and recorded (baseline for the fixed-point build) — verified by `tests/hw/benchmark-fp32.md`.

## Maturity

**MVP (committed):**

- fp32 forward pass, tokenizer and sampler, with a fixed prompt.

**Future (aspirational, not committed):**

- Placing the hot loops in ITCM/DTCM and using the DS hardware divider and square root in the fp32 build too.

## Out of scope for this spec

- Performance (`fixed-point-inference`).
- Text input and screen layout (`ds-interface`).
