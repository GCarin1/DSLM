# Manual test: ui-book

**Status:** runnable on melonDS since change 0032. Proves `ds-interface`: a story or a history entry can be read as a book, on rotated pages across both screens.

## Setup

Stage the card and build `dslm.nds`. Key mapping as in `ui-navigation.md`. Turn the DS a quarter turn counter-clockwise to read (top screen on the left).

## Steps and expected result

1. Stories, START, let it finish. The Story screen shows a "Book (START)" button next to the speed figure.
2. START: both screens fade to light pages turned a quarter turn, with the title (the model) at the top, a rule, the text in dark ink, and "page / pages" at the foot. The left page is on the top screen and the right page on the bottom screen. A right page after the last one stays dark.
3. A, R, Right, Down or a touch on the bottom screen: the next spread (when there is one). Y, L, Left or Up: the previous one.
4. B or START: back to the Story screen, upright again.
5. Home → History. The hint shows "A:book". A opens the selected entry as a book titled with its date. B returns to the History list, faded in.
6. A story over 20 lines spreads over several pages, and the page numbers count them.

## Results

| Date | Target | Build | Result | emulated |
|------|--------|-------|--------|----------|
| 2026-11-27 | melonDS 1.1 | change 0032 | 1-5 pass: a 257-token story on page 1 / 1 (right page dark), START and B in and out with fades, the History entry titled "2026-11-26 21:37"; the page count follows 20 lines per page | yes |
