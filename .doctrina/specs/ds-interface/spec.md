# Spec — ds-interface

**Capability:** ds-interface
**Status:** active
**Implementation:** verified — bitmap screens in the DSLM palette and pixel font, logo splash, Home menu with Stories, Chat, History, Settings and About, own QWERTY keyboard typed by D-pad or touch, VBlank-latched input, fades, chat bubbles, synthesized interface sounds, history on the card, streaming, cancel and settings (`src/ui/`, `src/main.c`); verified on melonDS, the reference R4 run of the graphical build lands with the v1.3.0 release change
**Realizes:** SC4
**Depends on:** inference-engine
**Last updated:** 2026-11-26
**Version:** 0.17.0

## Purpose

The homebrew's interface on the DS's two 256x192 screens. The top screen streams the generated text. The bottom (touch) screen shows the on-screen keyboard for typing the prompt and the generation status. Both screens are bitmap graphics in the colours of the DSLM logo, with the project's pixel font and the logo splash while a model loads (ADR 0011).

## Requirements (EARS)

### Ubiquitous

- The interface shall show generated text on the top screen and the on-screen keyboard on the bottom touch screen.
- The firmware shall confine all screen and input handling to the ui layer behind `src/ui/ui.h`, so the storage and engine layers build and test without libnds.
- The interface shall make every action reachable both by a DS button and by a touch target on the bottom screen.
- The interface shall wrap text on both screens at word boundaries, breaking a word only when it is longer than a line.
- The interface shall draw both screens as bitmap graphics in the DSLM palette with the project pixel font, and shall show the DSLM logo splash while a model loads.
- The interface shall let the user type the prompt both by touching the keys of the on-screen QWERTY keyboard and by moving its highlight with the D-pad and pressing A.
- The interface shall open on a Home screen with Stories, Chat, Settings and About cards, and SELECT shall return to it from the prompt and story screens.
- The interface shall fade between screens and animate the brain icon while generating, using only hardware brightness and palette changes.
- The interface shall play original sounds synthesized on the PSG channels for its actions and moments (key, move, delete, page, open, back, start, stop, done, error, boot), with no audio samples.
- The interface shall list the saved history newest first from the History card, show the chosen entry on the top screen, and delete an entry only after the delete button is pressed twice.
- Where the DS settings hold a user name, the interface shall greet the user by that name on the Home screen and in the welcome text.

### Event-driven

- When the user confirms the prompt on the keyboard, the interface shall start generation with that prompt.
- When the engine produces a token, the interface shall append its text to the top screen without waiting for the whole answer.
- When the user presses B during generation, even briefly, the interface shall stop generation at the next token and keep the text produced so far.
- When the user presses L or R on the prompt screen, the interface shall replace the prompt with the previous or next example prompt.
- When the user saves the settings screen, the interface shall write the settings to /dslm/dslm.ini and apply them to the next generation.
- When the user picks another story model on the Settings screen and saves in Stories mode, the interface shall unload the current model, load the chosen one with the loading progress, and save it to /dslm/dslm.ini.
- When the loaded model is a chat model, the interface shall send the typed text as a question in the model's chat format, show the question and the answer as speech bubbles, stop at the end-of-answer marker and offer example questions on L/R.
- When the user opens the Stories or Chat card while the other kind of model is loaded, the interface shall load that mode's model with the loading progress and save the mode to /dslm/dslm.ini, so the next boot loads it and focuses its card.
- When the user switches Sound off on the Settings screen and saves, the interface shall stop playing sounds and save the choice to /dslm/dslm.ini.
- When a generation finishes or is stopped, the interface shall save its text, prompt, model and date to a new file in /dslm/history.
- When the user picks My DS on the Colour row of the Settings screen and saves, the interface shall draw its accent in the favourite colour from the DS settings and save the choice to /dslm/dslm.ini.
- When a generation ends with the lid closed, the interface shall play the done sound and then put the DS to sleep, showing the result when the lid is opened.
- When the user opens the Benchmark card, the interface shall generate the first example prompt at temperature 0 for 128 steps on the loaded model, show its speed, time per token, load time and heap peak, and append the run to /dslm/bench.txt without changing the settings or the history.
- When the user presses Y while a story model generates, the interface shall offer the five most likely next tokens at each step and use the one the user picks, until Y gives the choice back to the model or B stops.
- When the user presses X on the Story screen of a story model with room left in the context, the interface shall write three alternative continuations, append the one the user picks, and offer three more until B finishes.
- When the user presses START on the Story screen or A on a History entry, the interface shall show the text as a book on rotated pages across both screens, turning pages on A, R or a touch and closing on B or START.

### State-driven

- While generation is running, the interface shall show on the bottom screen that the model is working and how many tokens have been produced.
- While a generation is running, closing the lid shall turn both backlights off and let the generation continue instead of putting the DS to sleep.
- While the battery is low, the interface shall show a red battery icon in the title bar.

### Unwanted-behavior (must-not)

- The interface shall not accept a new prompt while a generation is running.
- The interface shall not leave the DS without a model: when the chosen model fails to load, it shall show the reason and load the previous model again.
- The interface shall not slow generation by more than 2% nor grow the binary by more than 64 KB against the console interface it replaced.

### Optional

- Where the user opens the settings screen, the interface may let them change temperature, top-p and the step limit.
- Where a story model is generating, the interface may show the five most likely next tokens with their probabilities, toggled with X and remembered in /dslm/dslm.ini, without changing the generated tokens.

## Acceptance criteria

