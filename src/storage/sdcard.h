/* microSD access through the DLDI driver (BlocksDS libfat/FatFs). */
#ifndef DSLM_SDCARD_H
#define DSLM_SDCARD_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SD_OK = 0,
    SD_NOT_FOUND,
    SD_NO_MEMORY,
    SD_IO_ERROR,
    SD_TOO_LARGE
} sd_status;

/* Mount the card. False means the binary may need a manual DLDI patch. */
bool sd_mount(void);

/*
 * Read the whole file into a malloc'd buffer, in chunks of at least 32 KB.
 * Files above `max_size` are refused before any allocation. The caller frees.
 */
sd_status sd_read_file(const char *path, size_t max_size, void **data,
                       size_t *size);

const char *sd_status_text(sd_status st);

#endif
