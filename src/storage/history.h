/*
 * History files: one finished story or chat answer per file in
 * /dslm/history, named H0001.TXT, H0002.TXT... A few `key=value` header
 * lines, a blank line, then the text exactly as the top screen showed it.
 * Pure C: no libnds, so it builds and tests on the host.
 */
#ifndef DSLM_HISTORY_H
#define DSLM_HISTORY_H

#include <stddef.h>

#define HISTORY_DIR "/dslm/history"
#define HISTORY_PROMPT_MAX 96

typedef struct {
    char date[20];   /* "2026-11-26 20:15", or "" when the clock is unknown */
    char model[48];  /* "stories260K_q80" */
    int chat;        /* 1 for a chat answer */
    int question;    /* chat: length of the question at the start of text */
    int tokens;
    int stopped;
    char prompt[HISTORY_PROMPT_MAX + 1];
    const char *text; /* parse: points into the parsed buffer */
    size_t text_len;
} history_entry;

/* Header and text; returns the length, or 0 if `cap` is too small. */
size_t history_format(const history_entry *e, char *buf, size_t cap);

/*
 * Read a history file. Unknown keys are skipped; values that do not parse
 * keep their defaults (and `question` is clamped to the text). A header
 * cut short (no blank line yet) still fills what it has, with no text.
 * Returns 0, or -1 when `buf` is not a history file.
 */
int history_parse(const char *buf, size_t len, history_entry *e);

/* "H0042.TXT" (any case) -> 42; anything else -> -1. */
int history_number(const char *name);
/* 42 -> "H0042.TXT" (out holds 16 bytes). */
void history_name(int n, char *out);

#endif
