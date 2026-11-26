# ADR 0014 — Interface sounds synthesized on the PSG channels by a VBlank sequencer; no samples

- **Status:** accepted
- **Date:** 2026-11-26
- **Deciders:** Carini (chose original sounds synthesized on the DS on 2026-11-26)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/ui/sfx.c`, `src/ui/ui_gfx.c`, `tests/host/test_sfx.c`
- **Landed:** 2026-11-26 — `src/ui/sfx.c`, `src/ui/ui_gfx.c`, `tests/hw/ui-sound.md`

## Context

The graphical interface (ADRs 0011-0013) was silent. The deciders asked for sounds like the Nintendo DS's own system sounds, taken from a video recording of them. Those recordings are Nintendo's copyrighted audio, and DSLM is published openly (repository and package), so they cannot be shipped. Policy ADR 0005 likewise keeps third-party firmware and ROM content out. Main RAM is also nearly full with LittleKedi (heap peak 3.42 MB), so even small samples compete with the model.

## Decision

- **Original sounds only, synthesized on the DS's PSG square-wave channels.** No samples and no audio files, on the card or in the binary.
  - 11 sounds, each a short sequence of notes (pitch, length in frames, volume, duty cycle) in `src/ui/sfx.c`: key, move, delete, page, open, back, start, stop, done, error and boot.
  - Feedback blips last at most 4 frames. Interface sounds last at most 40 frames, and the boot chime at most 90.
- **Sequencer.**
  - A pure-C player (`sfx_tick()`, tested on the host) steps one frame at a time from the VBlank interrupt, through one voice, with a linear decay to half volume over each note. A new sound replaces the playing one.
  - The glue calls `soundPlayPSG()`, `soundSetVolume()` and `soundKill()`. The PSG rate is 8x the pitch (8 timer steps per square cycle), which was measured on melonDS.
- **Wiring.**
  - `ui_poll()` plays the sound of each event it returns. Focus moves and page changes play inside the ui layer.
  - `main.c` plays the boot chime, "done", and "error" on a failed load.
- **Switch.** A Sound row on the Settings screen. It is saved as `[ui] sound = on|off` in `/dslm/dslm.ini` (contract `model-file-format`), and the default is on.

## Alternatives considered

1. **The Nintendo DS system sounds taken from a recording.** Rejected: copyrighted, and the project is distributed publicly.
2. **Original WAV samples embedded in the .nds.** Richer timbre, but they cost binary size (main RAM) and need the sample channels. The PSG covers clicks and chimes.
3. **User-supplied samples in `/dslm/sfx/`.** Possible later. Not needed for this change.
4. **Driving the sounds from the main loop.** During generation the main loop runs once per token, which would stretch the notes. The VBlank interrupt keeps the timing exact.

## Consequences

**Positive**

- Every action has audible feedback and the boot has a chime, at 0 bytes of RAM for audio data.
- The binary grows only by the sequencer and tables: 221,184 bytes against 219,136 after change 0023.
- int8 generation is unchanged on melonDS (256 steps: 35,460 ms) and parity is identical.

**Negative**

- Square waves sound simple next to sampled audio.

**Neutral**

- One voice: a new sound cuts the previous one, which suits short interface feedback.

<!--
Once this ADR is accepted, do not edit it. To change the decision,
create a new ADR that supersedes this one and update the "Superseded by"
header above to point at the new ADR. Status transitions:
proposed -> accepted | rejected
accepted -> deprecated | superseded by NNNN
-->
