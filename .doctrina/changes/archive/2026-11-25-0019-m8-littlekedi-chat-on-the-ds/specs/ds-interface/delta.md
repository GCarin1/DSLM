# Spec Delta — capability: ds-interface

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/ds-interface/spec.md`

---

```ops
bump-version minor
append-requirement event-driven: When the loaded model is a chat model, the interface shall send the typed text as a question in the model's chat format, show the question above the answer, stop at the end-of-answer marker and offer example questions on L/R.
append-criterion [verified] With LittleKedi loaded, START on the first example question shows the question and the answer and the run matches the host golden token for token in melonDS — verified by `tests/parity/littlekedi-chat.md`
```
