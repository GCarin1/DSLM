/* Word wrapping for the console screens. Pure C, tested on the host. */
#ifndef DSLM_TEXTVIEW_H
#define DSLM_TEXTVIEW_H

#include <stddef.h>

typedef struct {
    size_t start;
    size_t len;
} tv_line;

/*
 * Split `text` into lines of at most `width` characters, breaking at spaces
 * when possible and at `\n` always. The space a line breaks on is dropped.
 * Fills up to `max` entries of `lines` (which may be NULL) and returns the
 * total number of lines, so a NULL call counts them.
 */
int tv_wrap(const char *text, size_t len, int width, tv_line *lines, int max);

/* Append `s` to a NUL-terminated buffer of `cap` bytes; truncates. */
void tv_append(char *buf, size_t *len, size_t cap, const char *s);

#endif
