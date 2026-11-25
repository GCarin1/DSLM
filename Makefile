# DSLM top-level Makefile. Runs inside docker/Dockerfile (or any BlocksDS install).
#   make nds              build dslm.nds
#   make host-test        build and run the pure-C unit tests with the host compiler
#   make tools-test       fetch the pinned model and run the PC toolchain tests
#   make golden           build tests/golden/stories260K.json (FORCE=1 to overwrite)
#   make stage CARD=dir   write the /dslm card tree into dir (default build/card)
#   make all              host-test, tools-test and nds

PY   ?= python3
CARD ?= build/card

.PHONY: all nds host-test tools-test fetch inspect golden stage clean

all: host-test tools-test nds

host-test:
	$(MAKE) -C tests/host

nds:
	$(MAKE) -f nds.mk

fetch:
	$(PY) tools/dslm.py fetch

inspect: fetch
	$(PY) tools/dslm.py inspect

tools-test: fetch
	$(PY) -m unittest discover -s tools/tests -v

golden: fetch
	$(PY) tools/dslm.py golden $(if $(FORCE),--force)

stage: fetch
	$(PY) tools/dslm.py stage $(CARD)

clean:
	$(MAKE) -C tests/host clean
	$(MAKE) -f nds.mk clean
