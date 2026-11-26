#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC "dslm-history=1\n"

/* Copy a header value up to the end of the line, without newlines. */
static void copy_line(char *dst, size_t cap, const char *src, size_t n)
{
    size_t k = 0;
    for (size_t i = 0; i < n && k + 1 < cap; i++)
        if (src[i] != '\n' && src[i] != '\r')
            dst[k++] = src[i];
    dst[k] = '\0';
}

size_t history_format(const history_entry *e, char *buf, size_t cap)
{
    char prompt[HISTORY_PROMPT_MAX + 1], model[sizeof e->model];
    copy_line(prompt, sizeof prompt, e->prompt, strlen(e->prompt));
    copy_line(model, sizeof model, e->model, strlen(e->model));
    int n = snprintf(buf, cap, MAGIC "date=%s\nmodel=%s\nkind=%s\nquestion=%d\ntokens=%d\nstopped=%d\nprompt=%s\n\n",
                     e->date, model, e->chat ? "chat" : "story", e->chat ? e->question : 0, e->tokens,
                     e->stopped ? 1 : 0, prompt);
    if (n <= 0 || (size_t)n + e->text_len + 1 > cap)
        return 0;
    memcpy(buf + n, e->text, e->text_len);
    buf[n + e->text_len] = '\0';
    return (size_t)n + e->text_len;
}

static int int_value(const char *v, size_t n, int fallback)
{
    char tmp[16];
    char *end;
    if (n == 0 || n >= sizeof tmp)
        return fallback;
    memcpy(tmp, v, n);
    tmp[n] = '\0';
    long x = strtol(tmp, &end, 10);
    return *end == '\0' && x >= 0 && x <= 1000000 ? (int)x : fallback;
}

int history_parse(const char *buf, size_t len, history_entry *e)
{
    memset(e, 0, sizeof *e);
    size_t m = strlen(MAGIC) - 1; /* without the newline: CRLF files are fine */
    if (len < m || memcmp(buf, MAGIC, m) != 0)
        return -1;
    const char *first_nl = memchr(buf, '\n', len);
    size_t pos = first_nl ? (size_t)(first_nl - buf) + 1 : len;
    while (pos < len) {
        const char *line = buf + pos;
        const char *nl = memchr(line, '\n', len - pos);
        size_t n = nl ? (size_t)(nl - line) : len - pos;
        pos += n + (nl ? 1 : 0);
        if (n == 0 || (n == 1 && line[0] == '\r')) { /* blank line: the text follows */
            e->text = buf + pos;
            e->text_len = len - pos;
            break;
        }
        const char *eq = memchr(line, '=', n);
        if (!eq)
            continue;
        size_t kn = (size_t)(eq - line);
        const char *v = eq + 1;
        size_t vn = n - kn - 1;
        if (vn && v[vn - 1] == '\r')
            vn--;
        if (kn == 4 && memcmp(line, "date", 4) == 0)
            copy_line(e->date, sizeof e->date, v, vn);
        else if (kn == 5 && memcmp(line, "model", 5) == 0)
            copy_line(e->model, sizeof e->model, v, vn);
        else if (kn == 6 && memcmp(line, "prompt", 6) == 0)
            copy_line(e->prompt, sizeof e->prompt, v, vn);
        else if (kn == 4 && memcmp(line, "kind", 4) == 0)
            e->chat = vn == 4 && memcmp(v, "chat", 4) == 0;
        else if (kn == 8 && memcmp(line, "question", 8) == 0)
            e->question = int_value(v, vn, 0);
        else if (kn == 6 && memcmp(line, "tokens", 6) == 0)
            e->tokens = int_value(v, vn, 0);
        else if (kn == 7 && memcmp(line, "stopped", 7) == 0)
            e->stopped = int_value(v, vn, 0) != 0;
    }
    if (!e->chat || (size_t)e->question > e->text_len)
        e->question = e->chat ? (int)e->text_len : 0;
    return 0;
}

int history_number(const char *name)
{
    if (strlen(name) != 9 || (name[0] != 'H' && name[0] != 'h'))
        return -1;
    int n = 0;
    for (int i = 1; i <= 4; i++) {
        if (name[i] < '0' || name[i] > '9')
            return -1;
        n = n * 10 + (name[i] - '0');
    }
    const char *ext = name + 5;
    for (int i = 0; i < 4; i++) {
        char c = ext[i], want = ".TXT"[i];
        if (c != want && c != want + ('a' - 'A') * (want >= 'A'))
            return -1;
    }
    return n;
}

void history_name(int n, char *out)
{
    snprintf(out, 16, "H%04d.TXT", n % 10000);
}
