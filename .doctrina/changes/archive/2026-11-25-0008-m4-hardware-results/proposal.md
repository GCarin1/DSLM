# Change 0008-m4-hardware-results — M4 hardware results

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (confident; signals: record)
- **Affects specs:** model-loading

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

M4 hardware results: record the first real R4 run (fp32 parity against run.c, load time and time per position) in benchmarks/ and the hardware runbooks

## What

Milestone M4. The user ran the install package on a real DS with an R4 clone (YSMenu kernel) and typed the prompt `hi`. The card's `/dslm/tokens.txt` matches `run.c` on the PC token for token (153/153). Measured on the DS with its hardware timers: model load 249 ms, 627.5 ms per position in fp32 (152 positions, stopped with B). The numbers go to `benchmarks/2026-11-25-r4-ysmenu.txt` and the hardware runbooks; the inference-engine and model-loading hardware criteria are met.

## Scope boundaries

No code change. `/dslm/bench.txt` and a heap-peak measurement are not implemented yet, so the benchmark comes from `tokens.txt` and records the heap peak as not measured. The cart's label domain, the DS model and the microSD brand were not recorded.

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

- The card reader reports a 64 GB disk with a 3.8 GB partition; if the card is counterfeit, larger writes may fail. Worth checking with a capacity test tool.

<!-- doctrina:applied-deltas specs/inference-engine/delta.md=20e13564,e7c38c12,53154e6f,f7a50a7f specs/model-loading/delta.md=603ed858,e7c38c12,4c02e17b,741d37e9 -->
