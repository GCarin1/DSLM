# ADR 0012 — Custom QWERTY keyboard typed by D-pad or touch; input latched in the VBlank interrupt

- **Status:** accepted
- **Date:** 2026-11-26
- **Deciders:** Carini (chose QWERTY and the D-pad control map on 2026-11-26)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/ui/keyboard.c`, `src/ui/ui_gfx.c`, `tests/host/test_keyboard.c`
- **Landed:** 2026-11-26 — `src/ui/keyboard.c`, `src/ui/ui_gfx.c`, `tests/hw/ui-keyboard.md`

## Context

ADR 0011 kept the libnds keyboard on the prompt screen. That keyboard is touch-only, its keys are small, and it looks like the old console interface. ADR 0006 also left two gaps: typing needed the stylus, and stopping a generation needed B to be held, because input was read once per token. This is change 2 of the graphical interface update. It refines ADR 0011 for the prompt screen and its input and leaves the rest of ADR 0011 standing.

## Decision

- **Keyboard.**
  - An own QWERTY keyboard, drawn in the ADR 0011 style, replaces the libnds one. It has 34 keys of 23x26 px in four rows (y 76-188):
    - `q`-`p`
    - `a`-`l`
    - `Aa`, `z`-`m`, `'`, `DEL`
    - `?123`, `,`, space, `.`, `GO`
  - The pages are lower case, upper case (one-shot shift) and digits/symbols, all on one geometry.
  - The layout, the D-pad focus movement (rows and columns wrap, the nearest key by centre) and the pages are pure C in `src/ui/keyboard.c`, tested on the host.
- **Control map on the prompt screen.**
  - The D-pad moves the highlighted key, and A types it.
  - B deletes and Y types a space. X toggles shift. L/R pick examples, START generates and SELECT opens Settings.
  - A touch types the touched key and moves the highlight there.
  - A no longer generates (START and GO do), and Up/Down no longer scroll the last story on this screen (they still do on the Story screen).
- **Input.**
  - `scanKeys()` runs in the VBlank interrupt, which ORs the new presses, the auto-repeats and the position where a touch began into latches.
  - `ui_poll()` takes and clears the latches. So during generation a B tap or a STOP tap made between two tokens stops at the next token. No hold is needed.
  - Touches act on their down edge only, so a touch that opens a screen does nothing on the next one.

## Alternatives considered

1. **ABC grid.** Easier to find letters with the D-pad, but less familiar by touch. The deciders chose QWERTY.
2. **Keep the libnds keyboard and add D-pad focus to it.** Its key map is internal and its look clashes with ADR 0011.
3. **Poll input more often during generation (between layers).** It would touch the engines, and the latch gives the same result from the ui layer alone.

## Consequences

**Positive**

- The prompt can be typed without the stylus, and the keys are about twice as large as before.
- The stop is reliable with a tap. The ADR 0006 "hold B" note no longer applies.
- The binary shrinks by about 12 KB, because the libnds keyboard graphics are gone.

**Negative**

- Players used to A = generate on the prompt screen must use START.
- The last story cannot be scrolled from the prompt screen.

**Neutral**

- Only one event is reported per `ui_poll()`. Presses latched together in one frame beyond the first are dropped, which is fine at human speed.

<!--
Once this ADR is accepted, do not edit it. To change the decision,
create a new ADR that supersedes this one and update the "Superseded by"
header above to point at the new ADR. Status transitions:
proposed -> accepted | rejected
accepted -> deprecated | superseded by NNNN
-->
