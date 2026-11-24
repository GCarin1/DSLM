---
name: ds-memory-budget
description: "Check whether a model and its buffers fit in the DS's memory and decide where each buffer lives (main RAM, ITCM, DTCM, VRAM). Use when: The task adds or resizes a model, KV cache or buffer, changes seq_len or the quantized format, or hits an allocation failure on the DS."
---
<!-- doctrina:skill-export — generated from .doctrina/skills/ds-memory-budget.md by `doctrina index rebuild`; edit that file, not this one. -->

# Skill — ds-memory-budget

## When to use this skill

- Choosing or converting a checkpoint.
- Changing `seq_len`, `dim`, the KV cache type or the int8 group size.
- An allocation fails or the heap peak in `bench.txt` grows.
- Moving a kernel or buffer into ITCM/DTCM.

## Procedure

1. Run the `pc-toolchain` inspect on the checkpoint and read the header fields and the budget.
2. Budget against **about 3.5 MB**, not 4 MB: main RAM ends at the IPC region (`0x23FF000`), the binary and stack come first, and DLDI takes 16 KB.
3. Sum: weights + KV cache (`2 × n_layers × seq_len × kv_dim × bytes`) + run-state buffers + tokenizer + code. Keep headroom for the console and the libc heap.
4. Placement: matmul inner loop in ITCM (32 KB); current activation vector and accumulators in DTCM (16 KB); large scratch buffers may go to unused VRAM banks (no 8-bit writes); everything else in main RAM.
5. Allocate once after loading. Confirm the heap peak in `bench.txt` on hardware.

## Anti-patterns

- Streaming weights from the microSD card per token (ADR 0002).
- Allocating inside the per-token loop.
- Byte-writing into VRAM.
- Assuming stories15M fits: about 16 MB even in int8.

## Related material

- ADR 0002, ADR 0003
- `docs/hardware-ds.md`
- `.doctrina/specs/model-loading/spec.md`, `.doctrina/contracts/model-file-format.md`
