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
append-requirement ubiquitous: The interface shall let the user type the prompt both by touching the keys of the on-screen QWERTY keyboard and by moving its highlight with the D-pad and pressing A.
append-criterion [verified] Every key is reachable by D-pad from every other key, every key is hit at its centre, and shift is one-shot — verified by `tests/host/test_keyboard.c`
append-criterion [verified] A prompt typed with the D-pad and A alone generates, and a short B tap stops generation — verified by `tests/hw/ui-keyboard.md`
set-header Implementation: verified — bitmap screens in the DSLM palette and pixel font, logo splash, own QWERTY keyboard typed by D-pad or touch, VBlank-latched input, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS, the reference R4 run of the graphical build lands with the v1.3.0 release change
replace-requirement event 3: When the user presses B during generation, even briefly, the interface shall stop generation at the next token and keep the text produced so far.
bump-version minor
```
