# Change 0010-m5b-fixed-point-engine — M5b fixed-point engine

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (confident; signals: build)
- **Affects specs:** fixed-point-inference, pc-toolchain

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

M5b fixed-point int8 engine: integer-only forward pass (Q16 activations, Q30 weight scales, int64 accumulation, table exp, integer sin/cos and rsqrt), ARM-mode kernels with the matmul in ITCM, deterministic so the DS matches the host build bit for bit, drift against runq.c measured

## What

Second half of M5 (ADR 0008). The int8 model now runs an integer-only engine: Q16 activations, Q30 weight scales converted in place at load, Q24 activation scales, int32/int64 dot products, and integer exp, sin/cos and rsqrt with compiled-in tables (no libm). Engine and math compile in ARM mode; matmul_fx sits in ITCM. Because nothing after load is float, the DS matches the host build bit for bit, which becomes the exact int8 golden; runq.c's output becomes the quality reference with a drift bound of 64 generated tokens (measured 82 to 128). tools/check_softfloat.py proves from the binary that the per-token path reaches no soft-float routine. melonDS: 16.5 s for 128 positions vs 69.6 s (float int8) and 87.5 s (fp32). The story screen shows tokens per second.

## Scope boundaries

No hardware divider or DTCM placement yet; no real-R4 int8 benchmark (runbook pending, needs the card on the DS). fp32 stays the exact oracle against run.c.

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

- Hardware divider and DTCM buffers, if the R4 number calls for more speed.

<!-- doctrina:applied-deltas specs/fixed-point-inference/delta.md=3e92eaf3,d90b5a5b,53154e6f,af92623d,fa003210,5aa65f3a,3170bfe3,c35cb19d,36827350 specs/pc-toolchain/delta.md=d90b5a5b,5a1119b8,48590616 -->
