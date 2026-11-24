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

## How much RAM DSLM actually gets

- Main RAM ends at the IPC region (`0x23FF000`). The flashcart kernel is gone once the homebrew runs; only the 16 KB DLDI area stays inside the binary.
- Practical budget: about **3.5 MB** for code, heap and stack. chatds reports a 3.65 MB heap peak with a 2.99 MB resident model.
- Extra memory: ARM7 WRAM (96 KB, owned by the ARM7), unused VRAM banks (no 8-bit writes), ITCM/DTCM (32/16 KB) for the matmul kernel and the current activation vector.

## `stories260K` memory budget (to be confirmed by `pc-toolchain`)

- Files: `stories260K.bin` is about 1.06 MB (1,056,540 bytes reported), `tok512.bin` about 5 KB.
- fp32 weights: ~1 MB. In int8: ~260 KB + scales.
- fp32 KV cache: `2 × n_layers × seq_len × kv_dim × 4 bytes`. Exact values come from the checkpoint header.
- Forward-pass buffers: tens of KB (vectors of size `dim`, `hidden_dim`, `vocab_size`, `seq_len`).

## microSD read speed through the R4

| Measurement | Value | Source quality |
|---|---|---|
| Sequential read, original R4, old libfat | ~2.7 MB/s | forum measurement |
| devkitPro libnds v2 DLDI claim | up to 5.0 MiB/s | vendor claim, cart unspecified |
| Random 16 KB reads | ~1.1 MB/s | anecdotal speed-tester run |

Takeaways: loading the model once takes well under a second; streaming weights per token would add 0.2–1 s per token (ADR 0002). Read in large blocks (32–64 KB). A faster microSD card does not help, because the DS card bus is the bottleneck.

## Reference ports

See `docs/prior-art.md`.

## Reference `llama2.c` ports (summary)

- Commodore 64: `ytmytm/llama2.c64` (stories260K, 2 MB REU)
- PSP: `sixf0ur/llama2psp`
- PS Vita: `psvita-llm`
- DOS and Windows 98: ports by Yeo Kheng Meng and others
