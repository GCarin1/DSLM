# Change ledger

One line per archived change, newest last. Appended by
`doctrina change archive`; edit freely, the CLI only appends.

- 2026-11-24 — 0001-r4-research-and-delivery-roadmap — R4 research and delivery roadmap (specs: model-loading MODIFIED, pc-toolchain MODIFIED, test-harness ADDED)
- 2026-11-25 — 0002-m1-toolchain-card-blocksds — M1 toolchain and card I/O: BlocksDS project skeleton with a UI-ready layered architecture (ui/engine/storage separation) and a hello-world .nds that reads /dslm/dslm.ini via DLDI (specs: ds-interface MODIFIED, model-loading MODIFIED)
- 2026-11-25 — 0003-m2-pc-toolchain — M2 PC toolchain (specs: pc-toolchain MODIFIED)
- 2026-11-25 — 0004-m3-fp32-skeleton — M3 fp32 skeleton (specs: inference-engine MODIFIED, model-loading MODIFIED)
- 2026-11-25 — 0005-m6-interface — M6 interface (specs: ds-interface MODIFIED)
- 2026-11-25 — 0006-install-package — Install package (specs: pc-toolchain MODIFIED)
- 2026-11-25 — 0008-m4-hardware-results — M4 hardware results (specs: inference-engine MODIFIED, model-loading MODIFIED)
- 2026-11-25 — 0007-m5a-int8-path — M5a int8 path (specs: fixed-point-inference MODIFIED, pc-toolchain MODIFIED)
- 2026-11-25 — 0009-spec-sync-for-stop-format-probe-and-tokens-txt — Spec sync for stop, format probe and tokens.txt (specs: inference-engine MODIFIED, model-loading MODIFIED)
- 2026-11-25 — 0010-m5b-fixed-point-engine — M5b fixed-point engine (specs: fixed-point-inference MODIFIED, pc-toolchain MODIFIED)
- 2026-11-25 — 0011-mit-license — MIT license (specs: pc-toolchain MODIFIED)
- 2026-11-25 — 0012-m5-hardware-results — M5 hardware results (specs: fixed-point-inference MODIFIED)
- 2026-11-25 — 0013-test-harness — Test harness (specs: test-harness MODIFIED)
- 2026-11-25 — 0015-model-picker — Model picker (specs: ds-interface MODIFIED, model-loading MODIFIED)
- 2026-11-25 — 0014-v1-0-0-release — v1.0.0 release (specs: ds-interface MODIFIED, inference-engine MODIFIED, model-loading MODIFIED, pc-toolchain MODIFIED, test-harness MODIFIED)
- 2026-11-25 — 0016-tiny1m-in-the-package — tiny1m in the package (specs: fixed-point-inference MODIFIED, pc-toolchain MODIFIED)
- 2026-11-25 — 0017-m8-littlekedi-reference-engine — M8 LittleKedi reference engine (specs: inference-engine MODIFIED)
- 2026-11-25 — 0018-m8-fixed-point-mla-engine — M8 fixed-point MLA engine (specs: fixed-point-inference MODIFIED)
- 2026-11-25 — 0019-m8-littlekedi-chat-on-the-ds — M8 LittleKedi chat on the DS (specs: ds-interface MODIFIED, model-loading MODIFIED)
- 2026-11-25 — 0020-release-v1-2-0 — Release v1.2.0
- 2026-11-26 — 0021-graphical-ui-base-visual — Graphical UI base and visual identity: replace the libnds text console with a tiled-graphics renderer (custom pixel font, palette from the DSLM logo, LZ77 assets embedded in the .nds and decompressed to VRAM via grit), a splash screen with the logo, a 32x32 banner icon, and the existing five screens redrawn 1:1 in the new look; supersedes ADR 0006's console decision; must keep tok/s within 2%, binary growth within 64 KB and LittleKedi loadable (specs: ds-interface MODIFIED)
- 2026-11-26 — 0022-custom-qwerty-keyboard-d-pad — Custom QWERTY keyboard with D-pad and touch typing: replace the libnds keyboard on the prompt screen with a keyboard drawn in the DSLM style, navigable with the D-pad (A types the focused key, B deletes, Y types a space, X toggles shift/symbols, L/R example prompts, START generates, SELECT settings) and by touch; input sampled in the VBlank interrupt and latched so a short B tap stops generation; keyboard layout and focus movement are pure C tested on the host (specs: ds-interface MODIFIED)
- 2026-11-26 — 0023-home-menu-motion-after — Home menu and motion: after the splash a Home screen with Stories, Chat, Settings and About cards (D-pad + A or touch); choosing Chat loads the chat model and Stories loads the story model from Settings, with the existing load-and-fallback; the last mode is saved in dslm.ini and focused on the next boot; B returns to Home from the prompt; an About screen with the logo, version and models on the card; motion: fade transitions between screens, a pulsing brain while generating, and chat questions and answers drawn as bubbles (specs: ds-interface MODIFIED)
- 2026-11-26 — 0024-sound-effects-original-sounds — Sound effects: original sounds synthesized on the DS PSG channels (no samples, no RAM) for interface actions and moments — key click, focus move, delete, page toggle, open/confirm, back, start and stop generation, story done, error, and a boot chime — played by a small sequencer in the VBlank interrupt; a Sound on/off row on the Settings screen saved in dslm.ini; the sequencer stepping and the sound tables are pure C tested on the host (specs: ds-interface MODIFIED)
- 2026-11-26 — 0025-history-save-every-finished — History: save every finished story or chat answer to /dslm/history on the microSD with its date from the DS clock, model, prompt and text; a History card on the Home screen lists them newest first, shows the selected one on the top screen (chat in bubbles), scrolls it with L/R and deletes an entry with Y pressed twice; the file format and its parser are pure C tested on the host (specs: ds-interface MODIFIED)
