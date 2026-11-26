#include "textview.h"

#include <string.h>

int tv_wrap(const char *text, size_t len, int width, tv_line *lines, int max)
{
    return tv_wrap_px(text, len, width, NULL, lines, max);
}

int tv_wrap_px(const char *text, size_t len, int width, const unsigned char *adv, tv_line *lines, int max)
{
    size_t pos = 0;
    int limit = width > 0 ? width : 1;
    int n = 0;

    while (pos < len) {
        size_t start = pos, end = len, next = len, space = 0;
        int w = 0, have_space = 0;

        for (size_t i = start; i < len; i++) {
            unsigned char c = (unsigned char)text[i];
            if (c == '\n') {
                end = i;
                next = i + 1;
                break;
            }
            int cw = adv ? adv[c] : 1;
            if (w + cw > limit) {
                if (c == ' ') { /* the line is full and a break follows */
                    end = i;
                    next = i + 1;
                } else if (have_space) {
                    end = space; /* drop the space */
                    next = space + 1;
                } else {
                    /* one word longer than the line; always take one byte */
                    end = i > start ? i : i + 1;
                    next = end;
                }
                break;
            }
            if (c == ' ') {
                space = i;
                have_space = 1;
            }
            w += cw;
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
