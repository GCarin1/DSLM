# Change 0013-test-harness — Test harness

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (uncertain)
- **Affects specs:** test-harness

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

Test harness for v1.0: the DS appends /dslm/bench.txt with version, timings and heap peak; tools/dslm.py bench imports it into benchmarks/; tests/parity/run_melonds.py runs an unattended melonDS parity check; DeSmuME moves to Future

## What

Completes the test harness for v1.0. The DS appends a block per run to /dslm/bench.txt (version, build, emulated=unknown, checkpoint, prompt, steps, stopped, load_ms, tokens, gen_ms, tok_per_s, heap_peak_bytes); the heap peak is the heap claimed at the end of the run. `tools/dslm.py bench` imports the last block into benchmarks/ with the cart/kernel/microSD/commit header and refuses emulator runs. `tests/parity/run_melonds.py` runs melonDS unattended (key posted to its window, normal close) and checks parity: fp32 129/129 in 87.4 s, heap peak 1,773,880 bytes; int8 129/129 in 16.5 s, heap peak 1,372,472 bytes (emulated). DSLM_VERSION 1.0.0 shows at boot and in the ROM title. DeSmuME moves to Future (spec rewritten, criteria 1 and 2 replaced).

## Scope boundaries

The hardware bench run with heap peak (criterion 3) needs the owner's DS; it is the v1.0 release check.

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

<!-- doctrina:applied-deltas specs/test-harness/delta.md=67d63186,d90b5a5b,1ddc9a8e,31742c8f,3e7779a6,bfa7a63c,3eb46bb7 -->
