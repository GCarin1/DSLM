# Tasks — Change 0013-test-harness

- [x] Contract: bench.txt block (version, build, emulated=unknown, timings, heap peak), appended per run
- [x] Firmware: append /dslm/bench.txt with heap peak; DSLM_VERSION 1.0.0 in the log, welcome text and ROM title
- [x] `tools/dslm.py bench` imports the last run into benchmarks/ with the header; tests
- [x] `tests/parity/run_melonds.py`: unattended melonDS parity, fp32 and int8 both 129/129
- [x] `tests/hw/runbook.md` for the hardware bench run; DeSmuME moved to Future
