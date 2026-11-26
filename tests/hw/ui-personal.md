# Manual test: ui-personal

**Status:** runnable on melonDS since change 0026. Proves `ds-interface`: the user's name from the DS settings greets them, enters a story example, and the Colour row switches the accent to the DS favourite colour and saves it.

## Setup

Stage the card and build `dslm.nds`. In melonDS, set the firmware user name and favourite colour (Config > Emu settings > Firmware, or `Username` and `FavouriteColour` under `[Instance0.Firmware]` in `melonDS.toml`). On hardware, use the DS System Settings.

## Steps and expected result

1. Boot with `accent = ds` in `[ui]`. The Home header reads "Hi, <name>!", and the focus ring, title and buttons use the favourite colour (the logo art stays cyan).
2. Open Stories. The welcome text starts with "Hi, <name>!". Press R twice: the example reads "<name> wanted to".
3. Home → Settings. Up selects the Colour row. Left/Right switches between "DSLM cyan" and "My DS" and repaints at once. START saves `accent = ds|dslm` in `/dslm/dslm.ini`, and Back without saving restores the saved colour.
4. With no name in the DS settings, the header shows "D-pad + A or touch" and the welcome and examples are unchanged.
5. int8 parity still passes (the first example is not personalized).

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0026 | 1-3 pass with name "Carini" and colour 4 (orange), then colour 13 (purple, Colour row saved `accent = ds`); 5: int8 parity 129/129 in 35,446 ms; `dslm.nds` 231,424 bytes | yes |
