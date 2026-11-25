# ADR 0004 — Use BlocksDS as the DS toolchain

- **Status:** accepted
- **Date:** 2026-11-24
- **Deciders:** Carini
- **Supersedes:** —
- **Superseded by:** —
- **Evidence:** n/a — no implementation yet
- **Landed:** 2026-11-25 — `docker/Dockerfile`, `nds.mk`, `docs/building.md`

## Context

The intake left the toolchain open: devkitPro (devkitARM + libnds + libfat) or BlocksDS. Both changed substantially in 2024–2026.

- **devkitPro libnds v2 ("calico", November 2024, now 2.0.x)** moved DLDI permanently to the ARM7 and replaced libfat with libdvm + FatFs. It claims up to 2x faster DLDI reads. However, the stock R4iMenu kernel shipped on many "DEMON"/timebomb R4 clones does not launch calico-built homebrew, and some ARM9-only DLDI drivers hang when run on the ARM7.
- **BlocksDS** is a libnds fork with picolibc and FatFs, under active development. It runs DLDI on the ARM9 by default and moves it to the ARM7 only when the driver declares `FEATURE_ARM7_CAPABLE` or the program asks for it. It ships an open-source R4 DLDI driver and documents its memory map, TCM/cache usage and DLDI behavior in detail. The closest prior art, chatds (a 2.75M-parameter chat model on the DS), is built with BlocksDS.

DSLM loads about 1 MB once at startup, so DLDI throughput barely matters (well under a second either way). What matters is that the binary launches and mounts the card on as many R4 clones as possible.

## Decision

DSLM is built with BlocksDS, with DLDI in its default ARM9 mode. The SDK version is pinned in the repository (a Docker image or a setup script) so builds are reproducible.

## Alternatives considered

1. **devkitPro libnds v2 (calico).** Faster DLDI and preemptive threads, but incompatible with the R4iMenu kernel found on many clones, and DLDI is forced onto the ARM7.
2. **devkitPro legacy libnds 1.x + libfat.** Compatible with old kernels, but frozen and no longer the supported path.

## Consequences

**Positive**

- Widest compatibility with R4 clones and old kernels. Better documentation for the low-level work (ITCM, DTCM, cache, memory map).
- Same toolchain as the closest prior art, so its build setup and tests can be studied directly.

**Negative**

- The devkitPro ecosystem treats BlocksDS as incompatible. Libraries and examples written for calico may need porting.

**Neutral**

- DLDI can still be moved to the ARM7 later (`dldiSetMode()`) if asynchronous loading becomes useful.
