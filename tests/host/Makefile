CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O1 -I../../src/storage

# No libnds on the include path: the storage logic must build without it.
test_ini: test_ini.c ../../src/storage/ini.c
	$(CC) $(CFLAGS) -o $@ test_ini.c ../../src/storage/ini.c

.PHONY: all clean
all: test_ini
	./test_ini

clean:
	rm -f test_ini test_ini.exe
