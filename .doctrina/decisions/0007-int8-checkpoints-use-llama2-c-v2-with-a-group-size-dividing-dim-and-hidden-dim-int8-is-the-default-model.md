# ADR 0007 — int8 checkpoints use llama2.c v2 with a group size dividing dim and hidden_dim; int8 is the default model

- **Status:** proposed
- **Date:** 2026-11-25
- **Deciders:** Carini
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `tools/dslm.py`, `src/engine/llama2.c`, `tools/tests/test_quantize.py`, `tests/parity/int8-stories260k.md`
- **Landed:** —

## Context

ADR 0003 makes int8 the performance path with parity against `runq.c`, and left the format open. `llama2.c`'s `export.py` writes the v2 int8 format, but it needs PyTorch and a training checkpoint; DSLM only has the published fp32 `.bin`. It also picks the group size from `dim` alone, while `runq.c`'s matmul only walks whole groups (`j <= n - GS`): with stories260K (dim 64, hidden_dim 172) and the default group size 64, the `w2` matmul would silently use 128 of 172 inputs.

## Decision

- int8 checkpoints are the `llama2.c` v2 format, byte for byte, so the stock `runq.c` is the oracle (contract `model-file-format`).
- `tools/dslm.py quantize` converts the pinned fp32 `.bin` with the Q8_0 rule of `export.py`, in plain Python.
- The group size is the largest power of two up to 64 that divides both `dim` and `hidden_dim` (4 for stories260K).
- One `.nds` runs both formats, telling them apart by the magic number; the int8 model (`stories260K_q80.bin`) is the default in `dslm.ini` and the package, and the fp32 model ships next to it as the reference.

## Alternatives considered

1. **Group size 64 as `export.py` would choose.** Fewer scales and faster, but `runq.c` and any faithful port drop part of `hidden_dim`: wrong output that still looks plausible.
2. **A custom DSLM int8 format (for example DSQ8 like chatds).** Could carry fixed-point scales, but loses the stock `runq.c` as an oracle.
3. **Keep fp32 as the default.** Exact against `run.c`, but slower and twice the size; fp32 stays available as the reference.

## Consequences

**Positive**

- Exact token parity with `runq.c` on the host and on melonDS; half the model size (521,728 vs 1,056,540 bytes) and faster loading.

**Negative**

- A group size of 4 means one float multiply-add per 4 integer products, so the int8 matmul only halves the soft-float work. Removing it needs fixed-point scales (M5b, `fixed-point-inference`).

**Neutral**

- int8 output differs from fp32 after 66 to 129 tokens depending on the prompt; each format has its own golden file.
