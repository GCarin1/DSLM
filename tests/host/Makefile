CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O1 -I../../src/storage

.PHONY: all clean
all: test_ini
	./test_ini

# No libnds on the include path: the storage logic must build without it.
test_ini: test_ini.c ../../src/storage/ini.c
	$(CC) $(CFLAGS) -o $@ test_ini.c ../../src/storage/ini.c

clean:
	rm -f test_ini test_ini.exe
