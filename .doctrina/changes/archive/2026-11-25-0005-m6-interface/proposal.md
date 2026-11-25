# Change 0005-m6-interface — M6 interface

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** product (uncertain)
- **Affects specs:** ds-interface

<!--
Optional, and usually absent. The closing docs gate reads COMMAND and FLAG
names out of the prose below and asks for documentation when it finds any.
It cannot tell a change from a mention: explaining an effect, or writing a
Scope boundaries line about what this deliberately does NOT touch, names
things just as loudly as changing them would.

When that happens, say so on the record instead of forcing the close:

- **Documented surface:** n/a — names two commands to explain an effect; alters neither

`none` reads the same as `n/a`, and a BARE one silences nothing — the
reason is the declaration.
-->

## Why

M6 interface: libnds touch keyboard for the prompt, screen flow Loading/Prompt/Generating/Story/Settings with word-wrapped streaming, B or STOP to cancel, example prompts on L/R, settings saved to dslm.ini, every action reachable by buttons and by touch

## What

Milestone M6, with the screen flow, control map and keyboard choice approved by the user. The app loop in `src/main.c` runs Loading, Prompt, Generating, Story and Settings over the `ui.h` boundary; `src/ui/ui_console.c` draws a title bar, a word-wrapped body and a hint bar on the top screen, and a panel per screen plus the libnds touch keyboard on the bottom screen. `ui_poll()` maps buttons and touch to the same events, so every action has both. The engine's token callback can now stop generation (hold B or touch STOP). Settings are edited on the DS and saved to `/dslm/dslm.ini` through the new `dslm_ini_format`. `src/ui/textview.c` does the word wrap. ADR 0006 records the design; `docs/user-guide.md` documents screens and controls.

## Scope boundaries

No custom keyboard, D-pad typing, bitmap graphics or custom font (ADR 0006 alternatives). No install package (next change). No hardware run (M4).

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass (`doctrina verify`, or the project's typecheck/test/build).
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`).

## Open questions

- D-pad typing needs a custom keyboard; deferred behind `ui.h` (ADR 0006).

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=51dd7c6f,d90b5a5b,4c02e17b,53154e6f,adea3423,2a68c32e,0763f27d,0ba031b3,4ca8d678,bd3696c8,390d525f -->