1. [verified] A prompt typed on the on-screen keyboard generates text that appears token by token on the top screen — verified by `tests/hw/ui-streaming.md`.
2. [verified] Pressing B stops generation and keeps the text produced so far — verified by `tests/hw/ui-cancel.md`.
3. [verified] The storage layer compiles and passes its tests on the host with no libnds header on the include path — verified by `tests/host/test_ini.c`
4. [verified] Every documented action works by button and by touch on melonDS — verified by `tests/hw/ui-navigation.md`
5. [verified] Wrapped lines never exceed the width and keep words whole — verified by `tests/host/test_textview.c`
6. [verified] Stopping generation keeps the tokens produced so far — verified by `tools/tests/test_engine_host.py`
7. [verified] Picking another model on the Settings screen switches models without a restart, and a model that does not fit is refused with the previous one restored — verified by `tests/hw/model-picker.md`
8. [verified] With LittleKedi loaded, START on the first example question shows the question and the answer and the run matches the host golden token for token in melonDS — verified by `tests/parity/littlekedi-chat.md`
9. [verified] Every screen draws in the DSLM palette and font, the splash shows the logo, and a touch that opens the prompt screen types nothing — verified by `tests/hw/ui-graphics.md`
10. [verified] int8 generation is no slower than the console build (256 steps on melonDS), the binary grows under 64 KB, and LittleKedi still loads and matches its golden — verified by `tests/hw/ui-graphics.md`
11. [verified] Every key is reachable by D-pad from every other key, every key is hit at its centre, and shift is one-shot — verified by `tests/host/test_keyboard.c`
12. [verified] A prompt typed with the D-pad and A alone generates, and a short B tap stops generation — verified by `tests/hw/ui-keyboard.md`
13. [verified] Home opens Stories, Chat (loading LittleKedi), Settings and About by D-pad and touch, chat shows bubbles, and the mode is saved and focused after a reboot — verified by `tests/hw/ui-home.md`
14. [verified] chat_checkpoint and the [ui] mode round-trip through dslm.ini and an unknown mode keeps the previous one — verified by `tests/host/test_ini.c`
15. [verified] Every sound is a playable PSG sequence within its length budget and the sequencer starts each note once, decays and ends — verified by `tests/host/test_sfx.c`
16. [verified] The boot chime and the open sound play on melonDS at their pitches, and Sound off is saved and silences them — verified by `tests/hw/ui-sound.md`
17. [verified] History files round-trip their header and text, tolerate CRLF and cut headers, and are named H0001.TXT upwards — verified by `tests/host/test_history.c`
18. [verified] Finished runs are saved with the clock date, listed newest first, shown again with chat bubbles and deleted by a double press — verified by `tests/hw/ui-history.md`
19. [verified] The 16 favourite colours are distinct and readable, their shades bracket them, and firmware names convert to trimmed ASCII — verified by `tests/host/test_theme.c`
20. [verified] The greeting, the personal example and the saved favourite-colour accent show on melonDS with a firmware name and colour — verified by `tests/hw/ui-personal.md`
21. [verified] With the lid closed mid-generation the tokens keep coming, the DS sleeps after the done sound and wakes on the finished story, and a low battery shows the red icon — verified by `tests/hw/ui-battery.md`
22. [verified] The Benchmark card reports the standard run, appends it to bench.txt and leaves dslm.ini and the history untouched — verified by `tests/hw/ui-benchmark.md`
23. [verified] The Generating screen shows the five likely tokens with bars, X toggles them, and int8 parity is unchanged with them shown — verified by `tests/hw/ui-peek.md`
24. [verified] Picks by D-pad and by touch are appended, Y hands the pen back and B stops with the co-written text kept — verified by `tests/hw/ui-cowrite.md`
25. [verified] Three paths are offered, a picked path (by D-pad or touch) is appended and the next paths continue it coherently — verified by `tests/hw/ui-paths.md`
26. [verified] Stories and history entries open as rotated book pages with page numbers and close back to their screen — verified by `tests/hw/ui-book.md`

## Maturity

**MVP (committed):**

- Bitmap screens in the DSLM palette and pixel font, logo splash and banner icon, own QWERTY keyboard typed by D-pad or touch, streaming and cancellation by a B tap (ADR 0011, ADR 0012).
- Seven screens (Loading, Home, Prompt, Generating, Story, Settings, About), Stories and Chat modes remembered in /dslm/dslm.ini, example prompts on L/R, every action on buttons and touch (ADR 0013, after ADRs 0011 and 0006).
- Motion by hardware only: brightness fades between screens and a pulsing brain while generating; chat in speech bubbles (ADR 0013).
- Original interface sounds synthesized on the PSG channels, with a Sound switch in Settings (ADR 0014).
- History of finished stories and answers in /dslm/history, browsed from the Home screen.
- Personal touches from the DS settings: a greeting with the user's name, a story example with it, and an optional accent in their favourite colour.
- Battery mode: the lid can close during a generation (screens off, model keeps writing, sleep when done); low battery shown in the title bar.
- Benchmark card: the standard run on the loaded model, its results on screen and in /dslm/bench.txt.
- Peek: the five likely next tokens with probability bars while a story model writes (X).
- Co-writing: the user picks the next tokens among the five likely ones (Y).
- Choose-your-path: a finished story goes on with three alternative continuations to pick from (X on the Story screen).
- Book mode: stories and history entries read on rotated pages across both screens.
- Settings screen for temperature, top-p and steps, saved to /dslm/dslm.ini.

**Future (aspirational, not committed):**

- Pixel-smooth scrolling.

## Out of scope for this spec

- Any inference logic.
