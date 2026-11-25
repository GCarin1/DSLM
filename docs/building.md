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

Pinned tag: **not yet recorded** (the image has not been built on a machine with Docker; see change 0002, open questions).

## Without Docker

Install BlocksDS as described in its documentation, set `BLOCKSDS`, then run `make nds`. `make host-test` needs only a C99 compiler (`cc`).

## Layout

- `src/storage/`: FAT mount, whole-file reads and the `dslm.ini` parser. No libnds in `ini.c`, so it builds on the host.
- `src/ui/`: the only code that touches screens and input, behind `ui.h`.
- `tests/host/`: unit tests that run on the host. `tests/hw/`: manual emulator and hardware runbooks.
