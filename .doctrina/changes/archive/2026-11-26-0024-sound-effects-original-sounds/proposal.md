# Change 0024-sound-effects-original-sounds — Sound effects: original sounds synthesized on the DS PSG channels (no samples, no RAM) for interface actions and moments — key click, focus move, delete, page toggle, open/confirm, back, start and stop generation, story done, error, and a boot chime — played by a small sequencer in the VBlank interrupt; a Sound on/off row on the Settings screen saved in dslm.ini; the sequencer stepping and the sound tables are pure C tested on the host

- **Status:** applied
- **Applied:** 2026-11-26
- **Date:** 2026-11-26
- **Owner:** Carini
- **Lane:** chore (confident; signals: move)
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

Sound effects: original sounds synthesized on the DS PSG channels (no samples, no RAM) for interface actions and moments — key click, focus move, delete, page toggle, open/confirm, back, start and stop generation, story done, error, and a boot chime — played by a small sequencer in the VBlank interrupt; a Sound on/off row on the Settings screen saved in dslm.ini; the sequencer stepping and the sound tables are pure C tested on the host

## What

An extra change before the v1.3.0 release, asked for by the deciders. They first wanted the Nintendo DS system sounds taken from a video; that was declined (copyright), and they chose original synthesized sounds instead.

- `src/ui/sfx.c/.h` (new, pure C): 11 original note sequences (key, move, delete, page, open, back, start, stop, done, error, boot) and the per-frame player (`sfx_start()`/`sfx_tick()`, linear decay). Tested by `tests/host/test_sfx.c` (new, in `tests/host/Makefile`).
- `src/ui/ui_gfx.c`: one PSG voice stepped from the VBlank interrupt (`soundPlayPSG()` at 8x the pitch, `soundSetVolume()`, `soundKill()`), `ui_sound()` and `ui_sound_enable()`. `ui_poll()` plays the sound of each event it returns. Focus moves and page changes play in place, and a fatal error plays "error". Settings has 5 rows of 20 px with the Sound row.
- `src/main.c`: boot chime on the first Home, "done" after a finished generation, "error" for a failed load on Home. The Sound row toggles and previews, and an unsaved switch is undone on Back.
- `src/storage/ini.c/.h`: `[ui] sound = on|off` (default on), with tests. The contract `model-file-format` documents it.
- ADR 0014. Docs: `docs/user-guide.md`, the package README in `tools/dslm.py`, `CHANGELOG.md`. New runbook `tests/hw/ui-sound.md`.

## Scope boundaries

- No samples, no recordings of other products' sounds, and no files on the card.
- No music, and no sound while text streams beyond start, stop and done.
- No engine change: token parity must be unchanged.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass: `make host-test` (ini with sound, textview, keyboard, sfx, fxmath), `make nds` with no warnings, and `doctrina verify`.
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`): `tests/host/test_sfx.c` and `tests/hw/ui-sound.md` (loopback recording on melonDS, Sound off saved, int8 parity 129/129 and 256 steps in 35,460 ms).

## Open questions

None.

<!-- doctrina:applied-deltas specs/ds-interface/delta.md=12d38842,09ae0519,87e5e831,79a8682b,2414b8a5,d90b5a5b -->
