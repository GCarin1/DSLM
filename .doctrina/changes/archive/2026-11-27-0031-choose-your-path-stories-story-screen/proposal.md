# Change 0031-choose-your-path-stories-story-screen — Choose-your-path stories: from the Story screen of a story model, X continues the story with three alternative short continuations generated from the end of the text; the user picks one (D-pad and A or touch), it is appended and three new ones follow, until B finishes; the engine gains llama2_next and llama2_feed to step and rebuild the cache from any position, proven to continue exactly like an uninterrupted generation

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

Choose-your-path stories: from the Story screen of a story model, X continues the story with three alternative short continuations generated from the end of the text; the user picks one (D-pad and A or touch), it is appended and three new ones follow, until B finishes; the engine gains llama2_next and llama2_feed to step and rebuild the cache from any position, proven to continue exactly like an uninterrupted generation

## What

Third change of v1.5 ("inside the model").

- `src/engine/llama2.c/.h`:
  - `llama2_next()` feeds a token at a position and samples the next one with the current temperature and top-p (int8 refreshes `fx_begin`).
  - `llama2_feed()` only feeds a token, to rebuild the cache.
  - Contract: after `llama2_generate()` returns n tokens, positions 0..n-2 are in the cache.
- `tests/host/engine_main.c` and `tools/tests/test_engine_host.py`: `DSLM_CONTINUE` stops after k tokens (k = 1, 17, 60) and steps on with `llama2_next`. The result must equal the int8 golden.
- `src/main.c`:
  - `paths_screen()` writes 3 paths of up to 24 tokens from the end of the story, at a temperature of at least 0.8 so they differ.
  - The user picks one. A pick other than the last one written is fed again. The tokens and text are appended, and the next round follows. B finishes and saves a history entry.
  - `can_branch()` requires a story model and room in the context. `gen_tokens` keeps the speed figure of the original run.
- `src/ui/ui_gfx.c`, `ui.h`: `ui_paths()` shows either the writing status or 3 cards with focus, plus Finish/Cancel. The Story panel gets a 4th button, "What happens next? (X)", and raises `UI_EV_PATHS`. `ui_panel_story()` takes `gen_tokens`.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-paths.md`.

## Scope boundaries

- Story models only. The chat engine has no stepping API.
- `llama2_generate()` is unchanged, so the goldens stay the same.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test`, `make tools-test` (59 tests, with the continuation test), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tools/tests/test_engine_host.py` and `tests/hw/ui-paths.md`.

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=f856782c,65ada79b,d90b5a5b specs/inference-engine/delta.md=57ea67aa,83d1d513,d90b5a5b -->
