# Contract — model-file-format

**Contract:** model-file-format
**Status:** active
**Last updated:** 2026-11-26

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
/dslm/models/tiny1m_q80.bin        int8
/dslm/models/tok512.bin
/dslm/models/littlekedi_q80.bin    chat model, MLQ1 (below)
/dslm/models/littlekedi.tok        its tokenizer, BPE1 (named in the MLQ1 header)
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
chat_checkpoint = /dslm/models/littlekedi_q80.bin

[sampling]
temperature = 0.0
topp        = 0.9
steps       = 256
seed        = 0

[ui]
mode = stories
```

Unknown keys are ignored. A missing file falls back to these defaults. `checkpoint` is the story model (with `tokenizer`), `chat_checkpoint` the chat model (it names its own tokenizer), and `mode` (`stories` or `chat`) the Home card used last, whose model the DS loads at boot. An older file without `[ui]` boots on `checkpoint`; if that is a chat model, the DS treats it as the chat model and Chat mode (since change 0023).

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

Replaced on every generation. `python tools/dslm.py parity` reads it and picks the golden file named after the checkpoint. For a chat model, `prompt` is the text the engine read (chat markers included), `prompt_tokens` has no BOS, and `steps` is the cap on answer tokens.

**`/dslm/bench.txt`: one `key=value` per line, one block per run, appended**

```
version=<DSLM version, e.g. 1.0.0>
build=fp32|int8|chat-int8
emulated=unknown
checkpoint=<path from dslm.ini>
prompt=<prompt text>
steps=<int>
stopped=0|1
load_ms=<int>
tokens=<int, BOS included>
gen_ms=<int>
tok_per_s=<decimal, two places>
heap_peak_bytes=<int>
```

Blocks are separated by a blank line. The DS cannot tell an emulator from hardware, so it writes `emulated=unknown`; `python tools/dslm.py bench` sets `emulated=yes|no` (and the cart, kernel, microSD and commit header of `benchmarks/README.md`) when it imports the last block into `benchmarks/`. `heap_peak_bytes` is the heap claimed from the system at the end of the run (allocations are never returned, so it is the peak).

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

**Chat model: MLQ1 (LittleKedi, DeepSeek-V3-style MLA + MoE), `<model>_q80.bin`**

Little-endian, produced by `tools/m8/convert_littlekedi.py` (fp32 MLA1, then `--quantize`; ADR 0010).

1. Header of 128 bytes: `char magic[4] = "MLQ1"`, `int32 version = 1`, `int32` `dim`, `n_layers`, `n_dense_layers`, `n_heads`, `qk_nope_dim`, `qk_rope_dim`, `v_head_dim`, `kv_rank`, `dense_hidden`, `n_experts`, `n_active`, `moe_hidden`, `vocab`, `max_seq`; `float32` `norm_eps`, `rope_theta` (must be 10000), `route_scale`; `int32 group_size` at byte 76; at byte 80 the tokenizer's file name (same folder, no `/`), NUL-padded to 48 bytes.
2. Tensors in file order, every matrix row-major `[out][in]`. A vector is `float32[n]`; a matrix is `int8 q[rows * cols]` then `float32 s[rows * cols / group_size]`, except the expert router (`gate_w`), which stays `float32[n_experts][dim]`. Per layer: `attn_norm`, `wq`, `wkv_a`, `kv_norm`, `wkv_b`, `wo`, `ffn_norm`; dense layers `w1`, `w2`, `w3`; MoE layers `gate_w`, `gate_b`, per expert `gate`, `up`, `down`, then the shared expert `w1`, `w2`, `w3`. Then `norm` and `embed[vocab][dim]` (also the output layer).

`group_size` must divide `dim`, `kv_rank`, `n_heads * v_head_dim` and both hidden sizes (32 for LittleKedi).

**Chat tokenizer: BPE1 (GPT-2 byte-level BPE)**

`char magic[4] = "BPE1"`, `int32 vocab`, `n_merges`, `n_special`; per token id `int32 len` and its raw bytes; per special token `int32 id`, `int32 len`, its text (e.g. `<|end|>`); per merge in rank order `int32 left, right, result`. Every byte must have a single-byte token. The chat prompt is `<|user|>question<|end|><|assistant|>`; the answer ends at `<|end|>`.

The loader tells the formats apart by the first four bytes.

## References

- `specs/model-loading`
- `specs/inference-engine`
- `specs/fixed-point-inference`
- `specs/pc-toolchain`
- `specs/test-harness`
