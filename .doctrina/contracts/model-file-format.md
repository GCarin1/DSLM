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
| Practical heap + code + stack for a flashcart-launched homebrew (ends at the IPC region, `0x23FF000`) | input | about 3.5 MB; measure on hardware |
| DLDI driver area reserved in the `.nds` (patched in by the kernel or `dlditool`) | input | 16384 bytes |
| Token length in the tokenizer (`max_token_length`) | input | defined in the `tok.bin` header |

## Interfaces

**Layout of the microSD card**

```
/                     kernel files (e.g. __rpg/, _DS_MENU.DAT); never shipped by DSLM
/dslm.nds             the homebrew (may also live under /apps/)
/dslm/dslm.ini        settings (below)
/dslm/models/stories260K_q80.bin   int8 (default)
/dslm/models/stories260K.bin       fp32 reference
/dslm/models/tok512.bin
/dslm/bench.txt       written by the harness after each run
/dslm/tokens.txt      written by the homebrew after each generation (below)
/dslm/manifest.txt    written by pc-toolchain: "<card path> <size> <sha256>" per staged file
```

The same tree, minus the kernel files, is the melonDS "Sync SD to folder" directory. `pc-toolchain` stages it.

**`/dslm/dslm.ini`: settings (no argv)**

```ini
[model]
checkpoint = /dslm/models/stories260K_q80.bin
tokenizer  = /dslm/models/tok512.bin

[sampling]
temperature = 0.0
topp        = 0.9
steps       = 256
seed        = 0
```

Unknown keys are ignored. A missing file falls back to these defaults.

**`/dslm/tokens.txt`: the last generation, one `key=value` per line**

```
checkpoint=<path from dslm.ini>
prompt=<prompt text>
steps=<int>
prompt_tokens=<int, BOS included>
tokens=<space-separated token ids, BOS first>
stopped=0|1
load_ms=<int>
gen_ms=<int>
```

Replaced on every generation. `python tools/dslm.py parity` reads it and picks the golden file named after the checkpoint.

**`/dslm/bench.txt`: one `key=value` per line, one block per run**

```
build=fp32|int8
emulated=yes|no
load_ms=<int>
tokens=<int>
tok_per_s=<float>
heap_peak_bytes=<int>
```

**fp32 checkpoint: legacy `llama2.c` format (read by `run.c`)**

Little-endian. The DS ARM9 is little-endian too, so no conversion is needed.

1. Header of 7 × `int32`: `dim`, `hidden_dim`, `n_layers`, `n_heads`, `n_kv_heads`, `vocab_size`, `seq_len`. A negative `vocab_size` means the final classifier does not share weights with the embedding (use the absolute value).
2. `float32` weights, in this order: `token_embedding_table`, `rms_att_weight`, `wq`, `wk`, `wv`, `wo`, `rms_ffn_weight`, `w1`, `w2`, `w3`, `rms_final_weight`, two legacy RoPE tables (`seq_len * head_size / 2` floats each, skipped) and, if not shared, `wcls`.

**Tokenizer: `llama2.c` format**

1. `int32 max_token_length`.
2. For each of the `vocab_size` tokens: `float32 score`, `int32 len`, `len` bytes of text (no terminator).

**int8 checkpoint: `llama2.c` v2 format (read by `runq.c`), `<model>_q80.bin`**

Little-endian, produced by `tools/dslm.py quantize` from the fp32 checkpoint (ADR 0007).

1. Header of 256 bytes, zero-padded: `uint32 magic = 0x616b3432` ("ak42"), `int32 version = 2`, the 7 × `int32` config (positive `vocab_size`), `uint8 shared_classifier`, `int32 group_size` (at byte 37).
2. `float32` norms: `rms_att_weight` (all layers), `rms_ffn_weight` (all layers), `rms_final_weight`. No RoPE tables.
3. Quantized tensors, each as `int8 q[n]` followed by `float32 s[n / group_size]` (`q ≈ w / s`, `s = max|w| / 127` per group of `group_size` consecutive values): `token_embedding_table`, then per layer `wq`, then per layer `wk`, `wv`, `wo`, `w1`, `w2`, `w3`, then `wcls` if not shared.

`group_size` must divide both `dim` and `hidden_dim`: `runq.c`'s matmul only walks whole groups and would drop the rest. For stories260K (dim 64, hidden 172) it is 4.

The loader tells the formats apart by the first four bytes.

## References

- `specs/model-loading`
- `specs/inference-engine`
- `specs/fixed-point-inference`
- `specs/pc-toolchain`
- `specs/test-harness`
