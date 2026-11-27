/*
 * Drawing on the two 256x192 screens: one 8bpp bitmap background per screen
 * (BG2, mode 5) sharing the palette in art_data.h. VRAM takes no 8-bit
 * writes, so single pixels are read-modify-written as 16-bit pairs. Only the
 * ui layer includes this.
 */
#ifndef DSLM_GFX_H
#define DSLM_GFX_H

#include <stddef.h>
#include <stdint.h>

#include "art_data.h"

#define GFX_W 256
#define GFX_H 192
#define GFX_LINE_H 11 /* text line pitch: 9-row glyphs plus 2 px */

typedef struct {
    volatile uint16_t *px; /* 256x256 bitmap, 128 halfwords per row */
    int w, h;              /* drawable area: 256x192, or 192x256 in book mode */
} gfx_surface;

extern gfx_surface gfx_top;
extern gfx_surface gfx_bot;

/* Sets the modes, VRAM banks A and C, both bitmaps and the palette. */
void gfx_init(void);
/* Loads sub-screen palette entries 0 and 32-255. */
void gfx_load_palette(void);

/*
 * Book mode: both screens show their bitmap turned a quarter turn by the
 * background hardware, so a page drawn 192 wide and 256 tall reads upright
 * with the DS held sideways (top screen on the left). Off restores 256x192.
 */
void gfx_book(int on);

void gfx_fill(gfx_surface *s, int x, int y, int w, int h, uint8_t c);
/* Filled rectangle with 2 px rounded corners (the corner pixels are left untouched). */
void gfx_round(gfx_surface *s, int x, int y, int w, int h, uint8_t c);
/* Indexed image, each pixel drawn as a scale x scale block; index 0 is transparent. */
void gfx_blit(gfx_surface *s, int x, int y, int w, int h, const uint8_t *src);
void gfx_blit_scaled(gfx_surface *s, int x, int y, int w, int h, const uint8_t *src, int scale);

/* Advance in pixels of each byte at scale 1 (UTF-8 lead bytes draw '?', continuation bytes 0). */
extern unsigned char gfx_advance[256];
int gfx_text_width(const char *t, size_t n, int scale);
/* Draws n bytes with the glyph cell's top at y; returns the x after the text. */
int gfx_text(gfx_surface *s, int x, int y, const char *t, size_t n, uint8_t c, int scale);
int gfx_str(gfx_surface *s, int x, int y, const char *t, uint8_t c, int scale);
/* Centred in [x, x + w). */
void gfx_str_centered(gfx_surface *s, int x, int y, int w, const char *t, uint8_t c, int scale);

#endif
