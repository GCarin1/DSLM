# Tasks — Change 0007-m5a-int8-path

- [x] Contract: llama2.c v2 int8 layout; int8 model as the default in dslm.ini
- [x] `tools/dslm.py quantize` (Q8_0, group size dividing dim and hidden_dim) and tests
- [x] Pin runq.c; int8 golden outputs (`tests/golden/stories260K_q80.json`)
- [x] Engine: format probe, int8 weights in place, integer matmul, runq.c forward; host parity test
- [x] Loader and tokens.txt know the format and record the checkpoint; parity picks the right golden
- [x] Stage and package ship both models; firmware and tool defaults point at int8
- [x] Bit-identical speedup: RoPE factors once per token, sqrt(head_size) hoisted
- [x] melonDS: 129/129 tokens vs runq.c; ADR 0007; runbooks
