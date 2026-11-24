# Running DSLM on an R4 flashcart

How to get `dslm.nds` running on a real DS / DS Lite with an R4-family flashcart. The policy behind this page is ADR 0005. DSLM never ships kernels or firmware: get them from the guides linked below.

## 1. Identify the cart

"R4" is a label, not a hardware model. Identify the cart by the domain printed on its label (r4ds.com, r4ids.cn, r4isdhc.com 2014+, ...) and by how it boots, then look it up:

- [flashcarts/flashcard-archive](https://github.com/flashcarts/flashcard-archive): the maintained archive. Its README consolidates identical clones (e.g. the `Ace3DS+_R4iLS` family) and lists which carts accept BL2CK OS.
- [Sanrax flashcart guides](https://sanrax.github.io/flashcart-guides/) and [nds.flashcarts.net](https://nds.flashcarts.net/): per-cart setup guides.

Boot symptoms that identify a cart:

| Symptom | Meaning |
|---|---|
| "Couldn't find _DS_MENU.DAT" on a cart sold as DSTTi/DEMON | It is actually an original-R4 or Ace3DS+/R4iLS clone |
| "Wrong date or firmware expired!" | Timebomb in an old stock kernel. Update the kernel or use YSMenu |

## 2. Pick a kernel

| Cart | Kernel | Notes |
|---|---|---|
| Original R4 (r4ds.com) / M3 Simply (**reference cart**) | Wood R4 1.62 (`__rpg/` + `_DS_MENU.DAT` at the root) | microSD of **2 GB or less** (no SDHC). DLDI `r4tf.dldi` works on ARM9 and ARM7 |
| Original R4 / DSTT alternative | Pico-Launcher + Pico-Loader | No soft reset to menu |
| DSTTi "DEMON"/timebomb clones | Pico-Launcher, AKMenu-Next or YSMenu | **Avoid R4iMenu**: it does not launch calico-built homebrew |
| Ace3DS+/R4iLS family | Ace Wood R4 1.62 / AceOS | Follow the Ace3DS+ guide |
| R4i Gold 3DS Plus (r4ids.cn), Acekard/R4iTT clones | Stock/Wood kernel or BL2CK OS | DLDI verified on ARM9/ARM7 by BlocksDS for the R4i Gold 3DS Plus |
| Any, as a secondary menu | TWiLight Menu++ (flashcard edition) | The kernel still handles DLDI |

## 3. Prepare the microSD card

1. Format FAT (FAT16/FAT32 for the original R4, 2 GB or less; most SDHC clones accept up to 32 GB FAT32). Use a brand-name card; speed class does not matter.
2. Install the kernel files at the root and confirm the menu opens.
3. Copy the staged DSLM tree (`pc-toolchain` stage output): `/dslm.nds` and `/dslm/` with models, tokenizer and `dslm.ini`. The full layout is in `.doctrina/contracts/model-file-format.md`.

## 4. DLDI patching

Kernels auto-patch homebrew with their DLDI driver at launch. The original R4 kernel does so since v1.10. Auto-patching is known to fail on some carts. If DSLM reports that FAT initialisation failed:

```bash
dlditool r4tf.dldi dslm.nds   # or the .dldi for your cart, from DS-Homebrew/DLDI
```

Copy the patched binary to the card and try again. Drivers larger than 16 KB do not fit modern homebrew. Do not force an ARM9-only driver onto the ARM7.

## 5. What the flashcard archives are (and are not) good for

| Repository | What it is | Use for DSLM |
|---|---|---|
| `flashcarts/flashcard-archive` | Maintained archive (`archive/` per clone domain, `common-kernels/`), mirrored at archive.flashcarts.net | Identify the cart and get the right kernel |
| `hurda/flashcard-archive` | Stale fork of the archived `DS-Homebrew/Flashcard-Firmware-Archive`, flat folders and loose zips | History only; use when a file vanished from the main archive |
| `DS-Homebrew/DLDI` | DLDI driver sources and builds | The actual technical reference for SD access and manual patching |

Both archives are binary kernel packages with mixed, unclear licensing. There is no source code worth porting from them.

## 6. Legal note

R4 carts are banned from sale or import in some countries (rulings in Japan 2009, the UK 2010 and France 2011). DSLM is homebrew and only distributes its own code. Never use flashcarts or these archives to obtain commercial ROMs.
