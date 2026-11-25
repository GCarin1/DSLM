# Tasks — Change 0019-m8-littlekedi-chat-on-the-ds

- [x] MLQ1 header names its tokenizer; context sized from free RAM (mlq_fit_seq); BPE1 loader hardened
- [x] Shared generation loop src/engine/mlq_chat.c (DS app and host driver); chat mode in src/main.c with example questions
- [x] Picker lists chat models; package ships littlekedi_q80.bin + littlekedi.tok with the Apache License 2.0
- [x] Golden runs at the DS defaults; melonDS parity 40/40; docs, contract, changelog
