# Spec Delta — capability: inference-engine

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/inference-engine/spec.md`

---

```ops
bump-version minor
append-requirement ubiquitous: The engine shall also run LittleKedi-style checkpoints (DeepSeek-V3 multi-head latent attention, sigmoid-gated top-k experts plus a shared expert, GPT-2 byte-level BPE) from the MLA1 and BPE1 files written by tools/m8/convert_littlekedi.py.
append-criterion [verified] The LittleKedi C engine and tokenizer reproduce the numpy oracle token for token on every golden prompt; the oracle equals llama.cpp greedy output with the repeat penalty off — verified by `tools/tests/test_mla_host.py`
```
