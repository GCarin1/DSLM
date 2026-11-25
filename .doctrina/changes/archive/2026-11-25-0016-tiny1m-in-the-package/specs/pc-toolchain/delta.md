# Spec Delta — capability: pc-toolchain

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/pc-toolchain/spec.md`

---

```ops
bump-version minor
append-requirement ubiquitous: The toolchain shall pin every packaged model (URL, size, SHA-256, author, license) in tools/models.json and ship each as int8, plus fp32 only when it fits the DS.
append-criterion [verified] The install package carries stories260K (int8 and fp32) and tiny1m (int8) with a notice per model — verified by `tools/tests/test_package.py`
```
