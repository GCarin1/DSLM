# Manual test: offline-run

**Status:** not runnable yet — needs the reference R4 (M4). Proves `inference-engine` criterion 2.

## Steps and expected result

1. Copy the staged card (`python tools/dslm.py stage <microSD root>`) and `dslm.nds` to the R4 microSD card (`docs/r4-flashcart.md`).
2. Boot the DS with Wi-Fi off and launch `dslm.nds` from the kernel menu. Text streams on the top screen and the bottom screen ends with `done`.
3. Run `python tools/dslm.py parity <microSD>/dslm/tokens.txt` on the PC afterwards.

## Results

| Date | Cart / kernel | Result | Parity |
|------|---------------|--------|--------|
