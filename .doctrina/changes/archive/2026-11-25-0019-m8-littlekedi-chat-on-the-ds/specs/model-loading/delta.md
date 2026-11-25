# Spec Delta — capability: model-loading

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/model-loading/spec.md`

---

```ops
bump-version minor
append-requirement event-driven: When the checkpoint starts with the MLQ1 magic number, the loader shall read the tokenizer named in its header from the same folder and size the context from the memory left after both files, refusing the model below 128 tokens.
append-criterion [verified] The picker offers the MLQ1 chat model, the loader finds its tokenizer from the header and the package ships both with the Apache-2.0 notice — verified by `tools/tests/test_mlq_host.py`
```
