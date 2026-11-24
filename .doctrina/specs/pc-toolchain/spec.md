# Spec — pc-toolchain

**Capability:** pc-toolchain
**Status:** active
**Implementation:** planned — current phase is documentation and planning only; no code yet
**Realizes:** SC2, SC5
**Last updated:** 2026-11-24
**Version:** 0.1.0

## Purpose

Python scripts that run on the PC and prepare everything that goes onto the R4's microSD card. They download the `stories260K` checkpoint and `tok512` tokenizer, inspect the header and compute memory use, build the int8 quantized checkpoint and produce the golden outputs (reference token sequences from `llama2.c`/`runq.c` at temperature 0). The parity tests compare the DS against these golden outputs.

## Requirements (EARS)

### Ubiquitous

- The toolchain shall produce files only in the layouts defined by `contracts/model-file-format`.
- The toolchain shall pin the `llama2.c` commit it uses to build golden outputs.

### Event-driven

- When asked to inspect a checkpoint, the toolchain shall print the header fields and the RAM the DS needs for weights, KV cache and run-state buffers.
- When asked to build golden outputs, the toolchain shall run the reference `llama2.c` binary with temperature 0 on a fixed prompt list and save the token ids and text for each prompt.
- When asked to quantize, the toolchain shall convert an fp32 checkpoint into the int8 format and report the size before and after.

### Unwanted-behavior (must-not)

- The toolchain shall not overwrite an existing golden output unless explicitly told to.

### Optional

- Where a training dataset is provided, the toolchain may train and export a custom model with the `llama2.c` training scripts.

## Acceptance criteria

1. [unverified] Inspecting `stories260K.bin` shows the header and a memory budget below 4 MB — verified by `tools/tests/test_inspect.py`.
2. [unverified] Golden outputs built twice from the same `llama2.c` commit are identical — verified by `tools/tests/test_golden.py`.
3. [unverified] The generated int8 checkpoint is read by the reference `runq.c` — verified by `tools/tests/test_quantize.py`.

## Maturity

**MVP (committed):**

- Download, inspection, fp32 golden outputs and int8 quantization of `stories260K`.

**Future (aspirational, not committed):**

- Training custom 100K–1M parameter models for a narrow domain (phase 6 of the Delivery order).

## Out of scope for this spec

- Any code that runs on the DS.
