<p align="center">
  <img src="assets/dslm-logo.png" alt="DSLM — Run LLMs on Nintendo DS" width="420">
</p>

# DSLM

A tiny LLM running **100% offline on a Nintendo DS**, launched from an **R4** flashcart. DSLM is a port of [`llama2.c`](https://github.com/karpathy/llama2.c) running the `stories260K` model, which writes short, simple stories in English.

> **Status:** phase 0, documentation and planning only. No code yet.

## Why it's hard

- **No FPU:** the ~67 MHz ARM9 emulates every `float` in software.
- **4 MB of RAM:** the binary, weights, KV cache and buffers all share it.
- **No OS:** no `mmap()`, no threads. Weights are read from the microSD card through libfat.

## How the project is organized

DSLM uses [Doctrina](https://github.com/GCarin1/Doctrina) for spec-driven development:

| Where | What |
|---|---|
| [`.doctrina/product.md`](.doctrina/product.md) | Vision, scope, success criteria (SC1–SC5) and **delivery order** |
| [`.doctrina/specs/`](.doctrina/specs/) | Capabilities: `model-loading`, `inference-engine`, `fixed-point-inference`, `ds-interface`, `pc-toolchain` |
| [`.doctrina/contracts/model-file-format.md`](.doctrina/contracts/model-file-format.md) | Binary layout of the files that go onto the microSD card |
| [`.doctrina/decisions/`](.doctrina/decisions/) | ADRs 0001–0003 (llama2.c, whole model in RAM, fixed point + fp32 reference) |
| [`docs/hardware-ds.md`](docs/hardware-ds.md) | DS hardware notes and memory budget |
| [`AGENTS.md`](AGENTS.md) | Instructions for AI agents |

## Roadmap (summary, details in `product.md`)

1. Environment: toolchain, hello world on melonDS and the R4, reading from microSD
2. fp32 skeleton: `stories260K` generating text from a fixed prompt
3. Parity with `llama2.c` on the PC (temperature 0)
4. int8 / fixed-point build and tokens/s measurement
5. Interface with on-screen keyboard and streaming
6. (Optional) training custom models

## Open questions

- Toolchain: **devkitPro** or **BlocksDS**?
- SC3 performance target: currently **≥ 3 tokens/s**, a provisional value.
- int8 checkpoint format: reuse `runq.c`'s v2 format or define our own?
- ADRs 0001–0003 are `proposed` and awaiting acceptance.

## Documentation gates

```bash
npm install
npx doctrina validate && npx doctrina clarify --all && npx doctrina trace
```

## Credits

Built on [`llama2.c`](https://github.com/karpathy/llama2.c) by Andrej Karpathy (MIT) and inspired by the C64, PSP, PS Vita and DOS ports.
