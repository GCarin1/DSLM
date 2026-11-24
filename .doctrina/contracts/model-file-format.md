# Contract — model-file-format

**Contract:** model-file-format
**Status:** active
**Last updated:** 2026-11-24

## Purpose

The seam between the PC and the DS is the set of files that go onto the microSD card. `pc-toolchain` produces them; `model-loading` and the inference engines consume them. This contract defines the binary layout and the location of the files on the card, so the two sides cannot drift apart. The project has no services, ports, environment variables or pipeline test selectors, so those tables do not apply.

## Ports

Not applicable: offline homebrew, no network.

## Environment

Not applicable: there are no environment variables on the DS.

## Wiring

Not applicable.

## Selectors

Not applicable: hardware tests are manual scripts under `tests/hw/`.

## Budgets

| Limit | Direction | Value |
|-------|-----------|-------|
| DS main RAM in DS mode (binary + weights + KV cache + buffers) | input | 4194304 bytes |
| Token length in the tokenizer (`max_token_length`) | input | defined in the `tok.bin` header |

## Interfaces

**Location on the microSD card**

- `/dslm/models/<name>.bin`: checkpoint.
- `/dslm/models/tok<vocab>.bin`: tokenizer (e.g. `tok512.bin` for `stories260K`).

**fp32 checkpoint: legacy `llama2.c` format (read by `run.c`)**

Little-endian. The DS ARM9 is little-endian too, so no conversion is needed.

1. Header of 7 × `int32`: `dim`, `hidden_dim`, `n_layers`, `n_heads`, `n_kv_heads`, `vocab_size`, `seq_len`. A negative `vocab_size` means the final classifier does not share weights with the embedding (use the absolute value).
2. `float32` weights, in this order: `token_embedding_table`, `rms_att_weight`, `wq`, `wk`, `wv`, `wo`, `rms_ffn_weight`, `w1`, `w2`, `w3`, `rms_final_weight`, two legacy RoPE tables (`seq_len * head_size / 2` floats each, skipped) and, if not shared, `wcls`.

**Tokenizer: `llama2.c` format**

1. `int32 max_token_length`.
2. For each of the `vocab_size` tokens: `float32 score`, `int32 len`, `len` bytes of text (no terminator).

**int8 checkpoint: to be defined**

The quantized format for `fixed-point-inference` has not been chosen yet. The candidate is `runq.c`'s v2 format (magic `ak42`, version, header, `group_size`, int8 weights with per-group `float32` scales), with the scales converted to fixed point at load or export time. It becomes a change to this contract when the capability is implemented.

## References

- `specs/model-loading`
- `specs/inference-engine`
- `specs/fixed-point-inference`
- `specs/pc-toolchain`
