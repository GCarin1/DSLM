CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O1 -I../../src/storage -I../../src/ui

.PHONY: all clean
all: test_ini test_textview
	./test_ini
	./test_textview

# No libnds on the include path: storage and text layout must build without it.
test_ini: test_ini.c ../../src/storage/ini.c
	$(CC) $(CFLAGS) -o $@ test_ini.c ../../src/storage/ini.c

test_textview: test_textview.c ../../src/ui/textview.c
	$(CC) $(CFLAGS) -o $@ test_textview.c ../../src/ui/textview.c

clean:
	rm -f test_ini test_ini.exe test_textview test_textview.exe
