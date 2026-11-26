#include <stdio.h>
#include <string.h>

#include "theme.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static int channel(uint16_t c, int i)
{
    return (c >> (5 * i)) & 31;
}

static void test_colours_are_distinct_and_bright_enough(void)
{
    for (int i = 0; i < THEME_COLOURS; i++) {
        uint16_t c = theme_colour(i);
        CHECK(c < 0x8000);
        int maxc = 0;
        for (int k = 0; k < 3; k++)
            maxc = channel(c, k) > maxc ? channel(c, k) : maxc;
        CHECK(maxc >= 14); /* readable as an accent on the navy background */
        for (int j = i + 1; j < THEME_COLOURS; j++)
            CHECK(theme_colour(j) != c);
    }
    CHECK(theme_colour(-1) == theme_colour(0));
    CHECK(theme_colour(16) == theme_colour(0));
}

static void test_shades_bracket_the_base(void)
{
    for (int i = 0; i < THEME_COLOURS; i++) {
        uint16_t base = theme_colour(i), d, l;
        theme_shades(base, &d, &l);
        for (int k = 0; k < 3; k++) {
            CHECK(channel(d, k) <= channel(base, k));
            CHECK(channel(l, k) >= channel(base, k));
        }
    }
    uint16_t d, l;
    theme_shades(0x7FFF, &d, &l);
    CHECK(l == 0x7FFF);
    theme_shades(0, &d, &l);
    CHECK(d == 0);
}

static void test_name_ascii(void)
{
    char out[16];
    const uint16_t gabriel[] = {'G', 'a', 'b', 'r', 'i', 'e', 'l'};
    CHECK(theme_name_ascii(gabriel, 7, out, sizeof out) == 7 && strcmp(out, "Gabriel") == 0);
    const uint16_t accented[] = {'J', 0x00F5, 'a', 'o'}; /* "Jõao" */
    theme_name_ascii(accented, 4, out, sizeof out);
    CHECK(strcmp(out, "J?ao") == 0);
    const uint16_t spaced[] = {' ', 'A', 'n', 'a', ' ', ' ', 0, 'x'};
    CHECK(theme_name_ascii(spaced, 8, out, sizeof out) == 3 && strcmp(out, "Ana") == 0);
    const uint16_t ten[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l'};
    CHECK(theme_name_ascii(ten, 12, out, sizeof out) == 10); /* the firmware holds 10 */
    CHECK(theme_name_ascii(ten, 12, out, 4) == 3 && strcmp(out, "abc") == 0);
    CHECK(theme_name_ascii(ten, 0, out, sizeof out) == 0 && out[0] == '\0');
}

int main(void)
{
    test_colours_are_distinct_and_bright_enough();
    test_shades_bracket_the_base();
    test_name_ascii();
    puts(failures ? "FAILED" : "all theme tests passed");
    return failures ? 1 : 0;
}
