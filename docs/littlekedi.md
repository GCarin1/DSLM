# LittleKedi on the DS (milestone M8)

[LittleKedi-tiny-instruct](https://huggingface.co/edededdy/LittleKedi-tiny-instruct) (Apache-2.0) is a 2.0M-parameter chat model in the style of DeepSeek-V3. It answers in a chat format, but its answers are often wrong or vague; this milestone brings a real chat mode to the DS anyway, as an experiment.

## Architecture

| Part | LittleKedi |
|---|---|
| Layers | 4 (the first dense, SwiGLU 352; the others mixture of experts) |
| Attention | Multi-head latent attention: 4 heads of 32 (16 without RoPE + 16 with interleaved RoPE), values of 16; keys and values come from a normalised 32-wide latent plus a 16-wide shared RoPE key |
| Experts | 8 routed (SwiGLU 64), top 2 by `sigmoid(score) + bias`, weights normalised to sum 1; plus 1 shared expert |
| Vocabulary | 8192 GPT-2 byte-level BPE tokens + 4 chat markers (`<|system|>`, `<|user|>`, `<|assistant|>`, `<|end|>`) |
| Embedding | 8196 × 128, tied with the output layer |
| Recommended sampling | temperature 0.7, top-p 0.9, repeat penalty 1.15 |

The cache keeps only the latent and the RoPE key: 48 values per token per layer, which is what makes the model fit next to its weights.

## Step 1: reference engine (done)

- `tools/m8/convert_littlekedi.py` turns the Hugging Face files into `littlekedi.bin` (fp32, every matrix `[out][in]`; the experts are transposed) and `littlekedi.tok` (raw token bytes, chat markers, merges by rank). `tools/dslm.py` fetches the files pinned in `tools/models.json` and converts them.
- `tools/m8/littlekedi_ref.py` is the numpy oracle. It equals llama.cpp's greedy output on the F16 GGUF once llama.cpp's repeat penalty (1.15, read from the GGUF) is turned off; layer by layer the activations agree to about 1e-3 (F16 weights).
- `src/engine/mla.c` and `src/engine/bpe.c` are the fp32 C engine and tokenizer; `tools/tests/test_mla_host.py` checks them against the oracle's golden outputs (`tests/golden/littlekedi_ref.json`), token for token.

Chain of evidence: llama.cpp ≡ numpy oracle ≡ C engine.

## Step 2: fixed-point engine (done)

- `tools/m8/convert_littlekedi.py --quantize` writes `littlekedi_q80.bin` (MLQ1, 2.25 MB): every matrix int8 with one fp32 scale per group of 32, the tied embedding included; vectors and the expert router stay fp32 and become Q16 at load.
- `src/engine/mlq_fixed.arm.c` is the integer forward pass. It shares its kernels (`src/engine/fxkern.arm.c`: RMSNorm, activation quantization, the ITCM matmul, softmax, sigmoid, sampler) with the llama2 int8 engine, whose goldens stay bit-exact.
- The sampler applies llama.cpp's repeat penalty (positive logits divided, negative multiplied, each recent token once) before temperature and top-p; without it the model loops ("a cat's cat's cat's…").
- **Fidelity** (ADR 0010): reading the fp32 golden sequences, the int8 engine picks the same token as the fp32 engine at 96.2% of positions, with 2.4% mean logit error. Exact sequences can still split early: a mixture-of-experts router can face two experts 0.001 apart, and any 8-bit rounding flips the choice. llama.cpp's Q8_0 file splits on the same prompts and gives the same answers as this engine.
- Runtime memory beyond the model: about 115 KB plus 768 bytes per context token (the latent and RoPE-key cache of 4 layers), 904 KB at the full 1024 tokens.

## Step 3: chat on the DS (done in melonDS)

- The loader recognises MLQ1 by its magic, reads the tokenizer file named in its header from the same folder, and sizes the context from the RAM left after the files (at least 128 tokens are required, up to 1024). The Settings picker lists it as `chat`.
- Chat mode (`src/engine/mlq_chat.c`, shared by the DS and the host driver): the typed question becomes `<|user|>question<|end|><|assistant|>`, the answer stops at `<|end|>`, and the repeat penalty 1.15 is always on. Each question is a new, single-turn chat; the top screen shows `> question` and the answer.
- melonDS: 40/40 tokens identical to the host golden on "What causes the seasons?" (`tests/golden/littlekedi_q80.json`, run by `tests/parity/run_melonds.py --model littlekedi`), about 3 tokens/s emulated.
- The package ships `littlekedi_q80.bin` and `littlekedi.tok` with the Apache License 2.0 in `THIRD_PARTY.txt`.

## Step 4: hardware (done)

On the reference R4 (YSMenu) the DS generated 256 answer tokens at **2.81 tokens/s**, heap peak 3,420,420 bytes, load 0.56 s, and all 263 tokens matched the host build (`benchmarks/2026-11-25-r4-ysmenu-littlekedi-chat-int8.txt`, `tests/parity/littlekedi-chat.md`). The estimate in the M8 plan was 2 to 3 tokens/s.
