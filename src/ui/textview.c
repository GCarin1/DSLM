#include "textview.h"

#include <string.h>

int tv_wrap(const char *text, size_t len, int width, tv_line *lines, int max)
{
    size_t pos = 0;
    size_t w = width > 0 ? (size_t)width : 1;
    int n = 0;

    while (pos < len) {
        size_t start = pos, end, next;
        size_t limit = len - start < w ? len - start : w;
        const char *nl = memchr(text + start, '\n', limit);

        if (nl) {
            end = (size_t)(nl - text);
            next = end + 1;
        } else if (len - start <= w) {
            end = len;
            next = len;
        } else if (text[start + w] == ' ' || text[start + w] == '\n') {
            /* The line fills the width exactly and a break follows. */
            end = start + w;
            next = end + 1;
        } else {
            size_t bp = start + w;
            while (bp > start && text[bp - 1] != ' ')
                bp--;
            if (bp > start) {
                end = bp - 1; /* drop the space */
                next = bp;
            } else {
                end = start + w; /* one word longer than the line */
                next = end;
            }
        }

        if (lines && n < max) {
            lines[n].start = start;
            lines[n].len = end - start;
        }
        n++;
        pos = next;
    }
    return n;
}

void tv_append(char *buf, size_t *len, size_t cap, const char *s)
{
    size_t n = strlen(s);
    if (cap == 0)
        return;
    if (*len + n > cap - 1)
        n = cap - 1 - *len;
    memcpy(buf + *len, s, n);
    *len += n;
    buf[*len] = '\0';
}
