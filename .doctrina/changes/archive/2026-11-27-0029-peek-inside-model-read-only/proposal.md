# Change 0029-peek-inside-model-read-only — Peek inside the model: a read-only step hook in the llama2 engine (called before each generated token is sampled, may force a token, absent by default so parity is unchanged), a top-k softmax helper in fixed point, and a visualizer that shows the five most likely next tokens with probability bars on the Generating screen for story models, toggled with X and remembered in dslm.ini

- **Status:** applied
- **Applied:** 2026-11-27
- **Date:** 2026-11-27
- **Owner:** Carini
- **Lane:** product (uncertain)
- **Affects specs:** inference-engine, ds-interface

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

Peek inside the model: a read-only step hook in the llama2 engine (called before each generated token is sampled, may force a token, absent by default so parity is unchanged), a top-k softmax helper in fixed point, and a visualizer that shows the five most likely next tokens with probability bars on the Generating screen for story models, toggled with X and remembered in dslm.ini

## What

First change of v1.5 ("inside the model").

- `src/engine/llama2.c/.h`:
  - `llama2.on_step`/`step_user` is called with the logits (fp32) or Q16 logits (int8) before each generated token is sampled. It may return a token to force, or -1. It is NULL after `llama2_init()`, so generation is unchanged. It is not called for prompt tokens.
  - `llama2_token_text()` exposes the decoded, printable piece.
- `src/engine/topk.c/.h` (new, pure C): the top-k with softmax probabilities in permille. Q16 uses `fx_exp_neg`; float uses double `exp`. Tested by `tests/host/test_topk.c` against a double reference.
- `tests/host/engine_main.c` and `tools/tests/test_engine_host.py`: `DSLM_HOOK_TOP1` forces topk's first token at temperature 1 and must equal the int8 temperature-0 golden. `tools/dslm.py` adds `topk.c` to the engine sources.
- `src/ui/ui_gfx.c`, `ui.h`:
  - `ui_peek_mode()`: none, offered, or on. When on, STOP shrinks and five rows of the likely tokens appear.
  - `ui_peek()` draws those rows with bars and percentages, with the pick highlighted. X raises `UI_EV_PEEK`.
- `src/main.c`: the `peek_step` hook fills the top-5 (story models only), `on_token` draws it and toggles on X, and the choice is saved after the run when it changed.
- `src/storage/ini.c/.h`: `[ui] peek = on|off`, with tests. The contract `model-file-format` documents it.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-peek.md`.

## Scope boundaries

- The chat engine (mlq) gets no hook: Peek is for story models.
- The hook only reads here. Forcing is used by the next change (co-writing) and by the host test.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (with topk), `make tools-test` (58 tests, with the hook test), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/host/test_topk.c`, `tools/tests/test_engine_host.py` and `tests/hw/ui-peek.md` (melonDS: parity 129/129 with peek on, +2.7% time while shown).

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=49c5b813,9acc44b0,d90b5a5b specs/inference-engine/delta.md=0f670e56,dd451970,14eeb0f9,d90b5a5b -->
