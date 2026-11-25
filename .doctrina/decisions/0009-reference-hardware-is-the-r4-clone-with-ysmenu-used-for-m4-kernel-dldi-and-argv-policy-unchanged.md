# ADR 0009 — Reference hardware is the R4 clone with YSMenu used for M4; kernel, DLDI and argv policy unchanged

- **Status:** accepted
- **Date:** 2026-11-25
- **Deciders:** Carini (approved replacing ADR 0005's reference cart, 2026-11-25)
- **Supersedes:** 0005
- **Superseded by:** —
- **Evidence:** `benchmarks/2026-11-25-r4-ysmenu.txt`, `tests/hw/offline-run.md`, `docs/r4-flashcart.md`
- **Landed:** 2026-11-25 — `benchmarks/2026-11-25-r4-ysmenu.txt`, `tests/hw/offline-run.md`

## Context

ADR 0005 named an original R4 (r4ds.com) or M3 Simply running Wood R4 1.62 with a microSD of 2 GB or less as the reference cart. That hardware is discontinued and was never in hand. The first real-hardware run (M4) used the project owner's cart instead: an R4 clone booting `R4.dat`/`TTMenu.dat` with the YSMenu kernel (the DSONEi SDHC / R4i-SDHC YSMenu package), whose `ttdldi.dat` DLDI-patches homebrew automatically. Its microSD reports 64 GB but holds a 3.8 GB FAT32 partition and is probably counterfeit; DSLM needs about 1.6 MB of it. On that cart the install package loaded in 249 ms, generated with the fp32 build at 627.5 ms per position, and matched `run.c` token for token. ADR 0005's other rules (no third-party kernels or DLDI in the repository, auto-patch with a `dlditool` fallback, no argv, avoid R4iMenu on timebomb clones, DS mode only) held throughout and are not in question.

## Decision

1. **Reference cart.** The reference test hardware is the R4 clone with the YSMenu kernel used for M4, with the card used within its real FAT32 partition (3.8 GB; DSLM needs about 1.6 MB). Hardware numbers in `benchmarks/` come from it unless a file names another cart. Other DLDI carts, including the original R4 with Wood R4 1.62, are supported best-effort and recorded with their cart, kernel and microSD.
2. **No kernels in the repository.** DSLM never redistributes kernels, firmware or DLDI binaries from third parties; the install package contains only DSLM's own files and the MIT-licensed model.
3. **DLDI.** Rely on the kernel's auto-patch. The documented fallback is a manual patch with `dlditool` and the cart's driver from `DS-Homebrew/DLDI`.
4. **No argv.** All settings come from `/dslm/dslm.ini` on the card.
5. **Avoid R4iMenu** on DEMON/timebomb clones. Use Pico-Launcher, AKMenu-Next or YSMenu there.
6. **DS mode only.** DSLM targets DS mode (67 MHz, 4 MB). Running on a DSi through TWiLight Menu++ is a bonus, not a requirement.

## Alternatives considered

1. **Keep ADR 0005 and buy an original R4.** Discontinued, sold used only, and needs microSD cards of 2 GB or less that are hard to find; the numbers would not come from the hardware actually used.
2. **No reference cart.** Benchmarks from different carts and kernels could not be compared.

## Consequences

**Positive**

- The reference matches the hardware that produced the first parity and timing results, so later numbers compare with them.
- YSMenu auto-patches DLDI, so no manual `dlditool` step was needed.

**Negative**

- A clone with a probably counterfeit microSD is less reproducible for other people than a well-known cart; every benchmark must name cart, kernel and card.
- Writes beyond the real capacity of a counterfeit card would corrupt data; DSLM stays far below it, and the card was formatted and checked clean before use.

**Neutral**

- The earlier FAT corruption on this card predates DSLM; it was fixed by formatting (backups on the owner's PC).
