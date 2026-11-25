# ADR 0006 — Console UI with the libnds keyboard; every action on buttons and touch

- **Status:** proposed
- **Date:** 2026-11-25
- **Deciders:** Carini (approved the screen flow, control map and keyboard choice on 2026-11-25)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/ui/ui.h`, `src/ui/ui_console.c`, `src/main.c`, `docs/user-guide.md`
- **Landed:** —

## Context

The M3 skeleton proved inference with a fixed prompt but had no way to type a prompt, no layout and no navigation. The DS has two 256x192 screens (the bottom one touch-only), ten buttons and a stylus that many players misplace. Text generation takes about half a second per token, so a story takes minutes and must be stoppable. The interface has to stay behind `src/ui/ui.h` so storage and engine code keep building without libnds.

## Decision

- The interface is built on the libnds text console (32x24 cells per screen) with ANSI colours, plus the default libnds touch keyboard on the bottom screen's BG3, shown only on the prompt screen and with its slide animation disabled.
- Five screens: Loading, Prompt, Generating, Story and Settings. The top screen always carries a title bar, a word-wrapped text body and a hint bar; the bottom screen carries the panel of the current screen.
- Every action has both a button and a touch target (control map in `docs/user-guide.md`); `ui_poll()` maps both to the same events. Typing itself is touch-only; L/R example prompts cover play without the stylus.
- Generation polls input once per token: holding B or touching STOP stops it and keeps the text.
- Settings (temperature, top-p, steps) are edited on the DS and saved to `/dslm/dslm.ini`.

## Alternatives considered

1. **A custom-drawn keyboard with D-pad navigation now.** Bigger keys and typing without the stylus, but more work and risk in this milestone; it can replace the libnds keyboard later behind `ui.h`.
2. **Bitmap graphics and a custom font.** Better looking, but costs VRAM and time before the experience is proven; deferred to the Future list of `ds-interface`.
3. **Buttons only or touch only.** Either excludes a common way of holding the DS.

## Consequences

**Positive**

- Works on the DS and melonDS with no extra assets; every screen is testable by posting keys and taps to melonDS.
- The `ui.h` boundary means a custom keyboard or graphics can replace the console without touching the app loop.

**Negative**

- No D-pad typing; the libnds keyboard key cells are small for fingers.
- A press shorter than one token is only seen if the button is still held when the token finishes, so stopping needs a hold (documented as "hold B").

**Neutral**

- The bottom console moves to tile base 3 so it does not overlap the keyboard graphics at tile base 0.
