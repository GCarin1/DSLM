# Intake — DSLM

- **Status:** converted
- **Date:** 2026-11-24
- **Source:** ../../../tmp/dslm-intake.md

<!-- Raw project intent, stored verbatim. The bootstrap playbook
     (doctrina intake) converts it into product.md and capability
     specs. Close it with `doctrina intake --converted`; after that the
     specs are the only source of truth; never edit this file to
     change requirements. -->

---

# DSLM — Intake

Run a tiny language model (LLM) 100% offline on an original Nintendo DS / DS Lite, launched from an R4 flashcart (`.nds` homebrew + weights on the microSD card).

## Context

- Technical base: a port of `llama2.c` (Andrej Karpathy) — Llama 2 architecture inference in pure C, no dependencies.
- Prior art: llama2.c has been ported to the Commodore 64 (with a 2 MB REU), DOS, Windows 98, PSP, PS Vita and Xbox 360. There is no known port for the Nintendo DS.
- Initial target model: `stories260K` (~260 thousand parameters, 512-token tokenizer), trained on the TinyStories dataset. It generates simple children's stories in English — it is not a chat assistant.

## Hardware constraints (Nintendo DS / DS Lite in DS mode)

- Main CPU: ARM946E-S @ ~67 MHz, **no FPU** (floating point only emulated in software).
- Secondary CPU: ARM7TDMI @ ~33 MHz (audio, Wi-Fi, input, touch).
- Main RAM: 4 MB (the DSi has 16 MB, but the R4 runs in DS mode).
- Small caches, 32 KB ITCM and 16 KB DTCM — useful for hot loops (matmul).
- Storage: microSD through the R4 flashcart, accessed with libfat (DLDI).
- Two 256x192 screens, the bottom one is a touch screen — text output and on-screen keyboard.

## Goals

1. Generate text with `stories260K` on real hardware, offline, with output identical (temperature 0) to the reference `llama2.c` on a PC.
2. A fixed-point / int8 inference build for acceptable speed (target: a few tokens/s).
3. Minimal UI: prompt typed on the touch-screen keyboard, generated text streamed to the screen.
4. (Optional) A pipeline to train and export custom 100K–1M parameter models for a narrow domain.

## Out of scope

- Large models (e.g. stories15M in fp32 = ~60 MB) loaded fully into RAM.
- Any network / Wi-Fi dependency.
- Native DSi / 3DS mode support (may become a future phase).

## Stack

- Toolchain: devkitPro (devkitARM + libnds + libfat) or BlocksDS.
- Firmware language: C (C99/C11).
- PC tools (training/export/quantization/verification): Python + PyTorch, scripts from the llama2.c repository.
- Testing: melonDS / DeSmuME emulators and real hardware with an R4.

## Current phase

Documentation and planning only. No code yet.
