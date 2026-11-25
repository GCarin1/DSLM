# BlocksDS build for dslm.nds: ARM9 code on top of the SDK's default ARM7 core.
BLOCKSDS	?= /opt/wonderful/thirdparty/blocksds/core

NAME		:= dslm
GAME_TITLE	:= DSLM 1.0.0
GAME_SUBTITLE1	:= Llama 2 on the Nintendo DS

SOURCEDIRS	:= src
INCLUDEDIRS	:= src
LIBS		:= -lnds9
LIBDIRS		:= $(BLOCKSDS)/libs/libnds

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
