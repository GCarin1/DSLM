# Nintendo DS: hardware notes for inference

A quick reference of the DS / DS Lite resources in DS mode, which is how the R4 launches homebrew, and what each one means for DSLM. Decisions live in the ADRs under `.doctrina/decisions/`. This file only gathers the context.

| Resource | Value | Impact on the project |
|---|---|---|
| Main CPU | ARM946E-S (ARMv5TE) @ ~67 MHz | Runs the whole forward pass |
| FPU | **None** | `float` becomes a software call → slow fp32 build (ADR 0003) |
| DSP instructions | `SMULxy`, `SMLAxy`, `QADD`/`QSUB` | 16×16 MAC in one instruction → basis of the fixed-point matmul |
| Hardware divider / sqrt | Memory-mapped registers (`REG_DIV*`, `REG_SQRT*`) | RMSNorm (sqrt) and softmax normalization (division) |
| Caches | 8 KB instruction / 4 KB data | Weights do not fit in cache, so the forward pass is RAM-bound |
| ITCM / DTCM | 32 KB / 16 KB | Matmul loop in ITCM, accumulators and small buffers in DTCM |
| Secondary CPU | ARM7TDMI @ ~33 MHz | Input, touch, audio. Could help with compute later |
| Main RAM | 4 MB (16-bit bus) | Ceiling for binary + weights + KV cache + buffers (ADR 0002) |
| Screens | 2 × 256×192, bottom one is touch | Text on top, keyboard on the bottom (`ds-interface`) |
| Storage | microSD in the R4, via libfat + DLDI | Weights read once at startup (`model-loading`) |

## `stories260K` memory budget (estimate, to be confirmed by `pc-toolchain`)

- fp32 weights: ~1 MB (~260 thousand parameters × 4 bytes). In int8: ~260 KB + scales.
- fp32 KV cache: `2 × n_layers × seq_len × kv_dim × 4 bytes`. Exact values come from the checkpoint header.
- Forward-pass buffers: tens of KB (vectors of size `dim`, `hidden_dim`, `vocab_size`, `seq_len`).

## Reference `llama2.c` ports

- Commodore 64: `ytmytm/llama2.c64` (stories260K, 2 MB REU)
- PSP: `sixf0ur/llama2psp`
- PS Vita: `psvita-llm`
- DOS and Windows 98: ports by Yeo Kheng Meng and others
