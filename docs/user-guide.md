# DSLM user guide

DSLM runs tiny language models (stories260K by default, tiny1m as an option) on a Nintendo DS with no internet connection. It writes short children's stories in English from the first words you give it. An experimental chat model, LittleKedi, answers questions instead.

## What goes on the microSD card

The DS never downloads anything. The model travels on the card with the program:

```
/dslm.nds                     the program (any folder your kernel can launch)
/dslm/dslm.ini                settings (optional; defaults are used without it)
/dslm/models/stories260K_q80.bin  the model, int8, about 0.5 MB (default)
/dslm/models/tiny1m_q80.bin       a larger model, int8, about 1 MB (richer stories, slower)
/dslm/models/stories260K.bin      the same model in fp32, about 1 MB (reference)
/dslm/models/tok512.bin       the tokenizer of the story models, about 6 KB
/dslm/models/littlekedi_q80.bin   the chat model, int8, about 2.2 MB (experimental)
/dslm/models/littlekedi.tok       its tokenizer, about 170 KB (must sit next to it)
/dslm/history/                    saved stories and answers (written by the DS)
```

Developers build this tree with `python tools/dslm.py stage <card root>` (`docs/building.md`). If a file is missing, the DS shows which one and stops.

## Screens

1. **Loading.** The DSLM logo on the top screen; a progress bar and the loading steps below while the model is read into memory.
2. **Home.** The logo on top and five cards below (with a greeting when your name is set in the DS System Settings): **Stories**, **Chat**, **History**, **Settings** and **About**. Stories and Chat load their model if the other one is loaded, then open the prompt. The DS remembers the last one and starts with its card highlighted.
3. **Prompt.** The top screen shows a welcome text (later, your last story). Its title bar shows the model and its state, and the bar at the bottom shows the buttons to press. The bottom screen shows your prompt, four buttons and the touch keyboard.
4. **Generating.** The story appears word by word on the top screen (in chat, the question and the answer appear as speech bubbles). The bottom screen shows progress, the time per token, a pulsing brain and a big STOP button.
5. **Story.** The finished story on the top screen; New prompt, Same prompt again and Menu below.
6. **Settings** (from Home). Temperature, top-p, the number of steps and the story model, saved to `/dslm/dslm.ini`.
7. **History** (from Home). Every finished story and answer is saved on the card with its date; pick one to read it again (chats in bubbles), or delete it.
8. **About** (from Home). The loaded model, its context, the free memory and the credits.

## Controls

Every action works with the buttons or with the stylus.

| Screen | Action | Buttons | Touch |
|--------|--------|---------|-------|
| Home | Choose a card | D-pad | — |
| Home | Open it | A or START | touch the card |
| Prompt | Move on the keyboard | D-pad | — |
| Prompt | Type the highlighted key | A | touch the key |
| Prompt | Delete a character | B | DEL |
| Prompt | Space | Y | space |
| Prompt | Capital letter (next one only) | X | Aa |
| Prompt | Numbers and symbols | — | ?123 (abc goes back) |
| Prompt | Previous / next example prompt | L / R | `<` / `>` |
| Prompt | Generate | START | GO |
| Prompt | Back to Home | SELECT | MENU |
| Generating | Stop and keep the text | B | STOP |
| Story | New prompt (keeps the old one for editing) | A | New prompt |
| Story | Same prompt again | Y | Same prompt again |
| Story | Scroll | Up / Down | — |
| Story | Back to Home | SELECT | Menu |
| History | Pick an entry | Up / Down | touch its row |
| History | Scroll the text | L / R | — |
| History | Delete it (press twice) | Y | Delete |
| History | Back to Home | B | Back |
| About | Back to Home | B | Back |
| Settings | Choose a setting | Up / Down | touch its row |
| Settings | Change it | Left / Right | `[-]` / `[+]` |
| Settings | Sounds on or off (row `Sound`) | Left / Right, then START | `[<]` / `[>]`, then Save |
| Settings | Accent colour (row `Colour`) | Left / Right, then START | `[<]` / `[>]`, then Save |
| Settings | Pick another model (row `Model`) | Left / Right, then START | `[<]` / `[>]`, then Save |
| Settings | Save to the card | START | Save |
| Settings | Back to Home without saving | B | Back |

Without the stylus, the D-pad moves the highlight over the keyboard and A types the key; `?123`, `GO` and `DEL` can be reached the same way. A short press of B is enough to stop: it is noticed after the current word.

## Settings

- **Colour** picks the accent: the DSLM cyan, or "My DS", the favourite colour chosen in the DS System Settings.
- **Sound** switches the interface sounds (clicks, chimes) on or off. They are original sounds made by the DS sound chip itself.
- **Temperature** 0 always picks the most likely next word, so the same prompt gives the same story. Higher values (up to 2.0) give more varied stories.
- **Top-p** limits sampling to the most likely words when temperature is above 0.
- **Steps** is the story length in tokens (16 to 512). The int8 model writes about 6 to 7 tokens per second on a DS; later tokens take a little longer.
- **Model** lists every story checkpoint in `/dslm/models/` (fp32 or int8, `llama2.c` format; the chat model has its own Home card). In Stories mode START unloads the current one and loads the new one (in Chat mode it is saved for the next Stories visit); if it does not fit in memory or cannot be read, the top screen says why and the previous model comes back. All models must use the tokenizer named in `dslm.ini` (`tok512.bin` for the included ones).

## Which model?

| Model | Speed on a DS | Memory | Good for |
|---|---|---|---|
| `stories260K_q80` (default) | about 7 tokens/s | 1.4 MB | Quick, simple stories |
| `tiny1m_q80` | about 4.5 tokens/s | 2.4 MB | Longer, more varied stories with dialogue |
| `stories260K` (fp32) | about 1.6 tokens/s | 1.8 MB | Testing only (exact reference) |
| `littlekedi_q80` (chat) | about 2.8 tokens/s | 3.4 MB | Asking questions; answers are often wrong |

**Chat mode.** With `littlekedi_q80` the prompt is a question ("What causes the seasons?"); L/R cycle example questions. The top screen shows your question after `>` and the answer below it; the answer ends by itself, and Steps only caps its length. Each question starts a new chat (the model does not remember earlier ones). DSLM always applies the model author's repeat penalty (1.15) so the answer does not loop; temperature and top-p work as for stories. The model has 2M parameters: expect confident nonsense.

Open the **Chat** card on the Home screen; **Stories** goes back to the story model. The mode is saved in `/dslm/dslm.ini` (`[ui] mode`, with the chat model in `chat_checkpoint`), so the DS starts with it next time. To add your own model, copy a `llama2.c` checkpoint (fp32 or int8 v2, made with `python tools/dslm.py quantize`) into `/dslm/models/`; it must use the same tokenizer and fit in about 3.5 MB of RAM, or the DS refuses it and keeps the current model.
