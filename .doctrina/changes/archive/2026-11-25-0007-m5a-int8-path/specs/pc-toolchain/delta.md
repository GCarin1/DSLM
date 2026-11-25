# Spec Delta — capability: pc-toolchain

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/pc-toolchain/spec.md`

---

```ops
set-header Implementation: implemented — fetch, inspect, quantize (int8 v2), stage, fp32 and int8 golden outputs, parity and install package (`tools/dslm.py`)
bump-version minor
append-requirement ubiquitous: The toolchain shall choose an int8 group size that divides both dim and hidden_dim.
append-requirement event: When asked to build golden outputs for the int8 model, the toolchain shall run the pinned runq.c on the quantized checkpoint and save them separately from the fp32 golden outputs.
```
