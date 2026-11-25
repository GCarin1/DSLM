# Tasks — Change 0017-m8-littlekedi-reference-engine

- [x] Read the architecture from the GGUF metadata and the safetensors layout
- [x] numpy oracle (tools/m8/littlekedi_ref.py); interleaved RoPE; equals llama.cpp with repeat penalty off
- [x] Converter to MLA1/BPE1 files; pins in tools/models.json; dslm.ensure_littlekedi()
- [x] fp32 C engine (src/engine/mla.c) and GPT-2 BPE tokenizer (src/engine/bpe.c), unaligned-safe
- [x] Golden from the oracle and host test; docs/littlekedi.md
