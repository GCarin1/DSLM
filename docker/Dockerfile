# DSLM build and test environment (ADR 0004: BlocksDS, pinned for reproducibility).
# Build the .nds and run the host tests without installing anything locally.
#
# The base image is the official BlocksDS image. Pin it by passing an exact
# tag or digest: docker build --build-arg BLOCKSDS_IMAGE=<image>:<tag> ...
ARG BLOCKSDS_IMAGE=skylyrac/blocksds:slim-latest
FROM ${BLOCKSDS_IMAGE}

# Host compiler for the pure-C unit tests (tests/host), Python for tools/.
RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

# BLOCKSDS, BLOCKSDSEXT and WONDERFUL_TOOLCHAIN come from the base image.
CMD ["make", "all"]
