# Spec Delta — capability: model-loading

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/model-loading/spec.md`

---

```ops
bump-version minor
append-requirement ubiquitous: The loader shall tell the fp32 and int8 checkpoint formats apart by the first four bytes and check the file size against the size its header implies.
append-requirement event: When a generation ends, the homebrew shall write /dslm/tokens.txt with the checkpoint, prompt, token ids and timings, in the format of contracts/model-file-format.
append-criterion [verified] The same loader runs the fp32 and the int8 checkpoint and records the checkpoint in tokens.txt — verified by `tests/parity/int8-stories260k.md`
```
