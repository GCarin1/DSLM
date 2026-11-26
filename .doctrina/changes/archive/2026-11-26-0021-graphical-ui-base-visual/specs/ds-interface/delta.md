# Spec Delta — capability: ds-interface

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/ds-interface/spec.md`

<!--
For ADDED: include the full new spec body below. On apply, the body is
written verbatim to the target path.

For MODIFIED: prefer a fenced `ops` block — on apply the CLI executes it
against the target spec, all ops or none (ADR 0007). The verbs cover
headers, acceptance criteria, AND the EARS requirement bullets, so a
typical delta applies mechanically end to end; only free-prose rewrites
(Purpose, Maturity, ...) stay a by-hand merge. A MODIFIED delta with no
`ops` block prints a manual-merge pointer.

  ```ops
  set-header Implementation: verified — durable adapter (`src/db.ts`)
  bump-version minor
  set-criterion 1: verified
  append-criterion [unverified] new signal — verified by `test/x.test.ts`
  append-requirement event: When <trigger>, the system shall <action>.
  replace-requirement ubiquitous 2: The system shall <action>.
  ```

Requirement sections: ubiquitous | event | state | unwanted | optional.
To take one out: `remove-requirement <section> <n>` / `remove-criterion <n>`
(the criteria after it renumber; write removals highest number first).
append-* ops resolve numbering/position at APPLY time, so several open
changes appending to the same spec never collide on numbers — order of
application decides.

For REMOVED: the body may be empty; on apply, the target spec file is
deleted and the capability is recorded in the change archive only.
-->

---

<!-- delta body below -->

```ops
append-requirement ubiquitous: The interface shall draw both screens as bitmap graphics in the DSLM palette with the project pixel font, and shall show the DSLM logo splash while a model loads.
append-requirement unwanted: The interface shall not slow generation by more than 2% nor grow the binary by more than 64 KB against the console interface it replaced.
append-criterion [verified] Every screen draws in the DSLM palette and font, the splash shows the logo, and a touch that opens the prompt screen types nothing — verified by `tests/hw/ui-graphics.md`
append-criterion [verified] int8 generation is no slower than the console build (256 steps on melonDS), the binary grows under 64 KB, and LittleKedi still loads and matches its golden — verified by `tests/hw/ui-graphics.md`
set-header Implementation: verified — bitmap screens in the DSLM palette and pixel font, logo splash, libnds keyboard, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS, the reference R4 run of the graphical build lands with the v1.3.0 release change
set-header Last updated: 2026-11-26
bump-version minor
```
