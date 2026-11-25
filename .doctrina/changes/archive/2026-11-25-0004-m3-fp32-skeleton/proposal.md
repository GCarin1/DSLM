# Change 0004-m3-fp32-skeleton — M3 fp32 skeleton

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (uncertain)
- **Affects specs:** inference-engine, model-loading

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

M3 fp32 skeleton: port run.c forward pass, BPE tokenizer and sampler to the DS ARM9, load stories260K fully into RAM with a memory budget check, generate from a fixed prompt with streamed text, and verify token parity against the golden output on melonDS

## What

Milestone M3. `src/engine/llama2.c` ports run.c (pinned 350e04f, MIT) line for line onto buffers already in RAM: weights are used in place from the checkpoint buffer, and the run state, tokenizer, sampler and scratch live in one allocation made at init, so generation never allocates (the vocabulary is sorted at init instead of lazily). `src/main.c` becomes the skeleton: it checks that both files exist, validates the header against the file size, compares the RAM need with the free heap before allocating, reads in 32 KB chunks with progress, streams the fixed prompt `Once upon a time` and writes `/dslm/tokens.txt`. `tools/dslm.py parity` compares that file with the golden.

Results on melonDS 1.1 (emulated): 129 of 129 tokens identical to the golden at 128 steps; 1,770,985 bytes needed of 4,038,656 free; load 321 ms and 87,480 ms for 128 positions (indicative only).

## Scope boundaries

No keyboard, settings screen or cancel (M6), no `bench.txt` and no hardware numbers (M4), no ITCM/DTCM placement or fixed point (M5). The prompt is fixed in code for the skeleton.

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

- melonDS reports 4,038,656 free heap bytes; the real figure on an R4-launched binary must be measured in M4 (ds-memory-budget skill budgets about 3.5 MB).

<!-- doctrina:applied-deltas specs/inference-engine/delta.md=b5a82828,d90b5a5b,4c02e17b,7f0bd37a specs/model-loading/delta.md=56b3efd6,d90b5a5b,53154e6f,f7a50a7f -->
