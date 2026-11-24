# Spec Delta — capability: model-loading

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/model-loading/spec.md`

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
append-requirement ubiquitous: The loader shall read model files with fread calls of at least 32 KB each.
append-requirement event: When the homebrew starts, the loader shall read the model path, tokenizer path and sampling settings from /dslm/dslm.ini.
append-requirement event: When FAT initialisation fails, the loader shall print a hint that the binary may need a manual DLDI patch with dlditool.
append-requirement unwanted: The loader shall not depend on launch arguments (argv) for any setting.
append-criterion [unverified] The stories260K load time on a real R4 is measured with a DS hardware timer and recorded — verified by `tests/hw/load-time.md`.
bump-version minor
```
