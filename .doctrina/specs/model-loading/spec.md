# Spec — model-loading

**Capability:** model-loading
**Status:** active
**Implementation:** partial — header check, RAM budget, 32 KB chunked reads with progress and error paths (`src/main.c`, `src/storage/`); real-R4 load and load time are M4
**Realizes:** SC1, SC5
**Depends on:** pc-toolchain
**Last updated:** 2026-11-25
**Version:** 0.4.0

## Purpose

Read the checkpoint (`model.bin`) and the tokenizer (`tok.bin`) from the R4 flashcart's microSD card into the Nintendo DS main RAM. The original `llama2.c` uses `mmap()`, which does not exist on the DS. Here the files are opened through libfat (the R4's DLDI driver), read with `fread` and kept fully in RAM (ADR 0002). This capability also owns the memory budget: before allocating, it confirms that weights, KV cache and buffers fit in the 4 MB.

## Requirements (EARS)

### Ubiquitous

- The loader shall read the checkpoint and the tokenizer from the microSD card through libfat, in the layout defined by `contracts/model-file-format`.
- The loader shall keep the whole model in main RAM; it shall compute the total memory needed (weights, KV cache, run-state buffers) from the checkpoint header before allocating.
- The loader shall read model files with fread calls of at least 32 KB each.

### Event-driven

- When the homebrew starts, the loader shall look for the model and tokenizer files in a fixed directory on the microSD card and print which files it found.
- When the checkpoint header reports a model whose total memory need exceeds the free heap, the loader shall stop and print the required and available byte counts.
- When the homebrew starts, the loader shall read the model path, tokenizer path and sampling settings from /dslm/dslm.ini.
- When FAT initialisation fails, the loader shall print a hint that the binary may need a manual DLDI patch with dlditool.

### State-driven

- While the model is being read, the loader shall show progress on screen.

### Unwanted-behavior (must-not)

- The loader shall not start inference when the FAT filesystem fails to mount or a model file is missing; it shall show the error on screen instead.
- The loader shall never read weights from the microSD card during token generation.
- The loader shall not depend on launch arguments (argv) for any setting.

### Optional

- Where more than one model is present in the model directory, the loader may let the user pick one from a list.

## Acceptance criteria

1. [unverified] stories260K.bin and tok512.bin load from the microSD card on melonDS (with a FAT/DLDI image) and on a real R4 — verified by `tests/hw/load-stories260k.md`.
2. [verified] With an oversized checkpoint (e.g. stories15M.bin), the loader refuses and shows the required and available bytes — verified by `tests/hw/oversized-model.md`.
3. [verified] With no microSD card or no files, the screen shows the error and nothing runs — verified by `tests/hw/missing-files.md`.
4. [unverified] The stories260K load time on a real R4 is measured with a DS hardware timer and recorded — verified by `tests/hw/load-time.md`.
5. [verified] The `/dslm/dslm.ini` parser applies the contract defaults for missing files, missing keys and unknown keys — verified by `tests/host/test_ini.c`
6. [verified] The hello-world `.nds` mounts the card and prints the settings read from `/dslm/dslm.ini` on melonDS — verified by `tests/hw/hello-card.md`

## Maturity

**MVP (committed):**

- Fixed directory and file names, one model at a time, full read into RAM.

**Future (aspirational, not committed):**

- On-screen model picker.
- Streaming weights from microSD for models larger than RAM, if a future ADR supersedes 0002.

## Out of scope for this spec

- The inference math (`inference-engine`, `fixed-point-inference`).
- Producing and converting the files on the PC (`pc-toolchain`).
