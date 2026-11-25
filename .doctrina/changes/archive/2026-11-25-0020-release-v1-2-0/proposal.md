# Change 0020-release-v1-2-0 — Release v1.2.0

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** chore (confident; signals: bump, changelog) — opened as chore
- **Affects specs:** (none — chore)

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

Release v1.2.0: chat mode with LittleKedi (version bump, release notes, changelog, tag)

## What

Version 1.2.0: the LittleKedi chat mode (changes 0017-0019) with its hardware result (2.81 tokens/s, 263/263 parity on the R4). Bumps DSLM_VERSION, dates the changelog, adds docs/releases/v1.2.0.md; tagging v1.2.0 runs .github/workflows/release.yml, which publishes the package, dslm.nds and SHA256SUMS.txt.

## Scope boundaries

No code or spec behaviour changes.

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

<!-- doctrina:applied-deltas none -->
