# Change 0012-m5-hardware-results — M5 hardware results

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (confident; signals: build, record)
- **Affects specs:** fixed-point-inference

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

M5 hardware results: record the first int8 fixed-point run on the reference R4 (6.55 tokens/s, 257/257 token parity against the host build) in benchmarks/ and the runbooks

## What

Closes M5 on hardware. The owner ran the M5b package on the reference R4 (ADR 0009): `Once upon a time`, 256 steps, finished. The card's `/dslm/tokens.txt`: load 124 ms, 256 positions in 39,080 ms, 152.7 ms per position (6.55 tokens/s, 4.1x the fp32 baseline of 627.5 ms). All 257 token ids match the host build of the engine (the first 129 also match the committed golden). SC3 (>= 3 tokens/s on hardware) is met. Recorded in `benchmarks/2026-11-25-r4-ysmenu-int8.txt`, the int8 benchmark and parity runbooks, and the README roadmap.

## Scope boundaries

No code change. Heap peak still not measured (no bench.txt yet).

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

<!-- doctrina:applied-deltas specs/fixed-point-inference/delta.md=44666dad,e7c38c12,4c02e17b -->
