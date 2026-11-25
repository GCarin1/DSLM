# Spec Delta — capability: fixed-point-inference

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/fixed-point-inference/spec.md`

---

```ops
bump-version minor
append-requirement ubiquitous: The fixed-point build shall also run LittleKedi from the int8 MLQ1 file (group-wise int8 matrices and embedding, fp32 expert router converted to Q16 at load) with integer arithmetic only, sharing the integer kernels of the llama2 engine (ADR 0010).
append-requirement ubiquitous: The LittleKedi sampler shall apply a llama.cpp-style repeat penalty over the recent tokens before temperature and top-p, in integer arithmetic.
append-criterion [verified] The fixed-point LittleKedi engine matches its host-build golden greedy and sampled, agrees with the fp32 engine's argmax on at least 93% of teacher-forced positions with mean relative logit error at most 0.04, and its repeat penalty breaks the greedy loop — verified by `tools/tests/test_mlq_host.py`
```
