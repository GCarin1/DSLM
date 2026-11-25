#include "ini.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum section { SEC_NONE, SEC_MODEL, SEC_SAMPLING };

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

/* Trim `[*b, *e)` in place. */
static void trim(const char **b, const char **e)
{
    while (*b < *e && is_space(**b))
        (*b)++;
    while (*e > *b && is_space((*e)[-1]))
        (*e)--;
}

static int span_eq(const char *b, const char *e, const char *lit)
{
    size_t n = (size_t)(e - b);
    return n == strlen(lit) && memcmp(b, lit, n) == 0;
}

static void set_path(char *dst, const char *b, const char *e)
{
    size_t n = (size_t)(e - b);
    if (n == 0 || n >= DSLM_PATH_MAX)
        return;
    memcpy(dst, b, n);
    dst[n] = '\0';
}

/* Copy a value into a bounded NUL-terminated buffer for strtod/strtol. */
static int value_copy(char *out, size_t cap, const char *b, const char *e)
{
    size_t n = (size_t)(e - b);
    if (n == 0 || n >= cap)
        return 0;
    memcpy(out, b, n);
    out[n] = '\0';
    return 1;
}

static void apply(enum section sec, const char *kb, const char *ke,
                  const char *vb, const char *ve, dslm_settings *s)
{
    char buf[32];
    char *end;

    if (sec == SEC_MODEL) {
        if (span_eq(kb, ke, "checkpoint"))
            set_path(s->checkpoint, vb, ve);
        else if (span_eq(kb, ke, "tokenizer"))
            set_path(s->tokenizer, vb, ve);
        return;
    }
    if (sec != SEC_SAMPLING || !value_copy(buf, sizeof buf, vb, ve))
        return;

    if (span_eq(kb, ke, "temperature")) {
        double v = strtod(buf, &end);
        if (*end == '\0' && v >= 0.0)
            s->temperature = (float)v;
    } else if (span_eq(kb, ke, "topp")) {
        double v = strtod(buf, &end);
        if (*end == '\0' && v >= 0.0 && v <= 1.0)
            s->topp = (float)v;
    } else if (span_eq(kb, ke, "steps")) {
        long v = strtol(buf, &end, 10);
        if (*end == '\0' && v > 0 && v <= 32767)
            s->steps = (int)v;
    } else if (span_eq(kb, ke, "seed")) {
        unsigned long v = strtoul(buf, &end, 10);
        if (*end == '\0' && buf[0] != '-')
            s->seed = (uint32_t)v;
    }
}

/* "d.ddd" from a non-negative float, rounded to three decimals. */
static void milli(char *out, size_t cap, float v)
{
    unsigned m = (unsigned)(v * 1000.0f + 0.5f);
    snprintf(out, cap, "%u.%03u", m / 1000, m % 1000);
}

size_t dslm_ini_format(const dslm_settings *s, char *buf, size_t cap)
{
    char t[16], p[16];
    milli(t, sizeof t, s->temperature);
    milli(p, sizeof p, s->topp);
    int n = snprintf(buf, cap,
                     "[model]\ncheckpoint = %s\ntokenizer  = %s\n\n"
                     "[sampling]\ntemperature = %s\ntopp        = %s\n"
                     "steps       = %d\nseed        = %lu\n",
                     s->checkpoint, s->tokenizer, t, p, s->steps, (unsigned long)s->seed);
    return n > 0 && (size_t)n < cap ? (size_t)n : 0;
}

void dslm_settings_defaults(dslm_settings *s)
{
    strcpy(s->checkpoint, "/dslm/models/stories260K_q80.bin");
    strcpy(s->tokenizer, "/dslm/models/tok512.bin");
    s->temperature = 0.0f;
    s->topp = 0.9f;
    s->steps = 256;
    s->seed = 0;
}

void dslm_ini_parse(const char *text, size_t len, dslm_settings *s)
{
    enum section sec = SEC_NONE;
    const char *p = text;
    const char *end = text + len;

    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p));
        const char *lb = p;
        const char *le = eol ? eol : end;
        p = eol ? eol + 1 : end;

        trim(&lb, &le);
        if (lb == le || *lb == ';' || *lb == '#')
            continue;

        if (*lb == '[') {
            const char *close = memchr(lb, ']', (size_t)(le - lb));
            const char *nb = lb + 1;
            const char *ne = close ? close : le;
            trim(&nb, &ne);
            if (span_eq(nb, ne, "model"))
                sec = SEC_MODEL;
            else if (span_eq(nb, ne, "sampling"))
                sec = SEC_SAMPLING;
            else
                sec = SEC_NONE;
            continue;
        }

        const char *eq = memchr(lb, '=', (size_t)(le - lb));
        if (!eq)
            continue;
        const char *kb = lb, *ke = eq;
        const char *vb = eq + 1, *ve = le;
        trim(&kb, &ke);
        trim(&vb, &ve);
        apply(sec, kb, ke, vb, ve, s);
    }
}
