# Spec — pc-toolchain

**Capability:** pc-toolchain
**Status:** active
**Implementation:** partial — fetch, inspect, stage, fp32 golden outputs, parity and install package (`tools/dslm.py`); int8 quantization waits for the format in M5
**Realizes:** SC2, SC5
**Last updated:** 2026-11-25
**Version:** 0.4.0

## Purpose

Python scripts that run on the PC and prepare everything that goes onto the R4's microSD card. They download the `stories260K` checkpoint and `tok512` tokenizer, inspect the header and compute memory use, build the int8 quantized checkpoint and produce the golden outputs (reference token sequences from `llama2.c`/`runq.c` at temperature 0). The parity tests compare the DS against these golden outputs.

## Requirements (EARS)

### Ubiquitous

- The toolchain shall produce files only in the layouts defined by `contracts/model-file-format`.
- The toolchain shall pin the `llama2.c` commit it uses to build golden outputs.
- The toolchain shall record the size in bytes and the SHA-256 of every staged model and tokenizer file.

### Event-driven

- When asked to inspect a checkpoint, the toolchain shall print the header fields and the RAM the DS needs for weights, KV cache and run-state buffers.
- When asked to build golden outputs, the toolchain shall run the reference `llama2.c` binary with temperature 0 on a fixed prompt list and save the token ids and text for each prompt.
- When asked to quantize, the toolchain shall convert an fp32 checkpoint into the int8 format and report the size before and after.
- When asked to stage a card, the toolchain shall write the /dslm/ tree (models, tokenizer and dslm.ini) into a target directory that works both as the melonDS SD sync folder and as the root of the R4 microSD card.
- When asked to package, the toolchain shall write a zip with dslm.nds, the /dslm tree with the pinned model and tokenizer, a user README, a manifest and the third-party license notices, laid out to unzip at the microSD root.

### Unwanted-behavior (must-not)

- The toolchain shall not overwrite an existing golden output unless explicitly told to.
- The toolchain shall not put flashcart kernels or DLDI driver files in a package.

### Optional

- Where a training dataset is provided, the toolchain may train and export a custom model with the `llama2.c` training scripts.

## Acceptance criteria

1. [verified] Inspecting `stories260K.bin` shows the header and a memory budget below 4 MB — verified by `tools/tests/test_inspect.py`.
2. [verified] Golden outputs built twice from the same `llama2.c` commit are identical — verified by `tools/tests/test_golden.py`.
3. [unverified] The staged directory boots in melonDS (DLDI sync folder) and, copied as-is, on a real R4 — verified by `tools/tests/test_stage.py`.
4. [verified] The package has the card-root layout, the licenses and no kernel or DLDI files, and two builds are byte-identical — verified by `tools/tests/test_package.py`
5. [verified] The unzipped package alone boots and generates on a fresh melonDS card folder — verified by `tests/hw/package-install.md`

## Maturity

**MVP (committed):**

- Download, inspection, fp32 golden outputs and int8 quantization of `stories260K`.

**Future (aspirational, not committed):**

- Training custom 100K–1M parameter models for a narrow domain (phase 6 of the Delivery order).

## Out of scope for this spec

- Any code that runs on the DS.
