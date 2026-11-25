# Tasks — Change 0010-m5b-fixed-point-engine

- [x] `tools/gen_fxtables.py` and `src/engine/fxtables.h` (exp2 table, constants), stale check in verify
- [x] `src/engine/fxmath.arm.c`: integer exp, exp2, sin/cos, isqrt; host accuracy test
- [x] `src/engine/llama2_fixed.arm.c`: in-place Q30 scales, Q16 embedding, integer forward pass, ITCM matmul, integer sampler
- [x] llama2.c: float int8 path removed; generation dispatches to the fixed engine; shared RNG
- [x] Goldens: q80 from the host engine (exact), q80_runq from runq.c (quality); drift test (>= 64 tokens)
- [x] `tools/check_softfloat.py`: call graph from the binary, no soft-float, matmul in ITCM; detector self-test
- [x] Sampling with temperature tested (reproducible per seed); tokens/s on the story screen
- [x] melonDS: 129/129 vs host build, 16.5 s for 128 positions; ADR 0008; runbooks
