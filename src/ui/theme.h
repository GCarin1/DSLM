/*
 * Personal touches from the DS firmware settings: the 16 favourite colours
 * as interface accents, the shades derived from an accent, and the user's
 * UTF-16 name as ASCII. Pure C, tested on the host; ui_gfx.c reads the
 * firmware data and applies them.
 */
#ifndef DSLM_THEME_H
#define DSLM_THEME_H

#include <stddef.h>
#include <stdint.h>

#define THEME_COLOURS 16

/* Favourite colour 0-15 from the DS settings -> RGB15 (out-of-range: 0). */
uint16_t theme_colour(int index);

/* The dark and light shades of an RGB15 accent (dark ~60%, light 60% towards white). */
void theme_shades(uint16_t base, uint16_t *dark, uint16_t *light);

/*
 * The firmware name (UTF-16, `len` units, at most 10) as printable ASCII in
 * `out` (cap bytes); other characters become '?', and surrounding spaces are
 * trimmed. Returns the length.
 */
size_t theme_name_ascii(const uint16_t *name, size_t len, char *out, size_t cap);

#endif
