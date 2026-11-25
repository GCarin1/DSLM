# Tasks — Change 0018-m8-fixed-point-mla-engine

- [x] Shared integer kernels (src/engine/fxkern.arm.c) used by the llama2 fixed engine, bit-exact goldens unchanged
- [x] MLQ1 int8 format (router fp32) and dslm.ensure_littlekedi_q(); fixed-point MLA + MoE engine (src/engine/mlq_fixed.arm.c) with repeat-penalty sampler
- [x] Host goldens, drift test against fp32 (teacher-forced), ADR 0010
