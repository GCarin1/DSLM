# Tasks — Change 0004-m3-fp32-skeleton

- [x] Port run.c to `src/engine/llama2.c` on in-memory buffers, one allocation, no allocation while generating
- [x] Host driver and test: the engine reproduces every golden prompt (`tools/tests/test_engine_host.py`)
- [x] Storage: header-only read, chunked read with progress, file write
- [x] UI: token streaming on the top screen, status line on the bottom screen
- [x] Loader in `src/main.c`: files found, header and size check, RAM budget before allocating
- [x] Fixed-prompt generation that writes `/dslm/tokens.txt`; `tools/dslm.py parity`
- [x] melonDS: 128-step parity, oversized model refused, missing file reported
- [x] Runbooks updated; M4 skeletons for offline-run and benchmark-fp32
