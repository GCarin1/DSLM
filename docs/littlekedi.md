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

## Next steps

1. **Fixed point for the DS**: int8 weights with group-wise scales like the existing engine; the embedding table stays int8 (expanded to Q16 it would take 4.2 MB); integer exp and sigmoid for attention and the router.
2. **Sampler**: temperature, top-p and the repeat penalty (without it the model loops).
3. **Chat mode in the interface**: wrap the typed text as `<|user|>…<|end|><|assistant|>`, stop at `<|end|>`, keep the conversation on screen.
4. **Hardware**: expected around 2 to 3 tokens/s and about 3 MB of RAM, to be measured.
