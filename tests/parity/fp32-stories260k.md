# Parity test: fp32-stories260k

Proves `inference-engine` criterion 1: at temperature 0, the DS fp32 build produces the same token ids as the PC golden output (`tests/golden/stories260K.json`, built from the pinned `llama2.c` `run.c`).

Two layers:

1. **Host (automated).** `tools/tests/test_engine_host.py` builds `src/engine/llama2.c` for the host and checks every golden prompt token for token. It runs in `doctrina verify` (`tools-test`).
2. **DS (this runbook).** The `.nds` runs the fixed prompt `Once upon a time` and writes `/dslm/tokens.txt`.

## Steps

1. Stage the card: `python tools/dslm.py stage .tools/card`, then set `steps = 128` in `.tools/card/dslm/dslm.ini` (the golden length).
2. Build `dslm.nds` (`docs/building.md`) and run it in melonDS with DLDI and "Sync SD to folder". Wait for `done` on the bottom screen.
3. Close melonDS normally (File > Quit or the window button) so it writes the card back to the folder.
4. `python tools/dslm.py parity .tools/card/dslm/tokens.txt` prints `parity ok: N tokens identical` or the first diverging index.

## Results

| Date | Target | Build | Steps | Result | gen_ms | emulated |
|------|--------|-------|-------|--------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M3 fp32 (`-O2`, BlocksDS digest in `docs/building.md`) | 32 | parity ok, 33 tokens | 17783 | yes |
| 2026-11-25 | melonDS 1.1 | M3 fp32 | 128 | parity ok, 129 tokens | 87480 | yes |
| 2026-11-25 | R4 clone / YSMenu (hardware) | M6 package | 256 (stopped) | parity ok, 153 tokens vs `run.c` for prompt `hi` | 95380 | no |

Emulator timings are indicative only; the hardware row is from the reference R4 (M4).
