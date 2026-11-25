# Parity test: littlekedi-chat

Proves that the DS runs the LittleKedi chat model (MLQ1, `src/engine/mlq_fixed.arm.c` through `src/engine/mlq_chat.c`) exactly like its host build (ADR 0008, ADR 0010).

- **Exact (DS identity):** `tests/golden/littlekedi_q80.json`, `runs`: the host build with the DS defaults (temperature 0, repeat penalty 1.15). `python tools/dslm.py parity` picks it from the checkpoint recorded in `tokens.txt` and matches the prompt the engine read (chat markers included).
- **Quality:** teacher-forced agreement with the fp32 engine, `tools/tests/test_mlq_host.py` (ADR 0010).

## Steps (DS)

1. `python tools/dslm.py stage .tools/card`.
2. `python tests/parity/run_melonds.py --model littlekedi --steps 128` (sets the checkpoint, boots melonDS, presses START on the first chat example "What causes the seasons?", runs parity). On the R4: pick `littlekedi_q80` in Settings with temperature 0, press START on the first example, then run `python tools/dslm.py parity E:/dslm/tokens.txt`.

## What the screen shows

The top screen shows `> What causes the seasons?` and the answer below it; the title bar ends with `done` when the model emits `<|end|>`.

## Results

| Date | Target | Build | Steps | Result | gen_ms | emulated |
|------|--------|-------|-------|--------|--------|----------|
| 2026-11-25 | melonDS 1.1 | chat-int8 (change 0019) | 128 | 40 tokens identical to the host build (9 prompt + 31 answer, stopped at `<|end|>`); heap peak 3,420,400 B | 13114 | yes |
