# Benchmarks

One file per hardware run: `<YYYY-MM-DD>-<cart>.txt`. Each file is a copy of `/dslm/bench.txt` from the card (format in `.doctrina/contracts/model-file-format.md`) with this header added:

```
cart=<label domain, e.g. r4ds.com>
kernel=<name and version, e.g. Wood R4 1.62>
microsd=<brand, size, filesystem>
commit=<DSLM git commit>
```

Only real-hardware runs (`emulated=no`) go here. Emulator timings stay in CI logs and are never published as results (spec `test-harness`).

- `2026-11-25-r4-ysmenu.txt`: first hardware run, fp32, 627.5 ms per position, load 249 ms, parity 153/153.
- `2026-11-25-r4-ysmenu-int8.txt`: int8 fixed point, 152.7 ms per position (6.55 tokens/s, 4.1x fp32), load 124 ms, parity 257/257.
