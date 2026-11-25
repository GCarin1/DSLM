# Change 0003-m2-pc-toolchain — M2 PC toolchain

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (confident; signals: build)
- **Affects specs:** pc-toolchain

<!--
Optional, and usually absent. The closing docs gate reads COMMAND and FLAG
names out of the prose below and asks for documentation when it finds any.
It cannot tell a change from a mention: explaining an effect, or writing a
Scope boundaries line about what this deliberately does NOT touch, names
things just as loudly as changing them would.

When that happens, say so on the record instead of forcing the close:

- **Documented surface:** n/a — names two commands to explain an effect; alters neither

`none` reads the same as `n/a`, and a BARE one silences nothing — the
reason is the declaration.
-->

## Why

M2 PC toolchain: fetch stories260K and tok512 with pinned SHA-256, inspect the header and DS memory budget, stage the /dslm card tree, and build fp32 golden outputs from a pinned llama2.c commit

## What

Milestone M2. `tools/dslm.py` (standard library only) with four subcommands: fetch (pinned URL, size and SHA-256 in `tools/models.json`), inspect (header plus the DS RAM budget: weights, KV cache, run state, tokenizer, 32-bit pointers), stage (the /dslm tree, a default dslm.ini that is kept unless forced, and a manifest) and golden. Golden outputs come from `tools/golden/golden.c`, which includes the pinned `run.c` unchanged and prints token ids; a test proves its text equals the stock `run.c` binary. `tests/golden/stories260K.json` is committed. The contract gains `/dslm/manifest.txt`.

stories260K needs 1,744,047 bytes of the 4 MB (41.6%, excluding code and stack).

## Scope boundaries

No int8 quantization: the format is not in the contract yet and lands with fixed-point-inference (M5). The quantize requirement stays in pc-toolchain; its acceptance criterion ("runq.c reads the generated int8 checkpoint") moved to fixed-point-inference as criterion 4, where it is visibly deferred instead of cited against a skipped test. Model files are downloaded into `.cache/`, never committed. No DS-side change.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass (`doctrina verify`, or the project's typecheck/test/build).
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`).

## Open questions

- Parity in M3 compares DS fp32 (software float, picolibc `expf`/`sqrtf`) with x86-64 fp32 built with `-ffp-contract=off`. Exact token parity is expected at temperature 0 but not guaranteed; any divergence is reported by index, not hidden.

<!-- doctrina:applied-deltas specs/pc-toolchain/delta.md=98f3668f,d90b5a5b,4c02e17b,53154e6f -->
