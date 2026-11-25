# Tasks — Change 0015-model-picker

- [x] Loading split into boot, load_model (returns an error instead of halting) and unload_model
- [x] Budget counts freed malloc blocks (mallinfo) so a second model can load
- [x] scan_models: /dslm/models checkpoints that pass the header and size checks
- [x] Settings: Model row with Left/Right and touch, START switches and saves, failure restores the previous model and explains why
- [x] melonDS: switch to tiny1m (129/129 vs host), oversized model refused and recovered
- [x] user guide, release notes, changelog, runbook
