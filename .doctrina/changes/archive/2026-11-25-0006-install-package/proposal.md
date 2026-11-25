# Change 0006-install-package — Install package

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** chore (confident; signals: readme)
- **Affects specs:** pc-toolchain

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

Install package: tools/dslm.py package builds a zip with dslm.nds, the /dslm tree with the pinned model, a user README and third-party license notices, ready to unzip at the microSD root; no kernels or DLDI files

## What

Brought forward from M7 at the user's request: players get one zip to unzip at the microSD root, with `dslm.nds`, the /dslm tree holding the pinned stories260K model and tokenizer, `README-DSLM.txt` (install, controls, troubleshooting), `MANIFEST.txt` and `THIRD_PARTY.txt`. The DS never downloads anything. Both the model (karpathy/tinyllamas) and llama2.c are MIT; the license text now lives in `third_party/llama2.c/LICENSE`, which the engine port also needed (ADR 0001). The zip is byte-reproducible for the same `dslm.nds`.

## Scope boundaries

No kernels or DLDI drivers (ADR 0005). No GitHub release workflow and no project LICENSE: DSLM's own license is still to be chosen by the owner. No real-R4 run (M4).

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

- DSLM's own license (README suggests MIT to match llama2.c); the owner decides.

<!-- doctrina:applied-deltas specs/pc-toolchain/delta.md=d90b5a5b,3439566b,56cdd616,e037f788,8686489e -->
