# Parity test: int8-stories260k

Proves `fixed-point-inference` criterion 3. Since M5b the int8 model runs the fixed-point engine (ADR 0008), which has two references:

- **Exact (DS identity):** `tests/golden/stories260K_q80.json`, the same engine built for the host. Every operation after load is integer, so a DS run must match it token for token. `python tools/dslm.py parity` picks it from the checkpoint recorded in `tokens.txt`.
- **Quality (drift bound):** `tests/golden/stories260K_q80_runq.json`, from the pinned `runq.c` (float). At temperature 0 the engine must follow it for at least 64 generated tokens on every prompt (`tools/tests/test_engine_host.py`).

## Steps (DS)

1. `python tools/dslm.py stage .tools/card --force`, set `steps = 128`, run in melonDS (or on the R4), let it finish, close melonDS normally.
2. `python tools/dslm.py parity .tools/card/dslm/tokens.txt`.

## Drift against runq.c (host, temperature 0, 128 steps)

| Prompt | Generated tokens identical to runq.c |
|--------|--------------------------------------|
| (empty) | 128 of 128 |
| Once upon a time | 124 of 124 |
| The little dog | 82 |
| Lily wanted to | 124 of 124 |
| Tom and his mom went to the park | 92 |

## Results

| Date | Target | Build | Steps | Result | gen_ms | emulated |
|------|--------|-------|-------|--------|--------|----------|
| 2026-11-25 | melonDS 1.1 | M5a int8 (float scales, GS 4) | 128 | 129 tokens identical to runq.c | 72658 | yes |
| 2026-11-25 | melonDS 1.1 | M5a int8 + per-token RoPE table | 128 | 129 tokens identical to runq.c | 69605 | yes |
| 2026-11-25 | melonDS 1.1 | M5b fixed point | 128 | 129 tokens identical to the host build | 16546 | yes |
| 2026-11-25 | R4 clone / YSMenu (hardware) | M5b fixed point | 256 | 257 tokens identical to the host build (`benchmarks/2026-11-25-r4-ysmenu-int8.txt`) | 39080 | no |

For comparison, fp32 on melonDS took 87480 ms for the same run. Emulator timings are indicative only; hardware numbers go to `tests/hw/benchmark-int8.md`.
