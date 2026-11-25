# Spec Delta — capability: fixed-point-inference

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/fixed-point-inference/spec.md`

---

```ops
set-header Implementation: implemented — integer-only int8 engine (`src/engine/llama2_fixed.arm.c`, `src/engine/fxmath.arm.c`, ADR 0008), matmul in ITCM, verified on the host and melonDS; the R4 benchmark is pending
bump-version minor
set-criterion 2: verified
remove-criterion 5
append-requirement ubiquitous: The fixed-point build shall produce bit-identical results on the DS and in its host build, using no libm and no float after the model is loaded.
append-requirement unwanted: The fixed-point build shall not diverge from runq.c within the first 64 generated tokens of any golden prompt at temperature 0.
append-criterion [verified] At temperature 0 the fixed-point engine matches runq.c for at least 64 generated tokens on every golden prompt — verified by `tools/tests/test_engine_host.py`
append-criterion [verified] A DS int8 run matches the host build of the engine token for token — verified by `tests/parity/int8-stories260k.md`
append-criterion [verified] The integer exp, sin/cos and square root stay within 3e-5 of libm over their working ranges — verified by `tests/host/test_fxmath.c`
```
