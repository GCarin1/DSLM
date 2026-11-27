# Changelog

All notable changes to DSLM are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/). Each entry links to the Doctrina change that delivered it (`.doctrina/changes/archive/`).

## [1.3.0-1] - 2026-11-27

### Changed

- Parallel build of 1.3.0 from the `release/1.3.0-1` branch: the same models, settings and card layout.

## [1.3.0] - 2026-11-26

### Changed

- **New look** (change 0021, ADR 0011): both screens are drawn as graphics in the colours of the DSLM logo, with an original pixel font (about 40 characters per line instead of 32), rounded buttons labelled with the DS button that presses them, a title bar with the model and its state, and a splash with the logo while a model loads. The flashcart menu shows a brain icon and "Run LLMs on Nintendo DS". Controls are unchanged.
- The top screen shows 14 lines of text instead of 20 (more fit on each line).
- **New keyboard** (change 0022, ADR 0012): a QWERTY keyboard in the same style with keys twice as large, typed by touch or without the stylus: the D-pad moves the highlight and A types the key; B deletes, Y adds a space, `Aa` makes the next letter a capital (X also did until change 0033), `?123` opens numbers and symbols. On the prompt screen START (or GO) generates; A now types.
- A short press of B stops a generation; it no longer has to be held.
- **Home screen** (change 0023, ADR 0013): after the logo, four cards: Stories, Chat, Settings and About. Chat loads LittleKedi by itself and Stories goes back to the story model; the DS remembers the last one (`[ui] mode` and `chat_checkpoint` in `dslm.ini`) and starts with its card highlighted. SELECT returns to Home from the prompt and story screens; Settings is opened from Home and lists the story models.
- Chat questions and answers appear as speech bubbles; screens fade into each other, and the brain icon pulses while the model writes.

### Added

- **Voice mode** (change 0033, ADR 0015): X on the prompt screen turns the microphone on with a level meter; blow on the DS (or speak loudly) for a moment and the story starts, and the louder it was, the more creative that story (temperature 0.2 to 1.2, your settings unchanged). Shift moved to the `Aa` key only.
- **Book mode** (change 0032): START on the Story screen (or A in History) shows the text on light pages across both screens; hold the DS sideways like a book. A/R or a touch turn the page, Y/L go back, B closes. The rotation is done by the DS graphics hardware.
- **What happens next?** (change 0031): after a story, X writes three different continuations; pick one (D-pad and A, or touch) and it is added, then three more follow, until B. The engine continues from its memory of the story (KV cache), proven to match an uninterrupted run.
- **Write together** (change 0030): press Y while a story model writes and pick each next token yourself among its five favourites (Up/Down and A, or touch); Y gives the pen back, B stops. Speed figures leave out the time you spend choosing.
- **Peek inside the model** (change 0029): press X while a story model writes to see the five tokens it found most likely at each step, with probability bars; the one it picked lights up. Remembered as `[ui] peek`; costs about 3% of speed while shown. Under the hood, the engine gained a read-only step hook and a fixed-point top-k (generation is unchanged when unused).
- **Benchmark card** (change 0028): measures your DS with the standard run (first example, temperature 0, 128 tokens) and shows tokens per second, time per token, load time and memory; the result is appended to `/dslm/bench.txt` for sharing, and your settings and history stay as they were.
- **Battery mode** (change 0027): close the lid while the model writes and the screens turn off while it keeps going; when it is done the DS chimes and sleeps, and opening it shows the result. A red battery icon in the title bar warns when the battery is low.
- **Personal touches** (change 0026): DSLM greets you by the name set in the DS System Settings (Home and welcome text) and uses it in one story example; the new `Colour` row in Settings switches the accent to your DS favourite colour (`[ui] accent` in `dslm.ini`).
- **History** (change 0025): every finished story and chat answer is saved to `/dslm/history` on the microSD card with its date and time. The new History card on the Home screen lists them newest first, shows the chosen one again (chats in bubbles), scrolls it with L/R and deletes it with Y pressed twice.
- **Sounds** (change 0024, ADR 0014): clicks, blips and chimes for typing, moving, opening, going back, starting and stopping a generation, a finished story, errors and the boot. They are original and made by the DS sound chip itself (no recordings, no extra memory). Settings has a `Sound` row to switch them off (`[ui] sound` in `dslm.ini`).

