# Manual test: ui-benchmark

**Status:** runnable on melonDS since change 0028. Proves `ds-interface`: the Benchmark card runs the standard measurement on the loaded model, shows the results and appends them to `/dslm/bench.txt`, without changing the user's settings or history.

## Setup

Stage the card and build `dslm.nds`. Note the files in `/dslm/history` and the `steps` and `temperature` values in `dslm.ini`. Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Home shows six cards. Down twice: Benchmark is highlighted.
2. A: the first example prompt is generated (128 steps, temperature 0) with the usual Generating screen. B stops it early, and the report says "(stopped)".
3. The results screen lists the model and format, the prompt, the tokens and seconds, tokens/s with ms per token, the load time and heap peak, and that the run went to `/dslm/bench.txt`. A runs it again; B returns to Home.
4. Close melonDS. `/dslm/bench.txt` ends with a block with `steps=128`. `/dslm/history` has no new file. `dslm.ini` is unchanged, and Stories still uses the user's steps and temperature.
5. On a real DS, import the block with `python tools/dslm.py bench` (`tests/hw/runbook.md`).

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0028 | 1-4 pass: stories260K_q80 int8, 128 tokens in 15.7 s, 8.13 tokens/s (122 ms each), load 138 ms, heap peak 1,338 KB; `bench.txt` got `steps=128`, history stayed at 5 files | yes |
| 2026-11-26 | DSi (DS mode), reference R4 / YSMenu | v1.3 development build | all four models: int8 7.68, tiny1m 4.93, chat 3.13, fp32 1.76 tokens/s; int8 parity 129/129 (`benchmarks/2026-11-26-r4-ysmenu-dsi-v1-3-dev-*.txt`) | no |
