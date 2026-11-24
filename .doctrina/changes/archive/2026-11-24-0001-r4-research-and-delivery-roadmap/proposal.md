# Change 0001-r4-research-and-delivery-roadmap — R4 research and delivery roadmap

- **Status:** applied
- **Applied:** 2026-11-24
- **Date:** 2026-11-24
- **Owner:** Carini
- **Lane:** docs (planning only; no runtime code exists yet)
- **Affects specs:** pc-toolchain, model-loading, test-harness, model-loading

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

Incorporate the R4 flashcart and prior-art research into the plan: BlocksDS toolchain, R4 hardware and kernel policy, microSD layout with a config file, emulator-first then hardware test harness, delivery roadmap

## What

- New capability `test-harness` (ADDED): emulator-first, hardware-second testing and benchmarking; realizes the new intent anchor SC6 (real-hardware benchmarks).
- `model-loading` (MODIFIED): settings from `/dslm/dslm.ini` instead of argv, large-block reads, DLDI failure hint, load-time criterion.
- `pc-toolchain` (MODIFIED): card staging for melonDS and the R4, file size and SHA-256 records.
- ADR 0004 (BlocksDS toolchain) and ADR 0005 (R4 hardware target and kernel policy), both proposed. ADRs 0001 and 0002 context updated with prior art and measured figures (still proposed).
- Contract `model-file-format`: microSD layout, `dslm.ini`, `bench.txt`, RAM and DLDI budgets.
- `product.md`: prior art in Problem, SC6, delivery order as milestones M0–M8.
- Docs: `r4-flashcart.md`, `testing.md`, `prior-art.md`, `risks.md`, updated `hardware-ds.md`; `benchmarks/README.md`.
- Skills: `r4-deploy`, `emulator-testing`, `ds-memory-budget`.
- README roadmap; AGENTS.md conventions; `verify.json` documentation checks; docs-gates GitHub Actions workflow; git hooks.

## Scope boundaries

- No firmware, tool or test code. Every spec stays at Implementation: planned.
- No kernels, firmware or DLDI binaries enter the repository.
- ADRs are not accepted here; acceptance is the owner's call.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass (`doctrina verify`: clarify, trace, local links).
- [x] Every new or modified acceptance criterion cites its future evidence path and is reported as deferred by `doctrina coverage`.

## Open questions

- Accept ADRs 0001–0005 as written?
- SC3 target (≥ 3 tok/s) stays provisional until M4/M5 measurements.
- Project license (MIT suggested).

<!-- doctrina:applied-deltas specs/model-loading/delta.md=717d980b,2f183fb9,9611b99a,a897f973,e9ff6c75,d90b5a5b specs/pc-toolchain/delta.md=623d3cd0,0d5d326c,477f7f29,d90b5a5b specs/test-harness/delta.md=text:af45c40e -->
