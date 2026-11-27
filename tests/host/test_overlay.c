#include <stdio.h>
#include <string.h>

#include "art_data.h"
#include "overlay.h"
#include "sfx.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static int text_px(const char *t, size_t n, int scale)
{
    int w = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)t[i];
        if (c >= FONT_FIRST && c < FONT_FIRST + FONT_COUNT)
            w += font_width[c - FONT_FIRST] + 1;
    }
    return w * scale;
}

static void test_texts_decode_to_printable_lines(void)
{
    char buf[OV_TEXT_MAX + 1];
    for (int id = 0; id < OV_TEXTS; id++) {
        size_t n = ov_text(id, buf, sizeof buf);
        CHECK(n > 0 && n <= OV_TEXT_MAX && strlen(buf) == n);
        for (size_t i = 0; i < n; i++)
            CHECK(buf[i] >= 32 && buf[i] < 127);
    }
    CHECK(ov_text(-1, buf, sizeof buf) == 0 && buf[0] == '\0');
    CHECK(ov_text(OV_TEXTS, buf, sizeof buf) == 0 && buf[0] == '\0');
    char small[4];
    CHECK(ov_text(0, small, sizeof small) == 3 && strlen(small) == 3);
}

static void test_matching_is_exact_after_normalising(void)
{
    static const uint8_t v1[] = {0xEE, 0x94, 0xA9, 0xAF, 0xAD, 0x8D, 0xD5, 0x6C, 0x60, 0x3C, 0x44, 0x59, 0x36, 0x24, 0x35};
    static const uint8_t v2[] = {0x87, 0x94, 0x80, 0xBC, 0xBE, 0xC8, 0x8C, 0x6D, 0x7A, 0x3C,
                                 0x7D, 0x64, 0x02, 0x00, 0x0D, 0x2F, 0x33, 0xBB, 0xB0, 0xBE};
    char p[32];
    ov_decode(0, v1, sizeof v1, p);
    CHECK(ov_match(p));
    ov_decode(0, v2, sizeof v2, p);
    CHECK(ov_match(p));
    /* the example prompts of both modes, and near misses, never match */
    static const char *const no[] = {
        "Once upon a time", "The little dog", "Lily wanted to", "Tom and his mom went to the park",
        "What causes the seasons?", "Hello! Who are you?", "Give me three tips to sleep better.",
        "Write a short story about a cat.", "", "   ", "?!", "I am", "Who are you", "a m", "ama",
    };
    for (size_t i = 0; i < sizeof no / sizeof no[0]; i++)
        CHECK(!ov_match(no[i]));
}

static void test_script_plays_once_within_its_length(void)
{
    int len = ov_length();
    CHECK(len >= 20 * 60 && len <= 40 * 60);

    ov_player p;
    ov_cmd c;
    char buf[OV_TEXT_MAX + 1];
    int frames = 0, ends = 0, lines = 0, hush_last = 0, sfx = 0;
    ov_start(&p);
    do {
        while (ov_poll(&p, &c)) {
            hush_last = c.op == OV_HUSH ? 1 : c.op == OV_END ? hush_last : 0;
            if (c.op == OV_END)
                ends++;
            if (c.op == OV_SFX)
                sfx += c.arg == SFX_WAKE;
            if (c.op == OV_LINE) {
                size_t n = ov_text(c.arg, buf, sizeof buf);
                int scale = c.flags & OV_LARGE ? 2 : 1;
                CHECK(c.len >= 1 && c.len <= n);
                CHECK(c.screen == OV_TOP || c.screen == OV_BOT);
                CHECK(text_px(buf, n, scale) <= 256 - 16); /* 8 px margins */
                CHECK(c.row * OV_ROW_PX + 9 * scale <= 192);
                lines += c.len == n;
            }
        }
        frames++;
    } while (ov_advance(&p) && frames < 10000);
    CHECK(frames == len);
    CHECK(ends == 1);
    CHECK(lines >= 20);
    CHECK(sfx == 1);
    CHECK(hush_last); /* the sound is off when it ends */
    CHECK(!ov_poll(&p, &c) && !ov_advance(&p));
}

static void test_typed_lines_reveal_one_character_at_a_time(void)
{
    ov_player p;
    ov_cmd c;
    int last_len = 0, last_id = -1, ok = 1;
    ov_start(&p);
    do {
        while (ov_poll(&p, &c)) {
            if (c.op != OV_LINE || !(c.flags & OV_TYPED))
                continue;
            if (c.arg == last_id)
                ok &= c.len == last_len + 1;
            else
                ok &= c.len == 1;
            last_id = c.arg;
            last_len = c.len;
        }
    } while (ov_advance(&p));
    CHECK(ok);
}

/* Photosensitivity: at most 3 large brightness changes in any second, every
 * glitch burst short, none in the quiet ending, and each glitch frame covers
 * less than a quarter of the screen. */
static void test_flashes_stay_within_safe_limits(void)
{
    enum { MAX_EVENTS = 64 };
    int events[MAX_EVENTS], n_events = 0, fill[2] = {-1, -1};
    int frame = 0, last_tone_off = -1, last_glitch = -1, bursts = 0;
    ov_player p;
    ov_cmd c;
    ov_start(&p);
    do {
        while (ov_poll(&p, &c)) {
            if (c.op == OV_CLEAR)
                for (int sc = 0; sc < 2; sc++)
                    if ((c.screen & (1 << sc)) && fill[sc] != c.colour) {
                        fill[sc] = c.colour;
                        if (n_events < MAX_EVENTS)
                            events[n_events++] = frame;
                    }
            if (c.op == OV_TONE && c.arg == 0)
                last_tone_off = frame;
            if (c.op == OV_GLITCH) {
                CHECK(c.arg >= 1 && c.arg <= OV_GLITCH_FRAMES);
                last_glitch = frame;
                bursts++;
            }
        }
        frame++;
    } while (ov_advance(&p));
    for (int i = 0; i < n_events; i++) {
        int in_window = 0;
        for (int j = i; j < n_events && events[j] < events[i] + 60; j++)
            in_window++;
        CHECK(in_window <= 3);
    }
    CHECK(bursts >= 4);
    CHECK(last_glitch >= 0 && last_glitch < last_tone_off);

    uint32_t rng = 1;
    ov_shape sh[OV_SHAPES_MAX];
    for (int f = 0; f < 5000; f++) {
        int n = ov_glitch(&rng, sh), area = 0;
        CHECK(n >= 3 && n <= OV_SHAPES_MAX);
        for (int i = 0; i < n; i++) {
            CHECK(sh[i].x >= 0 && sh[i].y >= 0 && sh[i].x + sh[i].w <= 256 && sh[i].y + sh[i].h <= 192);
            CHECK(sh[i].dx % 2 == 0 && sh[i].dx >= -24 && sh[i].dx <= 24);
            area += sh[i].kind == OV_G_SCAN ? sh[i].w * sh[i].h / 2 : sh[i].w * sh[i].h;
        }
        CHECK(area * 4 < 256 * 192);
        int j = ov_jitter(&rng);
        CHECK(j >= -3 && j <= 3);
    }
    uint32_t a = 7, b = 7;
    CHECK(ov_rand(&a) == ov_rand(&b) && a != 0); /* the same burst plays the same way */
}

int main(void)
{
    test_texts_decode_to_printable_lines();
    test_matching_is_exact_after_normalising();
    test_script_plays_once_within_its_length();
    test_typed_lines_reveal_one_character_at_a_time();
    test_flashes_stay_within_safe_limits();
    puts(failures ? "FAILED" : "all overlay tests passed");
    return failures ? 1 : 0;
}
