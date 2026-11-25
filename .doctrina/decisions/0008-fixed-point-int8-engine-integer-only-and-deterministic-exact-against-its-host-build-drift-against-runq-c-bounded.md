# ADR 0008 — Fixed-point int8 engine: integer-only and deterministic, exact against its host build, drift against runq.c bounded

- **Status:** proposed
- **Date:** 2026-11-25
- **Deciders:** Carini (agreed to accept bounded drift against runq.c, 2026-11-25)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/engine/llama2_fixed.arm.c`, `src/engine/fxmath.arm.c`, `tools/check_softfloat.py`, `tools/tests/test_engine_host.py`
- **Landed:** —

## Context

ADR 0003 calls for a fixed-point int8 build checked against `runq.c`. The M5a int8 path (ADR 0007) kept `runq.c`'s float arithmetic: exact, but only about 20% faster than fp32 on melonDS, because every 4 int8 products still paid a soft-float multiply-add and `expf`/`powf`/`cosf` dominated attention and RoPE. Fixed point cannot be bit-exact with float `runq.c`, so the project needed another exact oracle for DS runs, and a measure of quality instead of identity.

## Decision

- The int8 checkpoint runs a fixed-point engine with no float after load: Q16 activations, norms, RoPE and logits; weight scales converted in place to Q30 at load; activation scales Q24; int8 dot products in int32 per group and int64 per row; `exp`, `sin`/`cos` and `1/sqrt` from integer code (`fxmath`) and compiled-in tables (`tools/gen_fxtables.py`), never libm.
- Because every operation after load is integer, the DS and the host build of the same engine agree bit for bit. Their output is the exact golden for DS int8 runs (`tests/golden/stories260K_q80.json`); `runq.c`'s output becomes the quality reference (`stories260K_q80_runq.json`).
- Quality bound: at temperature 0 the fixed-point engine must match `runq.c` for at least 64 generated tokens on every golden prompt (measured 82 to 128, three prompts identical for all 128).
- The engine and the integer math compile in ARM mode (`*.arm.c`); `matmul_fx` lives in ITCM. `tools/check_softfloat.py` proves from the linked binary that the per-token path reaches no soft-float routine and that the matmul is in ITCM.
- The fp32 engine stays the exact oracle against `run.c` (ADR 0003).

## Alternatives considered

1. **Keep the float int8 path (ADR 0007).** Exact against `runq.c`, but about 4x slower than fixed point on melonDS.
2. **Fixed point with libm-built tables at start-up.** Simpler, but `cosf`/`exp2f` differ between picolibc and glibc in the last bit, which would break DS-versus-host identity.
3. **Exactness against `runq.c` as the gate.** Impossible for any fixed-point design; it would forbid the optimisation the milestone exists for.

## Consequences

**Positive**

- melonDS: 16.5 s for 128 positions (about 7.7 tokens/s) against 69.6 s for float int8 and 87.5 s for fp32, with exact DS parity against the host build.
- No float in the per-token path, checked mechanically on every `doctrina verify`.

**Negative**

- Stories drift from `runq.c` after tens of tokens on some prompts; quality is bounded by a test, not proven identical.
- The int8 checkpoint buffer is rewritten in place at load and is no longer a valid file in RAM.

**Neutral**

- Integer divisions (`__aeabi_ldivmod`) remain in rmsnorm, softmax, SiLU and quantize; the DS hardware divider is a later optimisation.
