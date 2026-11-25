# Change 0018-m8-fixed-point-mla-engine — M8 fixed-point MLA engine

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** capability
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

M8 step 2: int8 MLQ1 format for LittleKedi and a fixed-point MLA + MoE engine (shared integer kernels with the llama2 fixed engine, int8 embedding, integer sigmoid router, repeat-penalty sampler), validated against the fp32 C engine

## What

The integer kernels of the llama2 fixed engine (rmsnorm, quantize, ITCM matmul, softmax, sigmoid/SiLU, sampler) move to src/engine/fxkern.arm.c unchanged; the stories260K and tiny1m goldens stay bit-exact. tools/m8/convert_littlekedi.py --quantize writes MLQ1 (int8 group 32, router fp32; 2.25 MB). src/engine/mlq_fixed.arm.c is the integer MLA + MoE forward pass and a sampler with a llama.cpp-style repeat penalty. Tests: exact host goldens (greedy and sampled), teacher-forced drift against the fp32 engine (0.962 agreement, 0.024 error), and the penalty breaking the greedy loop. ADR 0010 records the router precision and the drift metric; llama.cpp Q8_0 gives the same answers as this engine where it diverges from F16.

## Scope boundaries

Not wired into the DS app yet (loader, chat mode, picker, package are M8 step 3); no DS run or benchmark yet.

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

<!-- doctrina:applied-deltas specs/fixed-point-inference/delta.md=d90b5a5b,d19f0195,e4d1bb28,a6360570 -->
