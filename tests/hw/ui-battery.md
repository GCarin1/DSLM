# Manual test: ui-battery

**Status:** runnable on melonDS since change 0027 (the backlight step needs hardware). Proves `ds-interface`: closing the lid during a generation keeps it running with the screens off, the DS sleeps once it is done, and a low battery shows in the title bar.

## Setup

Stage the card and build `dslm.nds`. In melonDS, map a key to "Close/open lid" (`HK_Lid` under `[Instance0.Keyboard]` in `melonDS.toml`). For the battery step, set `LevelOkay = false` under `[Instance0.DS.Battery]` (Config > Power management).

## Steps and expected result

1. Stories, START (256 steps). After about 3 s, close the lid.
2. The token count keeps rising while the lid is closed. On hardware both backlights go off (melonDS does not show backlight changes).
3. About 35 s later the done sound plays and the DS sleeps (melonDS shows black screens).
4. Open the lid: the DS wakes on the Story screen ("Done", 257 tokens, a normal time per token).
5. With the lid closed and no generation running, the DS sleeps as usual.
6. Low battery: a red battery icon left of the state pill in the title bar.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0027 | 1-4 pass: 28 → 56 tokens with the lid closed, sleep after done, woke on "Done 257 tokens, 35.4 s" (gen_ms 35,447); 6: the red icon with `LevelOkay = false`. The backlight-off step is left for the R4 run of the v1.3.0 release | yes |
| 2026-11-26 | DSi (DS mode), reference R4 / YSMenu | v1.3 development build | lid closed mid-generation turned the screens off and the run finished; sleep and wake on the result; checked by the owner | no |
