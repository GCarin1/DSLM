# Spec Delta — capability: inference-engine

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/inference-engine/spec.md`

---

```ops
bump-version minor
append-requirement event: When the caller's per-token callback asks to stop, the engine shall end generation after that token and keep every token produced so far.
append-criterion [verified] Stopping after N tokens returns BOS plus those N tokens, identical to the golden prefix — verified by `tools/tests/test_engine_host.py`
```
