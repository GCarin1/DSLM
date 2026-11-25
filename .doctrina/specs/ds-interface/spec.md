# Spec — ds-interface

**Capability:** ds-interface
**Status:** active
**Implementation:** verified — screens, keyboard, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS and the reference R4
**Realizes:** SC4
**Depends on:** inference-engine
**Last updated:** 2026-11-25
**Version:** 0.5.0

## Purpose

The homebrew's interface on the DS's two 256x192 screens. The top screen streams the generated text. The bottom (touch) screen shows the on-screen keyboard for typing the prompt and the generation status. It starts with the libnds text console, with no custom graphics.

## Requirements (EARS)

### Ubiquitous

- The interface shall show generated text on the top screen and the on-screen keyboard on the bottom touch screen.
- The firmware shall confine all screen and input handling to the ui layer behind `src/ui/ui.h`, so the storage and engine layers build and test without libnds.
- The interface shall make every action reachable both by a DS button and by a touch target on the bottom screen.
- The interface shall wrap text on both screens at word boundaries, breaking a word only when it is longer than a line.

### Event-driven

- When the user confirms the prompt on the keyboard, the interface shall start generation with that prompt.
- When the engine produces a token, the interface shall append its text to the top screen without waiting for the whole answer.
- When the user presses B during generation, the interface shall stop generation and keep the text produced so far.
- When the user presses L or R on the prompt screen, the interface shall replace the prompt with the previous or next example prompt.
- When the user saves the settings screen, the interface shall write the settings to /dslm/dslm.ini and apply them to the next generation.
- When the user picks another model on the Settings screen and saves, the interface shall unload the current model, load the chosen one with the loading progress, and save it to /dslm/dslm.ini.
- When the loaded model is a chat model, the interface shall send the typed text as a question in the model's chat format, show the question above the answer, stop at the end-of-answer marker and offer example questions on L/R.

### State-driven

- While generation is running, the interface shall show on the bottom screen that the model is working and how many tokens have been produced.

### Unwanted-behavior (must-not)

- The interface shall not accept a new prompt while a generation is running.
- The interface shall not leave the DS without a model: when the chosen model fails to load, it shall show the reason and load the previous model again.

### Optional

- Where the user opens the settings screen, the interface may let them change temperature, top-p and the step limit.

## Acceptance criteria

1. [verified] A prompt typed on the on-screen keyboard generates text that appears token by token on the top screen — verified by `tests/hw/ui-streaming.md`.
2. [verified] Pressing B stops generation and keeps the text produced so far — verified by `tests/hw/ui-cancel.md`.
3. [verified] The storage layer compiles and passes its tests on the host with no libnds header on the include path — verified by `tests/host/test_ini.c`
4. [verified] Every documented action works by button and by touch on melonDS — verified by `tests/hw/ui-navigation.md`
5. [verified] Wrapped lines never exceed the width and keep words whole — verified by `tests/host/test_textview.c`
6. [verified] Stopping generation keeps the tokens produced so far — verified by `tools/tests/test_engine_host.py`
7. [verified] Picking another model on the Settings screen switches models without a restart, and a model that does not fit is refused with the previous one restored — verified by `tests/hw/model-picker.md`
8. [verified] With LittleKedi loaded, START on the first example question shows the question and the answer and the run matches the host golden token for token in melonDS — verified by `tests/parity/littlekedi-chat.md`

## Maturity

**MVP (committed):**

- libnds console, default libnds keyboard, streaming and cancellation.
- Five screens (Loading, Prompt, Generating, Story, Settings), example prompts on L/R, every action on buttons and touch (ADR 0006).
- Settings screen for temperature, top-p and steps, saved to /dslm/dslm.ini.

**Future (aspirational, not committed):**

- Custom keyboard with D-pad typing, custom font and graphics, and generation history saved to microSD.

## Out of scope for this spec

- Any inference logic.
