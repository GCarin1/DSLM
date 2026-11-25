# BlocksDS build for dslm.nds: ARM9 code on top of the SDK's default ARM7 core.
BLOCKSDS	?= /opt/wonderful/thirdparty/blocksds/core

NAME		:= dslm
GAME_TITLE	:= DSLM 1.0.0
GAME_SUBTITLE1	:= Llama 2 on the Nintendo DS

# Sources are listed explicitly and sorted, so the link order (and so the
# binary) does not depend on the filesystem's directory order: a Windows bind
# mount and the Linux CI runner list them differently. The template still runs
# find on SOURCEDIRS, so it points at a directory that holds no sources.
SOURCES_C	:= $(sort $(shell find src -name "*.c"))
SOURCEDIRS	:= docker
INCLUDEDIRS	:= src
LIBS		:= -lnds9
LIBDIRS		:= $(BLOCKSDS)/libs/libnds

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
