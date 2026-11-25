# Spec Delta — capability: test-harness

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/test-harness/spec.md`

---

```ops
set-header Implementation: implemented — bench.txt with heap peak (`src/main.c`), import into benchmarks/ (`tools/dslm.py bench`), unattended melonDS parity (`tests/parity/run_melonds.py`), golden and parity tools; the v1.0 hardware bench run is pending
bump-version minor
remove-criterion 2
remove-criterion 1
append-requirement event: When a benchmark run is imported, the harness shall add the cart, kernel, microSD and commit and refuse to write an emulated run into benchmarks/.
append-criterion [verified] An unattended melonDS run of the fp32 and of the int8 build reports zero divergence against their golden outputs — verified by `tests/parity/run_melonds.py`
append-criterion [verified] bench.txt blocks are parsed and imported with the header, and emulator runs are refused — verified by `tools/tests/test_bench.py`
```
