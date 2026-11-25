# BlocksDS build for dslm.nds. The template path is unverified until a build runs.
BLOCKSDS	?= /opt/blocksds/core

NAME		:= dslm
GAME_TITLE	:= DSLM
GAME_SUBTITLE1	:= Llama 2 on the Nintendo DS

SOURCEDIRS	:= src src/storage src/ui
INCLUDEDIRS	:= src
LIBS		:= -lnds9
LIBDIRS		:= $(BLOCKSDS)/libs/libnds

include $(BLOCKSDS)/sys/default_makefiles/rom_combo/Makefile
