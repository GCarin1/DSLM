# Tasks — Change 0003-m2-pc-toolchain

- [x] Pin the stories260K files and the llama2.c commit with sizes and SHA-256 (`tools/models.json`)
- [x] `tools/dslm.py fetch` refuses downloads that do not match the pin
- [x] `tools/dslm.py inspect` prints the header and the DS memory budget
- [x] `tools/dslm.py stage` writes the /dslm tree with dslm.ini and manifest.txt
- [x] `tools/golden/golden.c` + `tools/dslm.py golden` write tests/golden/stories260K.json
- [x] Tests: inspect, stage, golden (twice identical, matches stock run.c)
- [x] Boot the staged tree in melonDS
- [x] Contract: list /dslm/manifest.txt; docs and verify.json updated
