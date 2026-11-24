# Prior art

What already exists, and what DSLM takes from it. Figures marked *unverified* come from repository descriptions or third parties and should be checked against the source before relying on them.

## LLMs on the Nintendo DS

| Project | What it is | Relevance |
|---|---|---|
| [papayuh/chatds](https://github.com/papayuh/chatds) (MIT, BlocksDS) | 2.75M-parameter chat model on a DS Lite. Resident DSQ8 weights, int16 Q8.8 KV cache, 256-token context, SD access via DLDI | Closest reference: file format read from SD, headless melonDS smoke tests, heap measurement. Speed (~3.4–3.6 decode steps/s) was measured with melonDS's timer; its real-DS run (v0.2.0, via DSpico) recorded no timings |
| RileyGreiff/ds-llm | int8-quantized Llama 2 inference on the DS, numpy Q8_0 converter, on-device tok/s instrumentation (devkitPro) | Direct precedent for the `runq.c`-style build. *Unverified:* models, speed and license (reported as MIT) |
| RileyGreiff/nintendo-ds-llm, RileyGreiff/tie | TinyStories/Pokédex model on a DS Lite; an engine with a planned RAM ceiling | *Unverified*; worth reading before the int8 work |

**What this means for DSLM:** DSLM is not the first LLM on the DS. Its contribution is narrower and checkable: exact `llama2.c` parity on the DS, and benchmarks measured on a real R4 (SC2, SC6).

## `llama2.c` ports on other constrained hardware

| Port | Notes |
|---|---|
| [ytmytm/llama2.c64](https://github.com/ytmytm/llama2.c64) | Commodore 64 with 2 MB REU, stories260K |
| sixf0ur/llama2psp | PSP homebrew; replaces `mmap()` with file reads |
| psvita-llm | PS Vita, VitaSDK |
| [yeokm1/dosllam2](https://github.com/yeokm1/dosllam2) | DOS, stories260K + tok512 |
| [brysontang/iiama2.c](https://github.com/brysontang/iiama2.c) | Wii, stories260K + tok512 |
| 3DS port | stories15M Q8_0 at a reported 18.46 tok/s on a New 3DS (*unverified*) |

`stories260K` + `tok512` is the standard "hello world" across these ports, which makes cross-checking easy.

## Flashcart references

See `docs/r4-flashcart.md`, section 5.
