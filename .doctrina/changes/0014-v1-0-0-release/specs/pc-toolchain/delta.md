# Spec Delta — capability: pc-toolchain

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/pc-toolchain/spec.md`

---

```ops
set-header Implementation: verified — `tools/dslm.py` (fetch, inspect, quantize, stage, golden, parity, package, bench); packages verified on melonDS and the reference R4
bump-version minor
set-criterion 3: verified
append-requirement event: When a version tag is pushed, the release workflow shall run the tests, build dslm.nds and publish the install package, the standalone .nds and their SHA-256 sums with the release notes for that version.
append-criterion [verified] The release workflow's steps run end to end in the pinned BlocksDS image and produce the three assets — verified by `.github/workflows/release.yml`
```
