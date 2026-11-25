# Spec Delta — capability: ds-interface

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/ds-interface/spec.md`

---

```ops
bump-version minor
append-requirement event: When the user picks another model on the Settings screen and saves, the interface shall unload the current model, load the chosen one with the loading progress, and save it to /dslm/dslm.ini.
append-requirement unwanted: The interface shall not leave the DS without a model: when the chosen model fails to load, it shall show the reason and load the previous model again.
append-criterion [verified] Picking another model on the Settings screen switches models without a restart, and a model that does not fit is refused with the previous one restored — verified by `tests/hw/model-picker.md`
```