### Fixed

- Touching Back on the Settings screen could type a character on the keyboard of the prompt screen.

### Hardware results

Measured with the Benchmark card (first example, temperature 0, 128 steps) on a DSi in DS mode with the reference R4 clone (YSMenu kernel), `emulated=no`:

| Model | Tokens/s | Heap peak | File |
|---|---|---|---|
| stories260K int8 | 7.68 | 1,374,340 B | `benchmarks/2026-11-26-r4-ysmenu-dsi-v1-3-dev-int8.txt` |
| tiny1m int8 | 4.93 | 2,418,820 B | `benchmarks/2026-11-26-r4-ysmenu-dsi-v1-3-dev-tiny1m-int8.txt` |
| LittleKedi chat int8 | 3.13 | 3,418,244 B | `benchmarks/2026-11-26-r4-ysmenu-dsi-v1-3-dev-littlekedi-chat-int8.txt` |
| stories260K fp32 | 1.76 | 1,783,940 B | `benchmarks/2026-11-26-r4-ysmenu-dsi-v1-3-dev-fp32.txt` |

int8 parity on the hardware: 129/129 tokens against the golden. The lid, battery icon, microphone and sounds were checked on the same DSi.

## [1.2.0] - 2026-11-25

### Added

- **Chat mode with LittleKedi** (experimental): [edededdy/LittleKedi-tiny-instruct](https://huggingface.co/edededdy/LittleKedi-tiny-instruct) (2.0M parameters, Apache-2.0), a DeepSeek-V3-style model (multi-head latent attention, mixture of experts, GPT-2 tokenizer). Pick `littlekedi_q80` in Settings, type a question, START. Its answers are often wrong; it is the first chat model on DSLM (changes 0017, 0018, 0019).
- Engines: an fp32 reference (`src/engine/mla.c`, equal to llama.cpp and a numpy oracle), the integer-only int8 engine (`src/engine/mlq_fixed.arm.c`) with a repeat penalty, a GPT-2 BPE tokenizer (`src/engine/bpe.c`); the integer kernels are shared with the llama2 engine (`src/engine/fxkern.arm.c`).
- File formats MLQ1 and BPE1 (`contracts/model-file-format`); the context is sized from the free RAM at load (up to 1024 tokens).
- `THIRD_PARTY.txt` carries the Apache License 2.0 for the chat model.

### Fixed

- Opening Settings from the story screen and switching between a story and a chat model kept the old prompt; the prompt now switches to the new model's first example from either screen.

### Hardware results

| Model | Tokens/s | Heap peak | Parity | File |
|---|---|---|---|---|
| LittleKedi (chat, int8) | 2.81 | 3,420,420 B | 263/263 vs host build | `benchmarks/2026-11-25-r4-ysmenu-littlekedi-chat-int8.txt` |

melonDS also matched the golden on "What causes the seasons?" (40/40 tokens).

## [1.1.0] - 2026-11-25

### Added

- **tiny1m in the package**: [shibatch/tiny1m](https://huggingface.co/shibatch/tiny1m) (about 0.9M parameters, MIT), quantized to int8 (`tiny1m_q80.bin`, 1.0 MB). Richer stories than stories260K at 4.4 tokens/s on a real DS, 2.4 MB of RAM; pick it on the Settings screen (change 0016).
- **Model picker** on the Settings screen, already described under 1.0.0, now ships with two int8 models to choose from.
- Toolchain: `tools/models.json` lists every packaged model with its author and license; `quantize`, `stage`, `package` and `golden --model` handle them all; `THIRD_PARTY.txt` lists each model.

### Changed

- The default model stays `stories260K_q80.bin` (fastest).
- Parity tests run per model: exact against the engine's host build for both, and a drift bound against `runq.c` of 64 generated tokens for stories260K and 6 for tiny1m (measured minimum 7; its stories stay coherent but leave the float path earlier).

### Hardware results

| Model | Tokens/s | Heap peak | Parity | File |
|---|---|---|---|---|
| tiny1m (int8) | 4.44 | 2,420,420 B | 60/60 vs host build | `benchmarks/2026-11-25-r4-ysmenu-tiny1m-int8.txt` |
| stories260K (int8) | 6.87 | 1,372,492 B | 197/197 vs host build | `benchmarks/2026-11-25-r4-ysmenu-v1.0.0-int8.txt` |

## [1.0.0] - 2026-11-25

First release: a tiny Llama 2 language model that runs 100% offline on a Nintendo DS launched from an R4 flashcart.

### Added

- **Fixed-point int8 engine** (default): integer-only forward pass, int8 weights, matmul in ITCM; 6.55 tokens/s on a real DS with an R4, 4.1x the fp32 build (change 0010, ADR 0008).
- **fp32 reference engine**: line-for-line port of `llama2.c`'s `run.c`, token-for-token identical to the PC (change 0004, ADR 0001).
- **Model picker**: the Settings screen lists the checkpoints in `/dslm/models/`, switches models without restarting and restores the previous one if the new one does not load (change 0015).
- **Interface**: touch keyboard, five screens (Loading, Prompt, Generating, Story, Settings), word-wrapped streaming, stop with B or STOP, example prompts on L/R, settings saved to `/dslm/dslm.ini`, tokens per second on the story screen; every action on buttons and on touch (changes 0005 and 0010, ADR 0006).
- **Model loading** from the microSD card through DLDI: format detection (fp32 or int8), size check, RAM budget before allocating, progress bar, clear errors for missing files and models that do not fit (changes 0002, 0004, 0007).
- **int8 model format**: `llama2.c` v2 (`ak42`), quantized from the pinned fp32 model with group size 4 (change 0007, ADR 0007).
- **Run records**: `/dslm/tokens.txt` after every generation and `/dslm/bench.txt` with timings and heap peak (changes 0004 and 0013).
- **PC toolchain** (`tools/dslm.py`, Python standard library only): fetch with pinned SHA-256, inspect and memory budget, quantize, stage a card, golden outputs, parity check, install package, benchmark import (changes 0003, 0006, 0007, 0013).
- **Install package**: one zip to unzip at the microSD root, with the model, README, manifest and licenses (changes 0006 and 0011).
- **Build and test environment**: Docker image on a pinned BlocksDS digest; host unit tests; parity tests against `run.c`, `runq.c` and the engine's host build; unattended melonDS runs; a binary check that the int8 path calls no soft-float routine (changes 0002, 0010, 0013).
- **Release workflow**: GitHub Actions builds, tests and publishes the release assets on a version tag (change 0014).
- MIT license (change 0011).

### Changed

- The default model is the int8 `stories260K_q80.bin`; the fp32 `stories260K.bin` ships next to it as the reference (change 0007, ADR 0007).
- Reference hardware is the R4 clone with the YSMenu kernel used for the hardware runs (ADR 0009, supersedes ADR 0005).

### Fixed

- Short button presses right after a screen change were lost while the keyboard slid in or out; the slide animation is off (change 0005).
- The title bar ran the model name into the state (`stories260K.bin[settings]`); it now keeps a space and drops the extension (change 0005).
- Generated text was cut in the middle of words on the 32-column screen; it is word-wrapped (change 0005).
- The settings printout at boot went missing in the M3 build; restored (change 0004).
- A normal end of the M1 build was shown as `ERROR`; it now ends cleanly (change 0002).
- With the group size `export.py` would pick (64), the int8 `w2` matmul would have silently used 128 of 172 inputs; the quantizer picks a group size that divides both inner sizes (change 0007).

### Hardware results

| Build | Load | Time per token | Tokens/s | Parity | File |
|---|---|---|---|---|---|
| int8 fixed point | 124 ms | 152.7 ms | 6.55 | 257/257 vs host build | `benchmarks/2026-11-25-r4-ysmenu-int8.txt` |
| fp32 reference | 249 ms | 627.5 ms | 1.59 | 153/153 vs `run.c` | `benchmarks/2026-11-25-r4-ysmenu.txt` |

[1.3.0-1]: https://github.com/GCarin1/DSLM/releases/tag/v1.3.0-1
[1.2.0]: https://github.com/GCarin1/DSLM/releases/tag/v1.2.0
[1.1.0]: https://github.com/GCarin1/DSLM/releases/tag/v1.1.0
[1.0.0]: https://github.com/GCarin1/DSLM/releases/tag/v1.0.0
