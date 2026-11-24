# DSLM — Product

## Vision

Run a tiny Llama 2 architecture language model entirely offline on an original Nintendo DS / DS Lite, launched from an R4 flashcart. DSLM is an experiment in inference on extremely constrained hardware (ARM9 at ~67 MHz, no FPU, 4 MB of RAM). It builds on the `llama2.c` ports that already run on the Commodore 64, DOS, PSP and PS Vita, and aims to prove the DS belongs on that list.

## Problem

There is no known port of `llama2.c` for the Nintendo DS. The console has three barriers that PC ports never face:

- no `mmap()` and no operating system;
- no floating-point unit, so every `float` is emulated in software;
- only 4 MB of main RAM, which must hold the binary, the weights, the KV cache and the buffers.

## Target users

- The project author, as a study of embedded AI and low-level C optimization.
- Nintendo DS homebrew enthusiasts and the "will it run an LLM?" community who own an R4 (or any DLDI-compatible flashcart) and want to reproduce the experiment.

## Scope

In scope:

- An `.nds` homebrew that generates text with the `stories260K` model (~260 thousand parameters, 512-token tokenizer, trained on TinyStories), reading the weights from the R4's microSD card.
- An fp32 reference build whose output, at temperature 0, is identical to `llama2.c` on a PC.
- A performance build with fixed-point / int8 inference.
- A minimal UI: prompt typed on the touch-screen keyboard and generated text streamed to the screen.
- Python PC tools to download, inspect, quantize and produce reference outputs (golden outputs) for the models.
- Testing on emulators (melonDS / DeSmuME) and on real hardware with an R4.

Out of scope (deferred or rejected):

- Models that do not fit fully in RAM, such as stories15M (~60 MB in fp32). Reading weights from the microSD card on every token is rejected for now (see ADR 0002).
- Any network or Wi-Fi use.
- Native DSi / 3DS mode (16 MB of RAM), which may become a future phase.
- A pipeline to train and export custom 100K–1M parameter models for a narrow domain. It is optional and belongs to a later phase (see Delivery order, step 6).

## Non-goals

- Being a chat assistant or following instructions. `stories260K` generates simple children's stories in English, and that is the expectation.
- Competing on text quality with PC or phone models.
- Supporting other consoles. Each port lives in its own community repository.

## Success criteria

- [SC1] `stories260K` generates text on a real Nintendo DS, launched from the R4, with no network connection.
- [SC2] At temperature 0 with the same prompt, the token sequence generated on the DS (fp32 build) is identical to the reference `llama2.c` on a PC.
- [SC3] The fixed-point / int8 build generates at least 3 tokens per second on real hardware (provisional target, to be confirmed by the first measurement).
- [SC4] The user types the prompt on the touch-screen keyboard and sees the text stream in, token by token.
- [SC5] There is a documented, reproducible path (PC scripts) to take a checkpoint in the `llama2.c` format onto the R4's microSD card.

## Delivery order (walking skeleton)

Depth before breadth. The skeleton is the shortest end-to-end path:

`pc-toolchain` (downloads the model and records the reference output) → `model-loading` (reads from microSD) → `inference-engine` (fp32 forward pass with a fixed prompt) → text on the DS console → comparison with the reference output.

1. **Environment.** Toolchain installed, a "hello world" `.nds` running on melonDS and on the R4, and a file read from the microSD card through libfat.
2. **fp32 skeleton.** `stories260K` loaded into RAM, a hard-coded prompt, text printed to the console. First on melonDS, then on the R4 (SC1).
3. **Parity.** Temperature-0 output identical to the PC (SC2), checked against the `pc-toolchain` golden outputs.
4. **Fixed point / int8.** Performance build, tokens/s measured on real hardware (SC3), approximate parity with the reference.
5. **Interface.** On-screen keyboard, streaming and basic controls (SC4).
6. **(Optional) Custom models.** Train and export 100K–1M parameter models for a narrow domain with the `llama2.c` scripts.
