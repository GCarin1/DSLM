# Manual test: ui-history

**Status:** runnable on melonDS since change 0025. Proves `ds-interface`: every finished story or chat answer is saved to `/dslm/history` with its date, and the History card lists, shows, scrolls and deletes them.

## Setup

Stage the card (`python tools/dslm.py stage .tools/card`) and build `dslm.nds`. Set `steps = 64` in `dslm.ini` for a quick run. Key mapping as in `ui-navigation.md`.

## Steps and expected result

1. Home: the five cards (Stories, Chat, History, Settings, About) in two columns.
2. Stories, START: a story is written. SELECT, then Chat, START: an answer. SELECT.
3. Down to History, A: the list shows two rows, newest first, as `MM-DD HH:MM` and the prompt (the chat question in light cyan). The top screen shows the chat in bubbles. The title bar shows the model and `history`.
4. Down (or touch the row): the story is on top. L/R scroll it by a page when it is longer than the screen.
5. Y: the button reads "Again: delete". Y again: the entry is gone from the list and from `/dslm/history`. Any other key cancels.
6. B: back to Home. Close melonDS. `/dslm/history/H000N.TXT` files hold `dslm-history=1`, `date`, `model`, `kind`, `question`, `tokens`, `stopped`, `prompt`, a blank line, then the text.
7. Parity runs still pass and each adds one entry.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-26 | melonDS 1.1 | change 0025 | 1-7 pass: two entries listed newest first with the host clock's date (2026-11-26 20:39), chat bubbles in the preview, double-Y delete removed `H0001.TXT`; LittleKedi parity 40/40 (heap peak 3,418,232 B), int8 parity 129/129 in 35,465 ms; `dslm.nds` 230,400 bytes | yes |
