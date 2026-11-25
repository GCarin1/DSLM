# Change 0014-v1-0-0-release — v1.0.0 release

- **Status:** proposed
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** runtime (confident; signals: github actions, tag) — opened anyway (--force)
- **Affects specs:** pc-toolchain, ds-interface, inference-engine, model-loading, test-harness

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

v1.0.0 release: CHANGELOG, release notes, compatibility table, README for players, every spec verified, and a GitHub Actions workflow that builds, tests and publishes the release assets on a version tag

## What

Milestone M7, DSLM 1.0.0. Release notes in docs/releases/v1.0.0.md (What's New, changes and defaults, bug fixes, technical, full changelog, assets), CHANGELOG.md, docs/compatibility.md, a player-facing README, and .github/workflows/release.yml: a v* tag runs the host and toolchain tests, builds dslm.nds in the pinned BlocksDS image, checks the int8 path for soft-float and ITCM placement, and publishes dslm-<tag>.zip, dslm.nds and SHA256SUMS.txt with the release notes. The remaining specs move to verified; test-harness criterion 3 closes with the owner's hardware run of the release build.

## Scope boundaries

No engine or UI change. Publishing (merge to main, tag, push) happens only after the owner's hardware run and go-ahead.

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

- Release from main after merging develop, or tag develop directly: owner's call.
