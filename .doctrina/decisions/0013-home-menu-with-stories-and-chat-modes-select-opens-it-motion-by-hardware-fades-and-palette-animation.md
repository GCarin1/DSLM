# ADR 0013 — Home menu with Stories and Chat modes; SELECT opens it; motion by hardware fades and palette animation

- **Status:** accepted
- **Date:** 2026-11-26
- **Deciders:** Carini (chose the Home menu, with Chat loading its model by itself, on 2026-11-26)
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** `src/main.c`, `src/ui/ui_gfx.c`, `src/storage/ini.c`, `.doctrina/contracts/model-file-format.md`
- **Landed:** 2026-11-26 — `src/main.c`, `src/ui/ui_gfx.c`, `tests/hw/ui-home.md`

## Context

After ADRs 0011 and 0012 the DS booted straight into the prompt of whatever model `dslm.ini` named. Story and chat models were picked from the same Settings list, and nothing explained what DSLM was. The deciders asked for a friendlier, smoother flow: a Home screen where Chat loads its model by itself, an About screen, and motion. This is change 3 of the graphical interface update.

## Decision

- **Flow.** After the splash comes Home, which shows the logo on top and four cards below: Stories, Chat, Settings and About. The D-pad and A (or START) open a card, and so does a touch.
  - **Stories** and **Chat** load their mode's model only when the other kind is loaded, using the existing load-with-fallback, then open the prompt.
  - **Settings** and **About** return to Home.
  - SELECT (and the MENU/Menu buttons) returns to Home from the prompt and story screens, so Settings now lives under Home.
- **Modes in `dslm.ini`** (contract `model-file-format`).
  - `checkpoint` stays the story model. The new `chat_checkpoint` is the chat model, and the new `[ui] mode = stories|chat` records the card used last. The DS loads that mode's model at boot and focuses its card.
  - An old file whose `checkpoint` is a chat model is read as Chat mode. The mode follows the kind that actually loaded.
  - The Settings model row lists story models only.
- **Motion, all by hardware so generation does not slow.**
  - Screen changes fade through the master brightness, 4 frames each way.
  - While generating, the brain icons breathe between dark and light cyan. Their palette entry (`COL_PULSE`, 48) is rewritten in the VBlank interrupt.
  - Text is not animated per pixel.
- **Chat layout.** The question and the answer are drawn as bubbles: the question in cyan on the right, the answer in dark blue on the left. The question's length is passed with `ui_set_chat()`, and no `> ` marker goes into the text.

## Alternatives considered

1. **No Home; keep booting into the prompt.** Fewer screens, but chat stays hidden in Settings and there is no place for About. The deciders chose the Home menu.
2. **B returns to Home from the prompt.** B already deletes characters there (ADR 0012). SELECT is free and means "menu" on the other screens too.
3. **Sliding transitions by background scroll.** Both screens are full bitmaps, so a slide needs a second buffer or a redraw per frame. A brightness fade costs nothing.
4. **Animating with sprites or per-frame redraws during generation.** It would compete with the engine for CPU time. Palette animation takes a few register writes per frame.

## Consequences

**Positive**

- Chat is one card away and remembered across boots. About shows the model, its context and the free RAM.
- Measured on melonDS: int8 256 steps took 35,503 ms, the same as change 0021, so the pulse and the input interrupt cost nothing measurable. LittleKedi had a heap peak of 3,420,968 B and parity 40/40. The binary is 219,136 bytes (v1.2.0: 217,088).

**Negative**

- One more press to reach the prompt after boot (START on the focused card). The melonDS parity runner now presses START twice.
- Settings is reached through Home, no longer straight from the prompt.

**Neutral**

- The fades add about 140 ms per screen change.

<!--
Once this ADR is accepted, do not edit it. To change the decision,
create a new ADR that supersedes this one and update the "Superseded by"
header above to point at the new ADR. Status transitions:
proposed -> accepted | rejected
accepted -> deprecated | superseded by NNNN
-->
