# Manual test: package-install

**Status:** runnable on melonDS since the install package; the real R4 half is M4. Proves `pc-toolchain`: the package alone, unzipped at the card root, is a working install.

## Steps and expected result

1. `make nds`, then `python tools/dslm.py package` writes `build/dslm-package.zip` (`--force` to rebuild).
2. Unzip it into an empty folder (melonDS "Sync SD to folder") or the root of an R4 microSD card that already has its kernel.
3. Launch the `dslm.nds` that came out of the zip. The model loads and START generates a story; `tokens.txt` appears in `dslm/`.

## Results

| Date | Target | Package | Result | emulated |
|------|--------|---------|--------|----------|
| 2026-11-25 | melonDS 1.1, fresh card folder | 1,095,384 bytes; model SHA-256 matches the pin | booted from the unzipped `dslm.nds`, loaded, generated (15/256 tokens at the capture), stopped with B | yes |
