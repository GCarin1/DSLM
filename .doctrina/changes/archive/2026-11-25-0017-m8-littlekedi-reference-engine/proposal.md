# Change 0017-m8-littlekedi-reference-engine — M8 LittleKedi reference engine

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (uncertain)
- **Affects specs:** inference-engine

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

M8 part 1: LittleKedi (DeepSeek-V3-style MLA + MoE, GPT-2 BPE) converter and an fp32 C reference engine on the PC, validated token for token against llama.cpp

## What

Step 1 of M8 (the owner chose to port LittleKedi despite its weak answers). A numpy oracle reproduces llama.cpp's greedy output on the F16 GGUF once llama.cpp's GGUF-provided repeat penalty is off (activations agree to about 1e-3 layer by layer; earlier divergences were that penalty). A converter writes MLA1 (fp32, experts transposed to [out][in]) and BPE1 (raw bytes, chat markers, merges); the files are pinned at a repo revision. The fp32 C engine (absorbed MLA, interleaved RoPE, sigmoid top-2 experts + shared expert) and the GPT-2 BPE tokenizer match the oracle token for token on five prompts. docs/littlekedi.md records the design and the next steps.

## Scope boundaries

No DS code path yet: no fixed point, no chat mode, no packaging. The engine files compile into dslm.nds but nothing calls them.

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

<!-- doctrina:applied-deltas specs/inference-engine/delta.md=d90b5a5b,52670f54,e8f3d8ac -->
