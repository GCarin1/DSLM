# ADR 0010 — LittleKedi int8: full-precision router, drift measured teacher-forced against the fp32 engine

- **Status:** accepted
- **Date:** 2026-11-25
- **Deciders:** project owner, Claude (agent)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/engine/mlq_fixed.arm.c`, `tools/m8/convert_littlekedi.py`, `tools/tests/test_mlq_host.py`
- **Landed:** 2026-11-25 — `src/engine/mlq_fixed.arm.c`, `tools/tests/test_mlq_host.py`

## Context

ADR 0008 makes the int8 engine integer-only, exact against its host build, and bounds its drift against `runq.c` by the number of generated tokens that match at temperature 0. LittleKedi (M8) has no `runq.c`; its reference is the fp32 C engine (`src/engine/mla.c`), which equals the numpy oracle and llama.cpp's greedy output. It is also a mixture-of-experts model: each MoE layer picks the top 2 of 8 experts by `sigmoid(score) + bias`, and on the golden prompts two experts can be within 0.001 of each other. Any int8 rounding then flips the pick, and the rest of the sequence differs. llama.cpp's own Q8_0 file shows the same thing: on "Hello! Who are you?" it answers "Hello! How can I help you today?" where F16 answers otherwise, which is exactly what the DSLM int8 engine answers. A first-divergent-token bound is therefore a measure of luck with router ties, not of engine quality.

## Decision

- The MLQ1 file keeps the expert router (`gate_w`, 8×128) in fp32. The engine converts it to Q16 at load and scores the experts against the unquantized Q16 activation in 64-bit integer arithmetic, as llama.cpp keeps `ffn_gate_inp` unquantized. Every other matrix is int8 with fp32 group scales (group 32), including the tied embedding table.
- The drift of the LittleKedi int8 engine is measured teacher-forced: the fp32 and int8 engines read the same golden token sequence, and at every position the argmax must agree on at least 93% of positions and the mean relative L2 error of the logits must stay at or below 0.04 (measured 0.962 and 0.024).
- ADR 0008's other rules still hold: integers only after load, exact against its host build (`tests/golden/littlekedi_q80.json`, greedy and sampled), no soft-float in the per-token pass.

## Alternatives considered

1. Keep ADR 0008's first-divergent-token bound: rejected, as router ties make it 0 on prompts where the int8 output is as good as llama.cpp's Q8_0.
2. Quantize the router to int8 too: it costs 1 KB less per layer but adds rounding to the decision most sensitive to it; no gain on a 2.2 MB model.
3. Keep more of the model in higher precision (int16 weights): doubles the weight memory to about 4.4 MB, which does not fit the 4 MB RAM (ADR 0002).

## Consequences

**Positive**

- The metric reflects average fidelity and does not break on a single near-tie.
- The int8 engine's behaviour matches llama.cpp's own 8-bit quantization on the same prompts.

**Negative**

- Two tests now apply to fixed-point engines: the llama2 engine is bounded by first divergence, LittleKedi by agreement rate.

**Neutral**

- The router stays a small fp32 block in the file (4 KB per MoE layer).

<!--
Once this ADR is accepted, do not edit it. To change the decision,
create a new ADR that supersedes this one and update the "Superseded by"
header above to point at the new ADR. Status transitions:
proposed -> accepted | rejected
accepted -> deprecated | superseded by NNNN
-->
