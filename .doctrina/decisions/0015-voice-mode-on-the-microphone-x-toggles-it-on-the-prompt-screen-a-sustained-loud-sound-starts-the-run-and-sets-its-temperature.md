# ADR 0015 — Voice mode on the microphone: X toggles it on the prompt screen; a sustained loud sound starts the run and sets its temperature

- **Status:** proposed
- **Date:** 2026-11-27
- **Deciders:** Carini (approved the v1.6 microphone features on 2026-11-26)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/ui/mic.c`, `src/ui/ui_gfx.c`, `src/main.c`, `tests/host/test_mic.c`
- **Landed:** —

## Context

The deciders' console is a DSi running DSLM through an R4 in DS mode, so the microphone is the one sensor beyond the buttons and touch screen (no camera in DS mode). Speech recognition cannot fit beside a 3.4 MB model on a 67 MHz CPU without an FPU. But loudness can be measured almost for free. Every prompt-screen button was already taken (ADR 0012), and X toggled shift, which the on-screen `Aa` key also does.

## Decision

- **X on the prompt screen switches voice mode on or off.** This amends ADR 0012's control map. Shift stays on the `Aa` key, reached by D-pad or touch. A header meter shows the live level, with a grey mark at the trigger threshold. The hint bar reads "blow:go START:go X:mic off".
- **Recording.**
  - `soundMicRecord()`: 12-bit samples (16-bit PCM), 8 kHz, a 2 x 400-sample double buffer (1.6 KB).
  - The callback invalidates the data cache over the half the ARM7 filled, then computes the level: the mean distance from the average, so DC offset does not count.
  - The microphone runs only while voice mode is on, and it is switched off when the run starts or the menu opens.
- **Blow.** A level of 40/127 or more for 15 frames in a row (about 0.25 s) starts the generation. Clicks and short sounds do not. A 90-frame cooldown follows.
- **Loudness sets creativity.** The loudest level of the blow maps linearly to a temperature from 0.2 (just loud enough) to 1.2 (loudest), for that run only. The saved settings keep their temperature, and the hint bar shows "voice:temp N.N".
- The detection and the mapping are pure C (`src/ui/mic.c`), tested on the host.

## Alternatives considered

1. **Word recognition (for example "go", "stop").** It does not fit beside the model in RAM and CPU.
2. **Mapping voice to another button combination.** Hard to discover. The `Aa` key already covers shift.
3. **A Settings row for the microphone.** Settings is full (six rows), and a toggle where it is used is clearer.

## Consequences

**Positive**

- A hands-free, playful start ("blow on the DS") that also teaches what temperature does.
- No cost when the mode is off. When it is on, it costs one level computation per 50 ms.

**Negative**

- X no longer shifts. Players use `Aa`.
- A noisy room can start a run while the mode is on.

**Neutral**

- On melonDS the microphone is fed by the "Mic" hotkey with the input set to noise or a WAV file.

<!--
Once this ADR is accepted, do not edit it. To change the decision,
create a new ADR that supersedes this one and update the "Superseded by"
header above to point at the new ADR. Status transitions:
proposed -> accepted | rejected
accepted -> deprecated | superseded by NNNN
-->
