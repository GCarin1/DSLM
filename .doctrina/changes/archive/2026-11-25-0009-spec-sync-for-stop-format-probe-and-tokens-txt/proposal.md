# Change 0009-spec-sync-for-stop-format-probe-and-tokens-txt — Spec sync for stop, format probe and tokens.txt

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (confident; signals: record, spec)
- **Affects specs:** inference-engine

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

Spec sync: record that the engine stops when the caller asks, that the loader detects fp32 or int8 by the magic number, and the /dslm/tokens.txt run record in the contract

## What

Bookkeeping that the closing review of changes 0005 and 0007 asked for: three behaviours already shipped and tested had no sentence in a spec. inference-engine gains the stop requirement (callback asks to stop, tokens kept); model-loading gains the format probe and the tokens.txt record; the contract documents /dslm/tokens.txt. No code changes.

## Scope boundaries

No code. bench.txt stays as specified and is still not written by the homebrew.

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

<!-- doctrina:applied-deltas specs/inference-engine/delta.md=d90b5a5b,e5f45f84,4b8b7977 specs/model-loading/delta.md=d90b5a5b,a60aee08,fa530c2e,4107815b -->
