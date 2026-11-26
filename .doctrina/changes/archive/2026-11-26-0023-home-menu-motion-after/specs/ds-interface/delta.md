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
append-requirement ubiquitous: The interface shall open on a Home screen with Stories, Chat, Settings and About cards, and SELECT shall return to it from the prompt and story screens.
append-requirement event: When the user opens the Stories or Chat card while the other kind of model is loaded, the interface shall load that mode's model with the loading progress and save the mode to /dslm/dslm.ini, so the next boot loads it and focuses its card.
append-requirement ubiquitous: The interface shall fade between screens and animate the brain icon while generating, using only hardware brightness and palette changes.
append-criterion [verified] Home opens Stories, Chat (loading LittleKedi), Settings and About by D-pad and touch, chat shows bubbles, and the mode is saved and focused after a reboot — verified by `tests/hw/ui-home.md`
append-criterion [verified] chat_checkpoint and the [ui] mode round-trip through dslm.ini and an unknown mode keeps the previous one — verified by `tests/host/test_ini.c`
set-header Implementation: verified — bitmap screens in the DSLM palette and pixel font, logo splash, Home menu with Stories, Chat, Settings and About, own QWERTY keyboard typed by D-pad or touch, VBlank-latched input, fades, chat bubbles, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS, the reference R4 run of the graphical build lands with the v1.3.0 release change
replace-requirement event 6: When the user picks another story model on the Settings screen and saves in Stories mode, the interface shall unload the current model, load the chosen one with the loading progress, and save it to /dslm/dslm.ini.
replace-requirement event 7: When the loaded model is a chat model, the interface shall send the typed text as a question in the model's chat format, show the question and the answer as speech bubbles, stop at the end-of-answer marker and offer example questions on L/R.
bump-version minor
```
