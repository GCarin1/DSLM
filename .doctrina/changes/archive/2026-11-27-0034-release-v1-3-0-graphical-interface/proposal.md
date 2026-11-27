# Change 0034-release-v1-3-0-graphical-interface — Release v1.3.0: graphical interface, keyboard, Home, sounds, history, personalization, battery mode, benchmark card, peek, co-writing, paths, book mode and voice mode (version bump, release notes, changelog, hardware results; no tag or main merge yet)

- **Status:** applied
- **Applied:** 2026-11-27
- **Date:** 2026-11-27
- **Owner:** Carini
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

Release v1.3.0: graphical interface, keyboard, Home, sounds, history, personalization, battery mode, benchmark card, peek, co-writing, paths, book mode and voice mode (version bump, release notes, changelog, hardware results; no tag or main merge yet)

## What

Version 1.3.0 combines the graphical-interface update and the follow-up feature sets (changes 0021-0033): interface, keyboard, Home, sounds, history, personalization, battery mode, Benchmark card, peek, co-writing, paths, book mode and voice mode.

- `DSLM_VERSION` becomes 1.3.0. The CHANGELOG section is dated and gains the hardware results (the DSi Benchmark-card runs and hardware int8 parity). The README status is updated.
- Release notes in `docs/releases/v1.3.0.md`.
- Hardware rows in `tests/hw/ui-battery.md`, `ui-voice.md`, `ui-sound.md` and `ui-benchmark.md`. The owner checked the lid, battery, microphone and sounds on the DSi.

## Scope boundaries

- No code or spec behaviour changes.
- **No tag and no merge to `main`.** The owner asked to keep everything on `develop` for now. Tagging v1.3.0 later runs `.github/workflows/release.yml`, which publishes the package.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test`, `make tools-test`, `make nds`, `make package`, and `doctrina verify` (the release workflow steps, run locally).
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`).

## Open questions

When to merge `develop` into `main` and tag v1.3.0 is the owner's call.

<!-- doctrina:applied-deltas none -->
