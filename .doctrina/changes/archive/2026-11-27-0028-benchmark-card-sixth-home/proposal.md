# Change 0028-benchmark-card-sixth-home — Benchmark card: a sixth Home card runs the standard benchmark on the loaded model (first example prompt, temperature 0, 128 steps) without touching the user's settings or history, shows tokens per second, time per token, load time and heap peak on a results screen with Run again and Back, and appends the run to /dslm/bench.txt

- **Status:** applied
- **Applied:** 2026-11-26
- **Date:** 2026-11-26
- **Owner:** Carini
- **Lane:** product (uncertain)
- **Affects specs:** ds-interface

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

Benchmark card: a sixth Home card runs the standard benchmark on the loaded model (first example prompt, temperature 0, 128 steps) without touching the user's settings or history, shows tokens per second, time per token, load time and heap peak on a results screen with Run again and Back, and appends the run to /dslm/bench.txt

## What

Last change of v1.4 ("personal and practical").

- `src/main.c`: `bench_screen()` runs `generate()` with the first example, temperature 0 and 128 steps. The `benchmark` flag skips the history. The report shows the model and format, the prompt, the tokens and time, tokens/s and ms per token, the load time and heap peak. Run again or Back. The user's settings and prompt are restored, and the benchmark text is not kept as their story. `append_bench()` already writes `/dslm/bench.txt`.
- `src/ui/ui_gfx.c`, `ui.h`: the sixth Home card (Benchmark, gauge icon in `assets/ui/icons.txt`) and `ui_report()`, a results panel with Run again (A) and Back (B), on the new `UI_REPORT` screen.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-benchmark.md`.

## Scope boundaries

- The bench.txt block format is unchanged (contract `model-file-format`), so `tools/dslm.py bench` imports it as before.
- No engine change.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test`, `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/hw/ui-benchmark.md`.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=8ee17957,e136ad2e,d90b5a5b -->
