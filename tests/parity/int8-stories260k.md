# Parity test: int8-stories260k

Proves `fixed-point-inference` criterion 3: at temperature 0, the DS int8 build produces the same token ids as `runq.c` on the PC (`tests/golden/stories260K_q80.json`, from `stories260K_q80.bin` built by `tools/dslm.py quantize`).

Two layers:

1. **Host (automated).** `tools/tests/test_engine_host.py` builds `src/engine/llama2.c` for the host and checks every int8 golden prompt token for token (`tools-test` in `doctrina verify`).
2. **DS (this runbook).** Same steps as `fp32-stories260k.md`; the default `dslm.ini` already points at the int8 model, and `tokens.txt` records the checkpoint so `python tools/dslm.py parity` picks the int8 golden.

## Results

| Date | Target | Build | Steps | Result | gen_ms | emulated |
|------|--------|-------|-------|--------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M5a int8 (GS 4) | 128 | parity ok, 129 tokens | 72658 | yes |
| 2026-11-25 | melonDS 1.1 | M5a int8 + per-token RoPE table | 128 | parity ok, 129 tokens | 69605 | yes |

For comparison, fp32 on melonDS took 87480 ms for the same run. Emulator timings are indicative only.
