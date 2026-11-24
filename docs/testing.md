# Testing DSLM: emulator first, R4 second

The requirements live in `.doctrina/specs/test-harness/spec.md`. This page is the how-to.

## Rules that apply everywhere

- Parity runs use temperature 0 and compare token ids with the `pc-toolchain` golden outputs.
- Timings come from DS hardware timers, never the host clock. Emulator timings are labelled `emulated=yes` and are never published as hardware results.
- Every run writes `/dslm/bench.txt` (format in the contract).

## melonDS (primary)

1. Config → Emu settings → DLDI: enable DLDI, create a 1 GB image and tick **Sync SD to folder**, pointing at the staged card directory.
2. Copy files into that folder **only while melonDS is closed**. Optionally mark the SD as read-only.
3. melonDS DLDI-patches the binary automatically and needs no BIOS/firmware dumps in DS mode (it uses FreeBIOS).
4. Automate parity with headless melonDS runs that read `/dslm/bench.txt` and the generated token log back from the sync folder (pattern used by chatds).

## DeSmuME (second opinion)

- GUI: Config → ROM loading → "Load entirely to RAM"; Slot 1 = R4, directory = staged card folder.
- CLI:

```bash
desmume-cli --slot1 R4 --preload-rom --slot1-fat-dir ./card dslm.nds
```

- Old versions (0.9.11) do not patch homebrew with the newer 16 KB DLDI header. If FAT fails, patch manually with `r4tf.dldi` or use MPCF in slot 2.

## no$gba (debugger only)

Breakpoints and cycle counts for kernel tuning. Do not use it to test the SD flow.

## Real hardware runbook

1. Prepare the card as in `docs/r4-flashcart.md`.
2. Boot the kernel, launch `dslm.nds`, confirm the model loads (`fatInitDefault()` and `fopen` succeed).
3. If FAT fails, apply the manual DLDI patch and retry.
4. Run the parity prompt set, then the benchmark run.
5. Copy `/dslm/bench.txt` to `benchmarks/<YYYY-MM-DD>-<cart>.txt` and record the cart, kernel and microSD card in the file header.
6. Test SD writes (logs) last. DLDI writes are slower and riskier than reads.

## Where emulator and hardware diverge

The 4 KB data cache, main-RAM wait states and DLDI latency differ between emulators and real hardware. Expect tokens per second to change between melonDS and the R4. That gap is exactly what SC6 exists to record.
