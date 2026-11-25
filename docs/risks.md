# Risks

| # | Risk | Impact | Mitigation |
|---|---|---|---|
| R1 | The R4 in hand is a clone with an unknown kernel | Binary does not launch or cannot mount the card | Identify the cart first (`docs/r4-flashcart.md`); reference cart per ADR 0009; record cart + kernel in every benchmark |
| R2 | DLDI auto-patch fails | `fatInitDefault()` fails on hardware only | Clear on-screen hint; manual `dlditool` patch; driver sources from `DS-Homebrew/DLDI` |
| R3 | Kernel rejects the binary (R4iMenu vs calico) | Nothing runs on DEMON clones | BlocksDS (ADR 0004); alternative kernels listed |
| R4 | Soft-float fp32 build is far too slow | Parity run takes very long on hardware | Expected: fp32 exists for correctness only (ADR 0003); short parity prompts and step limits |
| R5 | int8 misses the SC3 target | Weak headline result | ITCM kernels, DSP MAC instructions, hardware divider/sqrt, study ds-llm and chatds; SC3 is provisional until the first measurement |
| R6 | Emulator numbers mistaken for hardware numbers | Misleading claims | `emulated=yes` label; harness must-not rule; SC6 |
| R7 | Memory budget tighter than estimated | Model or KV cache does not fit | `pc-toolchain` inspect budget before staging; shrink `seq_len`; measure heap peak |
| R8 | Original R4 and ≤2 GB microSD cards are hard to buy | No reference hardware | Accept a documented clone as reference via a new ADR that supersedes 0005 |
| R9 | Legal status of R4 carts in some countries | Distribution concerns | Ship only DSLM's own code; no kernels, firmware or ROMs (ADR 0009, which keeps ADR 0005's policy) |
| R10 | Third-party code licenses (llama2.c, ds-llm, chatds) | Licensing conflict | Keep MIT notices on ported files; verify each license before reusing code |
