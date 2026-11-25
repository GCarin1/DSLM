# Spec Delta — capability: pc-toolchain

**Operation:** MODIFIED
**Target spec on apply:** `.doctrina/specs/pc-toolchain/spec.md`

---

```ops
bump-version minor
append-requirement event: When asked to package, the toolchain shall write a zip with dslm.nds, the /dslm tree with the pinned model and tokenizer, a user README, a manifest and the third-party license notices, laid out to unzip at the microSD root.
append-requirement unwanted: The toolchain shall not put flashcart kernels or DLDI driver files in a package.
append-criterion [verified] The package has the card-root layout, the licenses and no kernel or DLDI files, and two builds are byte-identical — verified by `tools/tests/test_package.py`
append-criterion [verified] The unzipped package alone boots and generates on a fresh melonDS card folder — verified by `tests/hw/package-install.md`
```
