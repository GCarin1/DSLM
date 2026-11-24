# ADR 0001 — Base the inference engine on llama2.c

- **Status:** proposed
- **Date:** 2026-11-24
- **Deciders:** Carini
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** n/a — no implementation yet
- **Landed:** —

## Context

The DS has no operating system, no FPU and 4 MB of RAM. Any inference engine must be pure C, small, dependency-free and easy to audit line by line. `llama2.c` (Andrej Karpathy) runs the Llama 2 architecture from a single C file of about 700 lines and has been ported to the Commodore 64, DOS, Windows 98, PSP, PS Vita and Xbox 360. Those ports show the required adaptations are well understood: replace `mmap()` with file reads and adapt the arithmetic. On the DS itself, ds-llm (int8 Llama 2 inference, MIT according to third parties) and chatds (BlocksDS, its own DSQ8 format) already exist; both are references to study, not engines to fork (see `docs/prior-art.md`).

## Decision

The inference engine is a port of `llama2.c`'s `run.c` (fp32) and, for the performance build, `runq.c` (int8). The project uses checkpoints and tokenizers in the `llama2.c` format, starting with `stories260K`.

## Alternatives considered

1. **llama.cpp / GGML.** Too large, depends on threads and on SIMD the ARM9 lacks, and the GGUF format carries far more metadata than needed.
2. **A custom engine from scratch.** Total freedom, but no reference to check parity against. `llama2.c` already serves as the oracle for the tests.
3. **A non-Transformer model (n-gram, small RNN).** Faster, but misses the goal of running an actual LLM.

## Consequences

**Positive**

- There is an executable reference on the PC to compare token by token (SC2).
- The `stories260K`/`stories15M` checkpoints and the training scripts already exist.

**Negative**

- `run.c` assumes `float` everywhere. The performance build is a substantial rewrite (ADR 0003).

**Neutral**

- The `llama2.c` MIT license must be preserved in ported files.
