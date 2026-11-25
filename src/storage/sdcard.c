#include "sdcard.h"

#include <fat.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_CHUNK (32 * 1024)

bool sd_mount(void)
{
    return fatInitDefault();
}

static sd_status open_sized(const char *path, FILE **out, size_t *size)
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
    *out = f;
    *size = (size_t)len;
    return SD_OK;
}

sd_status sd_read_head(const char *path, void *buf, size_t n, size_t *file_size)
{
    FILE *f;
    sd_status st = open_sized(path, &f, file_size);
    if (st != SD_OK)
        return st;
    size_t got = fread(buf, 1, n, f);
    fclose(f);
    return got == n ? SD_OK : SD_IO_ERROR;
}

sd_status sd_read_file(const char *path, size_t max_size, void **data,
                       size_t *size)
{
    return sd_read_file_progress(path, max_size, data, size, NULL, NULL);
}

sd_status sd_read_file_progress(const char *path, size_t max_size, void **data,
                                size_t *size, sd_progress_fn progress, void *user)
{
    FILE *f;
    size_t len;
    sd_status st = open_sized(path, &f, &len);
    if (st != SD_OK)
        return st;
    if (len > max_size) {
        fclose(f);
        return SD_TOO_LARGE;
    }

    /* One spare byte so text files can be NUL-terminated by the caller. */
    unsigned char *buf = malloc(len + 1);
    if (!buf) {
        fclose(f);
        return SD_NO_MEMORY;
    }

    size_t done = 0;
    while (done < len) {
        size_t want = len - done;
        if (want > READ_CHUNK)
            want = READ_CHUNK;
        size_t got = fread(buf + done, 1, want, f);
        if (got == 0) {
            free(buf);
            fclose(f);
            return SD_IO_ERROR;
        }
        done += got;
        if (progress)
            progress(done, len, user);
    }
    fclose(f);

    buf[len] = '\0';
    *data = buf;
    *size = len;
    return SD_OK;
}

sd_status sd_write_file(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return SD_IO_ERROR;
    size_t put = fwrite(data, 1, size, f);
    int closed = fclose(f);
    return put == size && closed == 0 ? SD_OK : SD_IO_ERROR;
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
