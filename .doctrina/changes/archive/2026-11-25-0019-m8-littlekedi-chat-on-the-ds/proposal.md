# Change 0019-m8-littlekedi-chat-on-the-ds — M8 LittleKedi chat on the DS

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** capability
- **Affects specs:** ds-interface, model-loading

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

M8 step 3: LittleKedi in the DS app - load MLQ1 models with their BPE tokenizer, chat mode in the UI (wrap prompt in chat markers, stop at <|end|>, repeat penalty), picker integration, package with Apache-2.0 notice, melonDS parity

## What

Step 3 of M8. The DS app loads MLQ1 chat models: the header names the tokenizer file (same folder), the context is sized from the RAM left after the files, the Settings picker lists the model as chat. Chat mode wraps the typed question in the model's markers, shows it above the answer and stops at the end marker, with the author's repeat penalty always on; the generation loop (src/engine/mlq_chat.c) is shared with the host driver so a DS run matches the golden exactly. The package ships littlekedi_q80.bin and littlekedi.tok with the Apache License 2.0. melonDS parity 40/40 (tests/parity/littlekedi-chat.md). Contract model-file-format gains MLQ1 and BPE1.

## Scope boundaries

No multi-turn memory (each question is a new chat), no release and no real-hardware run yet (the owner tests on the DS later).

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

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=d90b5a5b,7b4947ca,25aa5e85 specs/model-loading/delta.md=d90b5a5b,65d54c13,88197109 -->
