/* dslm.ini parser. Pure C: no libnds, so it builds and tests on the host. */
#ifndef DSLM_INI_H
#define DSLM_INI_H

#include <stddef.h>
#include <stdint.h>

#define DSLM_PATH_MAX 128

typedef enum { DSLM_MODE_STORIES, DSLM_MODE_CHAT } dslm_mode;

typedef struct {
    char checkpoint[DSLM_PATH_MAX];      /* the story model */
    char tokenizer[DSLM_PATH_MAX];       /* the story models' tokenizer */
    char chat_checkpoint[DSLM_PATH_MAX]; /* the chat model (names its own tokenizer) */
    dslm_mode mode;                      /* the Home card used last */
    float temperature;
    float topp;
    int steps;
    uint32_t seed;
} dslm_settings;

/* Fill `s` with the defaults from contracts/model-file-format. */
void dslm_settings_defaults(dslm_settings *s);

/*
 * Apply the settings in `text` (not necessarily NUL-terminated) over `s`.
 * Unknown sections and keys are ignored. A value that does not parse or is
 * out of range leaves the previous value in place.
 */
void dslm_ini_parse(const char *text, size_t len, dslm_settings *s);

/*
 * Write `s` as a dslm.ini that dslm_ini_parse reads back (floats with three
 * decimals, no printf float support needed). Returns the length, or 0 if
 * `cap` is too small.
 */
size_t dslm_ini_format(const dslm_settings *s, char *buf, size_t cap);

#endif
