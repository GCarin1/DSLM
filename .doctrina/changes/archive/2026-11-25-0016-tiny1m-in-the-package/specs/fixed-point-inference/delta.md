# Spec Delta — capability: fixed-point-inference

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/fixed-point-inference/spec.md`

---

```ops
bump-version minor
replace-requirement unwanted 2: The fixed-point build shall not diverge from runq.c at temperature 0 within the per-model bound of generated tokens on any golden prompt: 64 for stories260K and 6 for tiny1m (measured minima 82 and 7).
append-criterion [verified] Both packaged int8 models (stories260K, tiny1m) match their host-build golden exactly and stay within their drift bound against runq.c — verified by `tools/tests/test_engine_host.py`
append-criterion [verified] tiny1m runs on the reference R4 with token parity against the host build — verified by `tests/hw/model-picker.md`
```
