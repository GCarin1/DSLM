# Spec Delta — capability: ds-interface

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/ds-interface/spec.md`

---

```ops
set-header Implementation: partial — `ui.h` boundary and libnds console backend (`src/ui/`); keyboard, streaming and cancel still planned
bump-version minor
append-requirement ubiquitous: The firmware shall confine all screen and input handling to the ui layer behind `src/ui/ui.h`, so the storage and engine layers build and test without libnds.
append-criterion [unverified] The storage layer compiles and passes its tests on the host with no libnds header on the include path — verified by `tests/host/test_ini.c`
```
