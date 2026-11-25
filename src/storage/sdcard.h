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

/* Called after each chunk with the bytes read so far. */
typedef void (*sd_progress_fn)(size_t done, size_t total, void *user);

/* Mount the card. False means the binary may need a manual DLDI patch. */
bool sd_mount(void);

/* Read the first `n` bytes and the file size, without loading the file. */
sd_status sd_read_head(const char *path, void *buf, size_t n, size_t *file_size);

/*
 * Read the whole file into a malloc'd buffer, in chunks of 32 KB.
 * Files above `max_size` are refused before any allocation. The caller frees.
 */
sd_status sd_read_file(const char *path, size_t max_size, void **data,
                       size_t *size);
sd_status sd_read_file_progress(const char *path, size_t max_size, void **data,
                                size_t *size, sd_progress_fn progress, void *user);

/* Create or replace a file. */
sd_status sd_write_file(const char *path, const void *data, size_t size);

const char *sd_status_text(sd_status st);

#endif
