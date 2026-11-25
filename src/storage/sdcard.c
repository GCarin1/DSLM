#include "sdcard.h"

#include <fat.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_CHUNK (32 * 1024)

bool sd_mount(void)
{
    return fatInitDefault();
}

sd_status sd_read_file(const char *path, size_t max_size, void **data,
                       size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return SD_NOT_FOUND;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return SD_IO_ERROR;
    }
    long len = ftell(f);
    if (len < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return SD_IO_ERROR;
    }
    if ((size_t)len > max_size) {
        fclose(f);
        return SD_TOO_LARGE;
    }

    /* One spare byte so text files can be NUL-terminated by the caller. */
    unsigned char *buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return SD_NO_MEMORY;
    }

    size_t done = 0;
    while (done < (size_t)len) {
        size_t want = (size_t)len - done;
        if (want > READ_CHUNK)
            want = READ_CHUNK;
        size_t got = fread(buf + done, 1, want, f);
        if (got == 0) {
            free(buf);
            fclose(f);
            return SD_IO_ERROR;
        }
        done += got;
    }
    fclose(f);

    buf[len] = '\0';
    *data = buf;
    *size = (size_t)len;
    return SD_OK;
}

const char *sd_status_text(sd_status st)
{
    switch (st) {
    case SD_OK:        return "ok";
    case SD_NOT_FOUND: return "file not found";
    case SD_NO_MEMORY: return "out of memory";
    case SD_IO_ERROR:  return "read error";
    case SD_TOO_LARGE: return "file too large";
    }
    return "unknown error";
}
