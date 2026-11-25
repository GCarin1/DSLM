# ADR 0005 — R4 hardware target and kernel policy

- **Status:** superseded by 0009
- **Date:** 2026-11-24
- **Deciders:** Carini
- **Supersedes:** —
- **Superseded by:** 0009
- **Evidence:** n/a — no implementation yet
- **Landed:** —

## Context

"R4" is a label, not a hardware model. Dozens of clones (r4ds.com, r4ids.cn, r4isdhc.com, R4i Gold, Ace3DS+/R4iLS, "DEMON" timebomb carts, ...) ship different kernels and DLDI drivers. The community archive `flashcarts/flashcard-archive` maps clone families to their kernels. It is a collection of binary kernel packages with mixed, unclear licensing, not source code. The older `hurda/flashcard-archive` is a stale fork of the archived `DS-Homebrew/Flashcard-Firmware-Archive` and adds nothing for DSLM. Kernels replace themselves with the homebrew at launch and normally DLDI-patch it automatically, though auto-patching is known to fail on some carts. Clone kernels do not reliably pass launch arguments (argv). R4 carts are also illegal to sell or import in some countries (Japan, the UK, France).

## Decision

1. **Reference cart.** The reference test hardware is an original R4 (r4ds.com) or M3 Simply running the Wood R4 1.62 kernel, with a microSD card of 2 GB or less, formatted FAT. It has a verified DLDI driver (`r4tf.dldi`) on both ARM9 and ARM7. Other DLDI carts are supported best-effort and recorded in the benchmarks with their cart and kernel names.
2. **No kernels in the repository.** DSLM never redistributes kernels, firmware or DLDI binaries from third parties. The docs link to the flashcart guides and the archive instead.
3. **DLDI.** Rely on the kernel's auto-patch. The documented fallback is a manual patch with `dlditool` and the cart's driver from `DS-Homebrew/DLDI`.
4. **No argv.** All settings come from `/dslm/dslm.ini` on the card.
5. **Avoid R4iMenu** on DEMON/timebomb clones. Use Pico-Launcher, AKMenu-Next or YSMenu there.
6. **DS mode only.** DSLM targets DS mode (67 MHz, 4 MB). Running on a DSi through TWiLight Menu++ is a bonus, not a requirement.

## Alternatives considered

1. **Target a specific modern clone (e.g. R4i Gold 3DS Plus).** More common to buy today, but slower SD reads and more kernel variance.
2. **Target DSpico / modern open flashcarts.** Better tooling, but the project's goal is the classic R4.
3. **Ship a known-good kernel bundle in the repository.** Convenient, but it would redistribute proprietary firmware.

## Consequences

**Positive**

- One known-good hardware baseline for parity and benchmark claims.
- No licensing exposure from third-party firmware.

**Negative**

- The original R4 is discontinued. It has to be bought used, and SD cards of 2 GB or less are becoming rare.

**Neutral**

- The benchmarks folder doubles as a compatibility list of carts and kernels.
