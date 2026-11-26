#include <stdio.h>
#include <string.h>

#include "keyboard.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static int key_of(kb_mode m, int code)
{
    for (int i = 0; i < KB_KEYS; i++)
        if (kb_key_code(m, i) == code)
            return i;
    return -1;
}

static void test_layout_fits_and_does_not_overlap(void)
{
    for (int i = 0; i < KB_KEYS; i++) {
        kb_rect a = kb_key_rect(i);
        CHECK(a.x >= 0 && a.x + a.w <= 256 && a.y >= KB_TOP && a.y + a.h <= 192);
        for (int j = i + 1; j < KB_KEYS; j++) {
            kb_rect b = kb_key_rect(j);
            int apart = a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y;
            CHECK(apart);
        }
    }
}

static void test_every_key_is_hit_at_its_centre(void)
{
    for (int i = 0; i < KB_KEYS; i++) {
        kb_rect r = kb_key_rect(i);
        CHECK(kb_hit(r.x + r.w / 2, r.y + r.h / 2) == i);
    }
    CHECK(kb_hit(128, 10) == -1); /* above the keyboard */
}

static void test_pages_cover_letters_digits_and_punctuation(void)
{
    for (char c = 'a'; c <= 'z'; c++) {
        CHECK(key_of(KB_LOWER, c) >= 0);
        CHECK(key_of(KB_UPPER, c - 'a' + 'A') >= 0);
    }
    for (char c = '0'; c <= '9'; c++)
        CHECK(key_of(KB_SYMBOLS, c) >= 0);
    const char *punct = " .,'!?\"-";
    for (const char *p = punct; *p; p++)
        CHECK(key_of(KB_LOWER, *p) >= 0 || key_of(KB_SYMBOLS, *p) >= 0);
    for (int m = KB_LOWER; m <= KB_SYMBOLS; m++) {
        CHECK(key_of((kb_mode)m, KC_DEL) >= 0);
        CHECK(key_of((kb_mode)m, KC_GO) >= 0);
        CHECK(key_of((kb_mode)m, KC_SYM) >= 0);
        CHECK(key_of((kb_mode)m, ' ') >= 0);
    }
}

static void test_dpad_moves_to_the_nearest_key(void)
{
    int q = key_of(KB_LOWER, 'q'), p = key_of(KB_LOWER, 'p');
    CHECK(kb_move(q, 1, 0) == key_of(KB_LOWER, 'w'));
    CHECK(kb_move(q, -1, 0) == p);  /* rows wrap */
    CHECK(kb_move(p, 1, 0) == q);
    CHECK(kb_move(q, 0, 1) == key_of(KB_LOWER, 'a'));
    CHECK(kb_move(p, 0, 1) == key_of(KB_LOWER, 'l'));
    CHECK(kb_move(key_of(KB_LOWER, 'g'), 0, 1) == key_of(KB_LOWER, 'b') ||
          kb_move(key_of(KB_LOWER, 'g'), 0, 1) == key_of(KB_LOWER, 'v'));
    CHECK(kb_move(key_of(KB_LOWER, 'b'), 0, 1) == key_of(KB_LOWER, ' '));
    CHECK(kb_move(key_of(KB_LOWER, ' '), 0, 1) == key_of(KB_LOWER, 't') ||
          kb_move(key_of(KB_LOWER, ' '), 0, 1) == key_of(KB_LOWER, 'y')); /* columns wrap */
    CHECK(kb_move(q, 0, -1) == key_of(KB_LOWER, KC_SYM));
    /* every key is reachable from every key, so D-pad typing never gets stuck */
    for (int start = 0; start < KB_KEYS; start++) {
        int seen[KB_KEYS] = {0}, stack[KB_KEYS * 4], top = 0, n = 0;
        stack[top++] = start;
        while (top) {
            int k = stack[--top];
            if (seen[k])
                continue;
            seen[k] = 1;
            n++;
            stack[top++] = kb_move(k, 1, 0);
            stack[top++] = kb_move(k, -1, 0);
            stack[top++] = kb_move(k, 0, 1);
            stack[top++] = kb_move(k, 0, -1);
        }
        CHECK(n == KB_KEYS);
    }
}

static void test_shift_is_one_shot_and_symbols_toggle(void)
{
    kb_state s;
    kb_init(&s);
    int shift = key_of(KB_LOWER, KC_SHIFT), sym = key_of(KB_LOWER, KC_SYM);
    int h = key_of(KB_LOWER, 'h');
    CHECK(kb_press(&s, h) == 'h');
    CHECK(kb_press(&s, shift) == KC_NONE && s.mode == KB_UPPER);
    CHECK(kb_press(&s, h) == 'H' && s.mode == KB_LOWER);
    CHECK(kb_press(&s, shift) == KC_NONE);
    CHECK(kb_press(&s, shift) == KC_NONE && s.mode == KB_LOWER); /* shift twice: back to lower */
    CHECK(kb_press(&s, sym) == KC_NONE && s.mode == KB_SYMBOLS);
    CHECK(kb_press(&s, key_of(KB_LOWER, 'q')) == '1' && s.mode == KB_SYMBOLS); /* symbols stay */
    CHECK(strcmp(kb_key_label(KB_SYMBOLS, sym), "abc") == 0);
    CHECK(kb_press(&s, sym) == KC_NONE && s.mode == KB_LOWER);
    CHECK(kb_press(&s, key_of(KB_LOWER, KC_DEL)) == KC_DEL);
    CHECK(kb_press(&s, key_of(KB_LOWER, KC_GO)) == KC_GO);
    for (int m = KB_LOWER; m <= KB_SYMBOLS; m++)
        for (int i = 0; i < KB_KEYS; i++)
            CHECK(strlen(kb_key_label((kb_mode)m, i)) >= 1 && strlen(kb_key_label((kb_mode)m, i)) <= 7);
}

int main(void)
{
    test_layout_fits_and_does_not_overlap();
    test_every_key_is_hit_at_its_centre();
    test_pages_cover_letters_digits_and_punctuation();
    test_dpad_moves_to_the_nearest_key();
    test_shift_is_one_shot_and_symbols_toggle();
    puts(failures ? "FAILED" : "all keyboard tests passed");
    return failures ? 1 : 0;
}
