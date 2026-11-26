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
append-requirement event: When a generation finishes or is stopped, the interface shall save its text, prompt, model and date to a new file in /dslm/history.
append-requirement ubiquitous: The interface shall list the saved history newest first from the History card, show the chosen entry on the top screen, and delete an entry only after the delete button is pressed twice.
append-criterion [verified] History files round-trip their header and text, tolerate CRLF and cut headers, and are named H0001.TXT upwards — verified by `tests/host/test_history.c`
append-criterion [verified] Finished runs are saved with the clock date, listed newest first, shown again with chat bubbles and deleted by a double press — verified by `tests/hw/ui-history.md`
set-header Implementation: verified — bitmap screens in the DSLM palette and pixel font, logo splash, Home menu with Stories, Chat, History, Settings and About, own QWERTY keyboard typed by D-pad or touch, VBlank-latched input, fades, chat bubbles, synthesized interface sounds, history on the card, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS, the reference R4 run of the graphical build lands with the v1.3.0 release change
bump-version minor
```
