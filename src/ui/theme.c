#include "theme.h"

#define RGB15(r, g, b) ((uint16_t)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))

/* Approximations of the DS settings' favourite colours, in their menu order. */
static const uint16_t colours[THEME_COLOURS] = {
    RGB15(0x7D, 0x8B, 0x99), /* grey */
    RGB15(0xA8, 0x60, 0x30), /* brown */
    RGB15(0xF0, 0x28, 0x28), /* red */
    RGB15(0xF8, 0x78, 0xC8), /* pink */
    RGB15(0xF8, 0x90, 0x18), /* orange */
    RGB15(0xE8, 0xD8, 0x20), /* yellow */
    RGB15(0xA8, 0xF0, 0x20), /* lime */
    RGB15(0x28, 0xD8, 0x28), /* green */
    RGB15(0x18, 0x88, 0x30), /* dark green */
    RGB15(0x48, 0xD8, 0x90), /* sea green */
    RGB15(0x30, 0xC8, 0xF0), /* turquoise */
    RGB15(0x28, 0x58, 0xF8), /* blue */
    RGB15(0x30, 0x40, 0xC0), /* dark blue */
    RGB15(0x88, 0x38, 0xE8), /* purple */
    RGB15(0xC0, 0x48, 0xF0), /* violet */
    RGB15(0xF0, 0x38, 0xA8), /* magenta */
};

uint16_t theme_colour(int index)
{
    return index >= 0 && index < THEME_COLOURS ? colours[index] : colours[0];
}

void theme_shades(uint16_t base, uint16_t *dark, uint16_t *light)
{
    uint16_t d = 0, l = 0;
    for (int sh = 0; sh < 15; sh += 5) {
        int c = (base >> sh) & 31;
        d |= (uint16_t)((c * 3 / 5) << sh);
        l |= (uint16_t)((c + (31 - c) * 3 / 5) << sh);
    }
    *dark = d;
    *light = l;
}

size_t theme_name_ascii(const uint16_t *name, size_t len, char *out, size_t cap)
{
    size_t n = 0;
    if (cap == 0)
        return 0;
    if (len > 10)
        len = 10;
    for (size_t i = 0; i < len && n + 1 < cap; i++) {
        uint16_t c = name[i];
        if (c == 0)
            break;
        out[n++] = c >= 32 && c < 127 ? (char)c : '?';
    }
    while (n && out[n - 1] == ' ')
        n--;
    size_t start = 0;
    while (start < n && out[start] == ' ')
        start++;
    for (size_t i = 0; i < n - start; i++)
        out[i] = out[i + start];
    n -= start;
    out[n] = '\0';
    return n;
}
