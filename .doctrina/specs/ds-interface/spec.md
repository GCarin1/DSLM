# Spec — ds-interface

**Capability:** ds-interface
**Status:** active
**Implementation:** planned — current phase is documentation and planning only; no code yet
**Realizes:** SC4
**Depends on:** inference-engine
**Last updated:** 2026-11-24
**Version:** 0.1.0

## Purpose

The homebrew's interface on the DS's two 256x192 screens. The top screen streams the generated text. The bottom (touch) screen shows the on-screen keyboard for typing the prompt and the generation status. It starts with the libnds text console, with no custom graphics.

## Requirements (EARS)

### Ubiquitous

- The interface shall show generated text on the top screen and the on-screen keyboard on the bottom touch screen.

### Event-driven

- When the user confirms the prompt on the keyboard, the interface shall start generation with that prompt.
- When the engine produces a token, the interface shall append its text to the top screen without waiting for the whole answer.
- When the user presses B during generation, the interface shall stop generation and keep the text produced so far.

### State-driven

- While generation is running, the interface shall show on the bottom screen that the model is working and how many tokens have been produced.

### Unwanted-behavior (must-not)

- The interface shall not accept a new prompt while a generation is running.

### Optional

- Where the user opens the settings screen, the interface may let them change temperature, top-p and the step limit.

## Acceptance criteria

1. [unverified] A prompt typed on the on-screen keyboard generates text that appears token by token on the top screen — verified by `tests/hw/ui-streaming.md`.
2. [unverified] Pressing B stops generation and keeps the text produced so far — verified by `tests/hw/ui-cancel.md`.

## Maturity

**MVP (committed):**

- libnds console, default libnds keyboard, streaming and cancellation.

**Future (aspirational, not committed):**

- Settings screen, custom font and generation history saved to microSD.

## Out of scope for this spec

- Any inference logic.
