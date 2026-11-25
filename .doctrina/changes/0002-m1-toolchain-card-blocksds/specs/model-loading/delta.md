# Spec Delta — capability: model-loading

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/model-loading/spec.md`

---

```ops
set-header Implementation: partial — `/dslm/dslm.ini` parser and FAT mount (`src/storage/`); model reading still planned
bump-version minor
append-criterion [unverified] The `/dslm/dslm.ini` parser applies the contract defaults for missing files, missing keys and unknown keys — verified by `tests/host/test_ini.c`
append-criterion [unverified] The hello-world `.nds` mounts the card and prints the settings read from `/dslm/dslm.ini` on melonDS — verified by `tests/hw/hello-card.md`
```
