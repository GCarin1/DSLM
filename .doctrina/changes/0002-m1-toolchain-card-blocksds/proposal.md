# Change 0002-m1-toolchain-card-blocksds — M1 toolchain and card I/O: BlocksDS project skeleton with a UI-ready layered architecture (ui/engine/storage separation) and a hello-world .nds that reads /dslm/dslm.ini via DLDI

- **Status:** proposed
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product
- **Affects specs:** model-loading, ds-interface

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

M1 toolchain and card I/O: BlocksDS project skeleton with a UI-ready layered architecture (ui/engine/storage separation) and a hello-world .nds that reads /dslm/dslm.ini via DLDI

## What

First firmware code (milestone M1). A BlocksDS ARM9 project with three layers: `storage` (FAT mount, file reads, `dslm.ini` parser), `ui` (the only code that touches screens and input) and `main`. The `.nds` mounts the card, reads `/dslm/dslm.ini` and prints the settings on the top screen, or shows the failure. The `ui.h` boundary is what lets the M6 interface (touch keyboard, token streaming, cancel, settings) replace the console backend without touching storage or engine.

## Scope boundaries

No model reading, no inference, no keyboard, no streaming, no benchmarks. The SDK version pin (ADR 0004) is recorded as an open question below, not invented.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [ ] Automated checks pass (`doctrina verify`, or the project's typecheck/test/build).
- [ ] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`).

## Open questions

- Which BlocksDS release (or Docker image tag) to pin; needs a machine that has it installed.
- The Makefile template name (`rom_combo` assumed) is unverified until a BlocksDS build runs.
