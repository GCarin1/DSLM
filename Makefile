# DSLM top-level Makefile. Runs inside docker/Dockerfile (or any BlocksDS install).
#   make nds        build dslm.nds
#   make host-test  build and run the pure-C unit tests with the host compiler
#   make all        both

.PHONY: all nds host-test clean

all: host-test nds

host-test:
	$(MAKE) -C tests/host

nds:
	$(MAKE) -f nds.mk

clean:
	$(MAKE) -C tests/host clean
	$(MAKE) -f nds.mk clean
