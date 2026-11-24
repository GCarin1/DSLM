---
name: r4-deploy
description: Put a DSLM build on a real DS through an R4-family flashcart and fix the usual launch and DLDI failures.
when: The task involves running dslm.nds on real hardware, preparing a microSD card, choosing a flashcart kernel, or a FAT/DLDI failure that only happens on the R4.
---

# Skill — r4-deploy

## When to use this skill

- Moving a build from melonDS to a physical DS / DS Lite with an R4.
- `fatInitDefault()` / `fopen` fails on hardware but works in the emulator.
- The kernel refuses to launch `dslm.nds`, or shows a timebomb or "_DS_MENU.DAT" error.
- Recording a hardware benchmark.

## Procedure

1. Identify the cart from its label domain and boot behavior. Look it up in the `flashcarts/flashcard-archive` README and the Sanrax guides (`docs/r4-flashcart.md` §1).
2. Use the kernel from the table in `docs/r4-flashcart.md` §2. Reference cart: original R4 / M3 Simply + Wood R4 1.62, microSD ≤2 GB FAT.
3. Stage the card with `pc-toolchain` and copy the tree as-is. The layout is in `.doctrina/contracts/model-file-format.md`. Kernel files go at the root; DSLM files go in `/dslm.nds` and `/dslm/`.
4. Boot and launch. If FAT fails: `dlditool <cart>.dldi dslm.nds` (driver from `DS-Homebrew/DLDI`, `r4tf.dldi` for the original R4), copy and retry.
5. Run the parity prompts, then the benchmark run. Copy `/dslm/bench.txt` to `benchmarks/<YYYY-MM-DD>-<cart>.txt` with cart, kernel and card noted.

## Anti-patterns

- Committing kernels, firmware or third-party `.dldi` binaries to the repo (ADR 0005).
- Using R4iMenu on DEMON/timebomb clones.
- Passing settings through argv: clone kernels do not guarantee it. Use `/dslm/dslm.ini`.
- Forcing an ARM9-only DLDI driver onto the ARM7.
- Trusting the label: "R4" clones are often other carts.

## Related material

- ADR 0004 (BlocksDS), ADR 0005 (hardware target and kernel policy)
- `docs/r4-flashcart.md`, `docs/testing.md`
- `.doctrina/specs/test-harness/spec.md`
