# Change 0015-model-picker — Model picker

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (uncertain)
- **Affects specs:** ds-interface, model-loading

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

Model picker: the Settings screen lists the checkpoints in /dslm/models, lets the user pick one with buttons or touch, saves it to dslm.ini and reloads the model without restarting; a model that fails to load is reported and the previous one is restored

## What

The owner put a second model on the card and found no way to choose it on the DS. The Settings screen gains a Model row: Left/Right or touch cycles through the checkpoints in /dslm/models that pass the loader's header and size checks; START unloads the current model, loads the chosen one with the progress bar and saves it to dslm.ini. If the new model fails (does not fit, unreadable, wrong tokenizer), the top screen shows why and the previous model is reloaded. Loading is split into boot / load_model / unload_model, and the budget now counts memory malloc holds free. Verified on melonDS: switch to tiny1m (129/129 tokens vs the host build) and an oversized fp32 checkpoint refused with recovery.

## Scope boundaries

No per-model tokenizer selection (all included models share tok512.bin); tiny1m is not added to the install package.

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

- Whether tiny1m ships in the package as a second model (after the owner's hardware test).

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=d90b5a5b,2bca895a,c9e63048,ab01abb8 specs/model-loading/delta.md=d90b5a5b,b67caa8e,05f10967,06975d9a -->
