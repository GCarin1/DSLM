# Spec Delta — capability: fixed-point-inference

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/fixed-point-inference/spec.md`

---

```ops
set-header Implementation: partial — int8 weights with integer matmul and exact parity against runq.c (`src/engine/llama2.c`, ADR 0007); fixed-point activations, exp table and ITCM placement are M5b
bump-version minor
set-criterion 3: verified
set-criterion 4: verified
append-requirement event: When the checkpoint starts with the int8 v2 magic number, the engine shall run the int8 forward pass; otherwise it shall run the fp32 forward pass.
append-criterion [verified] The int8 engine built for the host reproduces every runq.c golden prompt token for token — verified by `tools/tests/test_engine_host.py`
```
