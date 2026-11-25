# Spec Delta — capability: pc-toolchain

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/pc-toolchain/spec.md`

---

```ops
bump-version minor
append-requirement event: When asked for the exact int8 DS reference, the toolchain shall build the DS engine for the host and save its outputs as the int8 golden, keeping the runq.c outputs as a separate quality reference.
append-requirement ubiquitous: The toolchain shall generate the fixed-point constant tables from a script and fail verification when the committed header is stale.
```
