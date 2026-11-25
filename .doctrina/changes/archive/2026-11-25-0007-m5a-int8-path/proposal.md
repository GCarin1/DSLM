# Change 0007-m5a-int8-path — M5a int8 path

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (confident; signals: build, define)
- **Affects specs:**

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

M5a int8 path: define the llama2.c v2 int8 format (ak42) in the contract, quantize stories260K with group size 4 in pc-toolchain, build int8 golden outputs from the pinned runq.c, and run the int8 model on the DS with integer matmul and token parity against runq.c

## What

First half of M5. The int8 format is llama2.c's v2 (`ak42`) byte for byte, so the pinned `runq.c` is the oracle; `tools/dslm.py quantize` builds `stories260K_q80.bin` (521,728 bytes) from the fp32 model with group size 4, the largest that divides dim 64 and hidden_dim 172 (ADR 0007). The engine probes the magic number and runs either forward pass from one `.nds`; the int8 path keeps weights in place, dequantizes the embedding once and does integer dot products per group, mirroring `runq.c`. The int8 model becomes the default in `dslm.ini`, the stage and the package; fp32 ships next to it. Results: exact parity with `runq.c` on the host for every golden prompt and on melonDS (129/129); melonDS time for 128 positions 69.6 s vs 87.5 s in fp32, load 131 ms vs 264 ms. Both engines now compute the RoPE factors once per token instead of once per layer, bit-identical.

## Scope boundaries

No fixed-point activations, exp table, fixed-point scales or ITCM placement: that is M5b, where the real speedup is (group size 4 keeps one float multiply-add per 4 integer products, and expf dominates attention). No hardware int8 run yet (runbook `tests/hw/benchmark-int8.md`).

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

- M5b design: fixed-point scales (int64 accumulation) and an exp table; whether to keep bit-exact parity with runq.c or accept bounded drift checked against it.

<!-- doctrina:applied-deltas specs/fixed-point-inference/delta.md=a48528fc,d90b5a5b,f7a50a7f,741d37e9,11e37568,fb4f51e4 specs/pc-toolchain/delta.md=cb703640,d90b5a5b,1b0f6703,1b2c3bf5 -->
