# Change 0011-mit-license — MIT license

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

License DSLM under MIT: LICENSE file at the root, shipped in the install package, README and package.json updated

## What

The project owner chose the MIT License, matching llama2.c and the stories260K model. `LICENSE` at the root (copyright 2026 Carini) notes that the engine is ported from llama2.c (MIT, `third_party/llama2.c/LICENSE`). The install package now ships `LICENSE.txt`; the README gains a License section and drops the open question; `package.json` and its lock declare MIT.

## Scope boundaries

No per-file license headers beyond the existing llama2.c notices; no code change.

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

<!-- doctrina:applied-deltas specs/pc-toolchain/delta.md=e7c38c12,a31dcd5a -->
