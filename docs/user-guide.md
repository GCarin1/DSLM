# DSLM user guide

DSLM runs a tiny language model (stories260K) on a Nintendo DS with no internet connection. It writes short children's stories in English from the first words you give it.

## What goes on the microSD card

The DS never downloads anything. The model travels on the card with the program:

```
/dslm.nds                     the program (any folder your kernel can launch)
/dslm/dslm.ini                settings (optional; defaults are used without it)
/dslm/models/stories260K_q80.bin  the model, int8, about 0.5 MB (default)
/dslm/models/stories260K.bin      the same model in fp32, about 1 MB (reference)
/dslm/models/tok512.bin       the tokenizer, about 6 KB
```

Developers build this tree with `python tools/dslm.py stage <card root>` (`docs/building.md`). If a file is missing, the DS shows which one and stops.

## Screens

1. **Loading.** A progress bar while the model is read into memory.
2. **Prompt.** The top screen shows a welcome text (later, your last story). The bottom screen shows your prompt, four buttons and the touch keyboard.
3. **Generating.** The story appears word by word on the top screen. The bottom screen shows progress, the time per token and a big STOP button.
4. **Story.** The finished story on the top screen; New prompt, Same prompt again and Settings below.
5. **Settings.** Temperature, top-p, the number of steps and the model, saved to `/dslm/dslm.ini`.

## Controls

Every action works with the buttons or with the stylus.

| Screen | Action | Buttons | Touch |
|--------|--------|---------|-------|
| Prompt | Type | — | touch keyboard |
| Prompt | Delete a character | B | Bksp |
| Prompt | Previous / next example prompt | L / R | `<` / `>` |
| Prompt | Generate | START or A | GO (or Rtrn) |
| Prompt | Settings | SELECT | SET |
| Prompt | Scroll the last story | Up / Down | — |
| Generating | Stop and keep the text | hold B | STOP |
| Story | New prompt (keeps the old one for editing) | A | New prompt |
| Story | Same prompt again | Y | Same prompt again |
| Story | Scroll | Up / Down | — |
| Story | Settings | SELECT | Settings |
| Settings | Choose a setting | Up / Down | touch its row |
| Settings | Change it | Left / Right | `[-]` / `[+]` |
| Settings | Pick another model (row `Model`) | Left / Right, then START | `[<]` / `[>]`, then Save |
| Settings | Save to the card | START | Save |
| Settings | Back without saving | B | Back |

Typing is touch-only for now; without the stylus, L/R pick an example prompt and START generates.

## Settings

- **Temperature** 0 always picks the most likely next word, so the same prompt gives the same story. Higher values (up to 2.0) give more varied stories.
- **Top-p** limits sampling to the most likely words when temperature is above 0.
- **Steps** is the story length in tokens (16 to 512). The int8 model writes about 6 to 7 tokens per second on a DS; later tokens take a little longer.
- **Model** lists every checkpoint in `/dslm/models/` (fp32 or int8, `llama2.c` format). START unloads the current one and loads the new one; if it does not fit in memory or cannot be read, the top screen says why and the previous model comes back. All models must use the tokenizer named in `dslm.ini` (`tok512.bin` for the included ones).
