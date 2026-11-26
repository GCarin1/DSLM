# Change 0027-battery-mode-while-generation — Battery mode: while a generation runs, closing the lid turns both backlights off instead of sleeping and the model keeps writing; when it finishes with the lid closed the done sound plays and the DS goes to sleep, waking on open with the result on screen; the title bar shows a red battery icon when the battery is low

- **Status:** applied
- **Applied:** 2026-11-26
- **Date:** 2026-11-26
- **Owner:** Carini
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

Battery mode: while a generation runs, closing the lid turns both backlights off instead of sleeping and the model keeps writing; when it finishes with the lid closed the done sound plays and the DS goes to sleep, waking on open with the result on screen; the title bar shows a red battery icon when the battery is low

## What

Third change of v1.4 ("personal and practical").

- `src/ui/ui_gfx.c`, `ui.h`: `ui_generating()` replaces `ui_pulse()` in the API.
  - While a generation runs it calls `disableSleep()`, and the VBlank interrupt turns both backlights off or on with the lid (`KEY_LID`, `powerOff/powerOn(PM_BACKLIGHT_*)`).
  - When the generation ends with the lid closed, `ui_frame()` waits for the done sound, then `systemSleep()`. `enableSleep()` restores normal lid sleep.
  - The title bar draws a red battery when `getBatteryLevel()` is at or below `BATTERY_LEVEL_DS_LOW`.
- `src/main.c`: `generate()` calls `ui_generating(1/0)`.
- Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-battery.md`.

## Scope boundaries

- No backlight dimming setting and no battery percentage (DS mode reports only high or low).
- No engine change.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test`, `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/hw/ui-battery.md` (melonDS lid hotkey and low-battery setting).

## Open questions

None. The backlight-off step is checked on hardware with the v1.3.0 release run.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=8723039e,98bc46b2,6a764c0b,401cbd1f,d90b5a5b -->
