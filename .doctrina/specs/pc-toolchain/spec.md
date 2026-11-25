# Spec — pc-toolchain

**Capability:** pc-toolchain
**Status:** active
**Implementation:** verified — `tools/dslm.py` (fetch, inspect, quantize, stage, golden, parity, package, bench); packages verified on melonDS and the reference R4
**Realizes:** SC2, SC5
**Last updated:** 2026-11-25
**Version:** 0.8.0

## Purpose

Python scripts that run on the PC and prepare everything that goes onto the R4's microSD card. They download the `stories260K` checkpoint and `tok512` tokenizer, inspect the header and compute memory use, build the int8 quantized checkpoint and produce the golden outputs (reference token sequences from `llama2.c`/`runq.c` at temperature 0). The parity tests compare the DS against these golden outputs.

## Requirements (EARS)

### Ubiquitous

- The toolchain shall produce files only in the layouts defined by `contracts/model-file-format`.
- The toolchain shall pin the `llama2.c` commit it uses to build golden outputs.
- The toolchain shall record the size in bytes and the SHA-256 of every staged model and tokenizer file.
- The toolchain shall choose an int8 group size that divides both dim and hidden_dim.
- The toolchain shall generate the fixed-point constant tables from a script and fail verification when the committed header is stale.
- The toolchain shall put DSLM's own license (LICENSE, MIT) in every install package next to the third-party notices.
- The toolchain shall pin every packaged model (URL, size, SHA-256, author, license) in tools/models.json and ship each as int8, plus fp32 only when it fits the DS.

### Event-driven

- When asked to inspect a checkpoint, the toolchain shall print the header fields and the RAM the DS needs for weights, KV cache and run-state buffers.
- When asked to build golden outputs, the toolchain shall run the reference `llama2.c` binary with temperature 0 on a fixed prompt list and save the token ids and text for each prompt.
- When asked to quantize, the toolchain shall convert an fp32 checkpoint into the int8 format and report the size before and after.
- When asked to stage a card, the toolchain shall write the /dslm/ tree (models, tokenizer and dslm.ini) into a target directory that works both as the melonDS SD sync folder and as the root of the R4 microSD card.
- When asked to package, the toolchain shall write a zip with dslm.nds, the /dslm tree with the pinned model and tokenizer, a user README, a manifest and the third-party license notices, laid out to unzip at the microSD root.
- When asked to build golden outputs for the int8 model, the toolchain shall run the pinned runq.c on the quantized checkpoint and save them separately from the fp32 golden outputs.
- When asked for the exact int8 DS reference, the toolchain shall build the DS engine for the host and save its outputs as the int8 golden, keeping the runq.c outputs as a separate quality reference.
- When a version tag is pushed, the release workflow shall run the tests, build dslm.nds and publish the install package, the standalone .nds and their SHA-256 sums with the release notes for that version.

### Unwanted-behavior (must-not)

- The toolchain shall not overwrite an existing golden output unless explicitly told to.
- The toolchain shall not put flashcart kernels or DLDI driver files in a package.

### Optional

- Where a training dataset is provided, the toolchain may train and export a custom model with the `llama2.c` training scripts.

## Acceptance criteria

1. [verified] Inspecting `stories260K.bin` shows the header and a memory budget below 4 MB — verified by `tools/tests/test_inspect.py`.
2. [verified] Golden outputs built twice from the same `llama2.c` commit are identical — verified by `tools/tests/test_golden.py`.
3. [verified] The staged directory boots in melonDS (DLDI sync folder) and, copied as-is, on a real R4 — verified by `tools/tests/test_stage.py`.
4. [verified] The package has the card-root layout, the licenses and no kernel or DLDI files, and two builds are byte-identical — verified by `tools/tests/test_package.py`
5. [verified] The unzipped package alone boots and generates on a fresh melonDS card folder — verified by `tests/hw/package-install.md`
6. [verified] The release workflow's steps run end to end in the pinned BlocksDS image and produce the three assets — verified by `.github/workflows/release.yml`
7. [verified] The install package carries stories260K (int8 and fp32) and tiny1m (int8) with a notice per model — verified by `tools/tests/test_package.py`

## Maturity

**MVP (committed):**

- Download, inspection, fp32 golden outputs and int8 quantization of `stories260K`.

**Future (aspirational, not committed):**

- Training custom 100K–1M parameter models for a narrow domain (phase 6 of the Delivery order).

## Out of scope for this spec

- Any code that runs on the DS.
