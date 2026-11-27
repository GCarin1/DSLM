#include "gfx.h"

#include <nds.h>
#include <string.h>

gfx_surface gfx_top;
gfx_surface gfx_bot;
unsigned char gfx_advance[256];

static int glyph_of(unsigned char b)
{
    if (b >= FONT_FIRST && b < FONT_FIRST + FONT_COUNT)
        return b - FONT_FIRST;
    if (b >= 0xC0)
        return '?' - FONT_FIRST; /* one '?' per non-ASCII character */
    return -1;                   /* controls and UTF-8 continuation bytes */
}

void gfx_load_palette(void)
{
    /* entries 1-31 are unused (they held the libnds keyboard's palette) */
    dmaCopy(art_palette + 32, BG_PALETTE_SUB + 32, 224 * 2);
    BG_PALETTE_SUB[0] = art_palette[COL_BG];
}

void gfx_init(void)
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    /* Sub screen bitmap at 48K (base 3, 16K units); bytes 0-48K are free. */
    int top = bgInit(2, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    int bot = bgInitSub(2, BgType_Bmp8, BgSize_B8_256x256, 3, 0);
    bgSetPriority(top, 3);
    bgSetPriority(bot, 3);
    gfx_top.px = bgGetGfxPtr(top);
    gfx_bot.px = bgGetGfxPtr(bot);
    gfx_top.w = gfx_bot.w = GFX_W;
    gfx_top.h = gfx_bot.h = GFX_H;

    dmaCopy(art_palette, BG_PALETTE, 256 * 2);
    BG_PALETTE[0] = art_palette[COL_BG];
    gfx_load_palette();

    for (int b = 0; b < 256; b++) {
        int g = glyph_of((unsigned char)b);
        gfx_advance[b] = g < 0 ? 0 : (unsigned char)(font_width[g] + 1);
    }
    gfx_fill(&gfx_top, 0, 0, GFX_W, GFX_H, COL_BG);
    gfx_fill(&gfx_bot, 0, 0, GFX_W, GFX_H, COL_BG);
}

void gfx_book(int on)
{
    /* screen (sx, sy) samples bitmap (bx, by) = (sy, 255 - sx); 8.8 fixed point */
    int16_t pa = on ? 0 : 256, pb = on ? 256 : 0, pc = on ? -256 : 0, pd = on ? 0 : 256;
    int32_t y0 = on ? 255 << 8 : 0;
    REG_BG2PA = pa;
    REG_BG2PB = pb;
    REG_BG2PC = pc;
    REG_BG2PD = pd;
    REG_BG2X = 0;
    REG_BG2Y = y0;
    REG_BG2PA_SUB = pa;
    REG_BG2PB_SUB = pb;
    REG_BG2PC_SUB = pc;
    REG_BG2PD_SUB = pd;
    REG_BG2X_SUB = 0;
    REG_BG2Y_SUB = y0;
    gfx_top.w = gfx_bot.w = on ? GFX_H : GFX_W;
    gfx_top.h = gfx_bot.h = on ? 256 : GFX_H;
}

static inline void put(gfx_surface *s, int x, int y, uint8_t c)
{
    volatile uint16_t *p = s->px + ((y << 8) + x) / 2;
    if (x & 1)
        *p = (uint16_t)((*p & 0x00FF) | (c << 8));
    else
        *p = (uint16_t)((*p & 0xFF00) | c);
}

void gfx_fill(gfx_surface *s, int x, int y, int w, int h, uint8_t c)
{
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > s->w)
        w = s->w - x;
    if (y + h > s->h)
        h = s->h - y;
    if (w <= 0 || h <= 0)
        return;
    uint16_t pair = (uint16_t)(c | (c << 8));
    for (int row = y; row < y + h; row++) {
        int x0 = x, x1 = x + w; /* [x0, x1) */
        if (x0 & 1)
            put(s, x0++, row, c);
        if (x1 & 1)
            put(s, --x1, row, c);
        volatile uint16_t *p = s->px + ((row << 8) + x0) / 2;
        for (int i = x0; i < x1; i += 2)
            *p++ = pair;
    }
}

void gfx_round(gfx_surface *s, int x, int y, int w, int h, uint8_t c)
{
    if (w < 5 || h < 5) {
        gfx_fill(s, x, y, w, h, c);
        return;
    }
    gfx_fill(s, x + 2, y, w - 4, 1, c);
    gfx_fill(s, x + 1, y + 1, w - 2, 1, c);
    gfx_fill(s, x, y + 2, w, h - 4, c);
    gfx_fill(s, x + 1, y + h - 2, w - 2, 1, c);
    gfx_fill(s, x + 2, y + h - 1, w - 4, 1, c);
}

void gfx_blit(gfx_surface *s, int x, int y, int w, int h, const uint8_t *src)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            uint8_t c = src[j * w + i];
            int px = x + i, py = y + j;
            if (c && px >= 0 && px < s->w && py >= 0 && py < s->h)
                put(s, px, py, c);
        }
}

void gfx_blit_scaled(gfx_surface *s, int x, int y, int w, int h, const uint8_t *src, int scale)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (src[j * w + i])
                gfx_fill(s, x + i * scale, y + j * scale, scale, scale, src[j * w + i]);
}

int gfx_text_width(const char *t, size_t n, int scale)
{
    int w = 0;
    for (size_t i = 0; i < n; i++)
        w += gfx_advance[(unsigned char)t[i]];
    return w * scale;
}

int gfx_text(gfx_surface *s, int x, int y, const char *t, size_t n, uint8_t c, int scale)
{
    for (size_t i = 0; i < n; i++) {
        int g = glyph_of((unsigned char)t[i]);
        if (g < 0)
            continue;
        const uint8_t *rows = font_rows + g * FONT_HEIGHT;
        int w = font_width[g];
        if (x + w * scale > s->w)
            break;
        for (int r = 0; r < FONT_HEIGHT; r++) {
            uint8_t bits = rows[r];
            int py = y + r * scale;
            if (!bits || py < 0 || py + scale > s->h)
                continue;
            for (int k = 0; k < w; k++)
                if (bits & (0x80 >> k)) {
                    if (scale == 1)
                        put(s, x + k, py, c);
                    else
                        gfx_fill(s, x + k * scale, py, scale, scale, c);
                }
        }
        x += (w + 1) * scale;
    }
    return x;
}

int gfx_str(gfx_surface *s, int x, int y, const char *t, uint8_t c, int scale)
{
    return gfx_text(s, x, y, t, strlen(t), c, scale);
}

void gfx_str_centered(gfx_surface *s, int x, int y, int w, const char *t, uint8_t c, int scale)
{
    size_t n = strlen(t);
    int tw = gfx_text_width(t, n, scale) - scale; /* no spacing after the last glyph */
    gfx_text(s, x + (w - tw) / 2, y, t, n, c, scale);
}
