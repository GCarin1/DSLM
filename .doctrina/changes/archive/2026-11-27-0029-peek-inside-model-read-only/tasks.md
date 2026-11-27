# Tasks — Change 0029-peek-inside-model-read-only

- [x] Engine step hook (`on_step`) and `llama2_token_text()`; NULL by default
- [x] `src/engine/topk.c/.h` with host tests against a double reference
- [x] Host engine test: the hook forcing top-1 at temperature 1 equals the greedy golden
- [x] Visualizer on the Generating screen (X), `[ui] peek` in `dslm.ini` with tests and the contract
- [x] melonDS: visualizer, parity with peek on, timing (`tests/hw/ui-peek.md`); docs and CHANGELOG
