# ADR 0002 — Load the whole model into the 4 MB main RAM

- **Status:** proposed
- **Date:** 2026-11-24
- **Deciders:** Carini
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** n/a — no implementation yet
- **Landed:** —

## Context

The R4 launches homebrew in DS mode, with 4 MB of main RAM. That space must hold the ARM9 binary, the libnds heap, the weights, the KV cache and the forward-pass buffers. `stories260K` is about 1 MB in fp32. The KV cache grows with `n_layers * seq_len * kv_dim`. `stories15M` is about 60 MB in fp32 or 15 MB in int8 and does not fit. Reading from the microSD card through the flashcart is orders of magnitude slower than RAM. Reading weights from the card on every token would make each token very slow.

## Decision

The whole model (weights + KV cache + buffers) lives in main RAM, loaded once at startup. `model-loading` computes the budget from the header and refuses models that do not fit. The initial target is `stories260K`.

## Alternatives considered

1. **Streaming weights from microSD per layer/token.** Would allow larger models, but per-token latency would be impractical. May be revisited in a future ADR.
2. **DSi mode (16 MB).** Requires a DSi/3DS and a loader that enables DSi mode. Out of the initial scope, possibly a future phase.
3. **Shrinking `seq_len` at runtime.** Reduces the KV cache and can be used as fine-tuning, but does not change the main decision.

## Consequences

**Positive**

- Inference with no I/O during generation. Time per token depends only on CPU and RAM.

**Negative**

- The practical model ceiling is around 1–3 MB of weights, depending on `seq_len` and format.

**Neutral**

- The memory budget becomes a number `pc-toolchain` can check before copying to the card.
