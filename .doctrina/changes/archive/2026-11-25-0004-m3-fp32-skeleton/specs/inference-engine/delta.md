# Spec Delta — capability: inference-engine

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/inference-engine/spec.md`

---

```ops
set-header Implementation: partial — fp32 forward pass, tokenizer and sampler (`src/engine/llama2.c`), parity on melonDS; real-R4 run and benchmark are M4
bump-version minor
set-criterion 1: verified
append-criterion [verified] The engine built for the host reproduces every golden prompt token for token — verified by `tools/tests/test_engine_host.py`
```
