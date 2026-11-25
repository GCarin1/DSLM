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

## Local emulator (melonDS, optional)

Used for the manual runbooks in `tests/hw/`. It is installed portably under `.tools/` (git-ignored), so nothing touches the system:

- `.tools/melonds/`: the melonDS 1.1 Windows x86_64 release; `melonDS.toml` next to the exe holds the config (portable mode). DLDI is enabled with "Sync SD to folder".
- `.tools/card/`: the virtual SD card (`/dslm/dslm.ini`, later the models). Edit it only while melonDS is closed.
- `.tools/sd.img`: the image melonDS generates from `card/`.

To uninstall everything: delete the `.tools/` folder. Docker images are removed with `docker image rm docker-build docker-test docker-shell`.

## Layout

- `src/storage/`: FAT mount, whole-file reads and the `dslm.ini` parser. No libnds in `ini.c`, so it builds on the host.
- `src/ui/`: the only code that touches screens and input, behind `ui.h`.
- `tests/host/`: unit tests that run on the host. `tests/hw/`: manual emulator and hardware runbooks.
