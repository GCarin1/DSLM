# Building DSLM

The build and test environment is a Docker image based on the official BlocksDS image (ADR 0004). Nothing else needs to be installed.

## Docker (recommended)

From the repository root:

```bash
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm test    # host unit tests
docker compose -f docker/docker-compose.yml run --rm build   # produces dslm.nds
docker compose -f docker/docker-compose.yml run --rm shell   # interactive shell
```

The repository is mounted at `/work`, so `dslm.nds` appears in the repository root.

### Pinning the SDK

The image defaults to `skylyrac/blocksds:slim-latest`, which moves over time. For a reproducible build, pin an exact tag or digest and record it here:

```bash
docker compose -f docker/docker-compose.yml build --build-arg BLOCKSDS_IMAGE=<image>:<tag>
```

Last known-good digest (built and verified 2026-11-25, BlocksDS image `slim-latest`):

```
skylyrac/blocksds@sha256:c5f19b95d9c2f312e6a68ea244d2ed5120203b4b82af9785937a349096f8e5e4
```

## Without Docker

Install BlocksDS as described in its documentation, set `BLOCKSDS`, then run `make nds`. `make host-test` needs only a C99 compiler (`cc`).

## PC toolchain (models, card, golden outputs)

`tools/dslm.py` uses only the Python standard library. `fetch`, `inspect` and `stage` also run directly on the host; `golden` compiles C, so run it in Docker.

```bash
python tools/dslm.py fetch               # pinned stories260K + tok512 into .cache/models/
python tools/dslm.py inspect             # header and DS memory budget
python tools/dslm.py stage .tools/card   # /dslm tree for melonDS or the R4 microSD root
docker compose -f docker/docker-compose.yml run --rm shell make tools-test golden
python tools/dslm.py parity .tools/card/dslm/tokens.txt   # after a DS run (tests/parity/)
```

`stage` keeps an existing `dslm.ini` unless `--force` is given. `golden` refuses to overwrite `tests/golden/stories260K.json` unless `--force` (`make golden FORCE=1`). Pins (URLs, sizes, SHA-256, `llama2.c` commit) live in `tools/models.json`.

## Install package (for players)

```bash
docker compose -f docker/docker-compose.yml run --rm shell make package
```

`build/dslm-package.zip` holds `dslm.nds`, the `/dslm` tree with the pinned model, `README-DSLM.txt` (install, controls, troubleshooting), `MANIFEST.txt` (sizes and SHA-256) and `THIRD_PARTY.txt` (MIT notices for the model and `llama2.c`). Players unzip it at the root of the flashcart's microSD card; nothing is downloaded on the DS. It never contains kernels or DLDI drivers (ADR 0009). The build is byte-reproducible for the same `dslm.nds`.

## Interface art

The screens are drawn from art embedded in the .nds (ADR 0011). The sources are `assets/dslm-logo.png` (the splash is a crop of it), `assets/ui/font.txt` (the pixel font) and `assets/ui/icons.txt`. After editing one, regenerate the committed outputs (needs Pillow):

```bash
python tools/ui_assets.py           # src/ui/art_data.c/.h, assets/ui/gfx/splash.png, assets/ui/icon.png
python tools/ui_assets.py --check   # exit 1 if an output is stale
```

At build time grit compresses `assets/ui/gfx/splash.png` (LZ77, flags in `splash.grit`) and ndstool takes `assets/ui/icon.png` as the banner icon.

## Local emulator (melonDS, optional)

Used for the manual runbooks in `tests/hw/`. It is installed portably under `.tools/` (git-ignored), so nothing touches the system:

- `.tools/melonds/`: the melonDS 1.1 Windows x86_64 release; `melonDS.toml` next to the exe holds the config (portable mode). DLDI is enabled with "Sync SD to folder".
- `.tools/card/`: the virtual SD card (`/dslm/dslm.ini`, later the models). Edit it only while melonDS is closed.
- `.tools/sd.img`: the image melonDS generates from `card/`.

To uninstall everything: delete the `.tools/` folder. Docker images are removed with `docker image rm docker-build docker-test docker-shell`.

## Layout

- `src/storage/`: FAT mount, whole-file reads and the `dslm.ini` parser. No libnds in `ini.c`, so it builds on the host.
- `src/ui/`: the only code that touches screens and input, behind `ui.h`. `gfx.c` draws on the two bitmap screens, `ui_gfx.c` lays out the screens, and `textview.c` wraps text (host-tested).
- `assets/ui/`: interface art sources and the generated splash and icon (see Interface art).
- `tests/host/`: unit tests that run on the host. `tests/hw/`: manual emulator and hardware runbooks.
