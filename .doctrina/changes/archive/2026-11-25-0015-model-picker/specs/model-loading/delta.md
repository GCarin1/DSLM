# Spec Delta — capability: model-loading

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/model-loading/spec.md`

---

```ops
bump-version minor
append-requirement ubiquitous: The loader shall list as models only the files in /dslm/models whose header and size pass the checkpoint checks.
append-requirement ubiquitous: The loader shall count memory released by a previous model as available when checking the budget for the next one.
append-criterion [verified] Only valid checkpoints are offered, and after unloading a model the budget sees the freed memory — verified by `tests/hw/model-picker.md`
```
