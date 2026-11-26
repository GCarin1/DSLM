CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O1 -I../../src/storage -I../../src/ui -I../../src/engine

.PHONY: all clean
all: test_ini test_textview test_keyboard test_sfx test_fxmath
	./test_ini
	./test_textview
	./test_keyboard
	./test_sfx
	./test_fxmath

# No libnds on the include path: storage and text layout must build without it.
test_ini: test_ini.c ../../src/storage/ini.c
	$(CC) $(CFLAGS) -o $@ test_ini.c ../../src/storage/ini.c

test_textview: test_textview.c ../../src/ui/textview.c
	$(CC) $(CFLAGS) -o $@ test_textview.c ../../src/ui/textview.c

test_keyboard: test_keyboard.c ../../src/ui/keyboard.c ../../src/ui/keyboard.h
	$(CC) $(CFLAGS) -o $@ test_keyboard.c ../../src/ui/keyboard.c

test_sfx: test_sfx.c ../../src/ui/sfx.c ../../src/ui/sfx.h
	$(CC) $(CFLAGS) -o $@ test_sfx.c ../../src/ui/sfx.c

test_fxmath: test_fxmath.c ../../src/engine/fxmath.arm.c ../../src/engine/fxtables.h
	$(CC) $(CFLAGS) -o $@ test_fxmath.c ../../src/engine/fxmath.arm.c -lm

clean:
	rm -f test_ini test_ini.exe test_textview test_textview.exe test_keyboard test_keyboard.exe test_sfx test_sfx.exe test_fxmath test_fxmath.exe
