#include "keyboard.h"

#include <stdlib.h>

/* Four rows of 26 px keys with 3 px gaps (y 76-188):
 *   q w e r t y u i o p
 *    a s d f g h j k l
 *   Aa z x c v b n m ' DEL
 *   ?123 , [space] . GO        */
#define ROW_Y(r) (KB_TOP + (r) * 29)
#define KEY_H 26

static const kb_rect keys[KB_KEYS] = {
    /* row 0: 10 letters */
    {3, ROW_Y(0), 23, KEY_H}, {28, ROW_Y(0), 23, KEY_H}, {53, ROW_Y(0), 23, KEY_H}, {78, ROW_Y(0), 23, KEY_H},
    {103, ROW_Y(0), 23, KEY_H}, {128, ROW_Y(0), 23, KEY_H}, {153, ROW_Y(0), 23, KEY_H}, {178, ROW_Y(0), 23, KEY_H},
    {203, ROW_Y(0), 23, KEY_H}, {228, ROW_Y(0), 23, KEY_H},
    /* row 1: 9 letters, offset half a key */
    {15, ROW_Y(1), 23, KEY_H}, {40, ROW_Y(1), 23, KEY_H}, {65, ROW_Y(1), 23, KEY_H}, {90, ROW_Y(1), 23, KEY_H},
    {115, ROW_Y(1), 23, KEY_H}, {140, ROW_Y(1), 23, KEY_H}, {165, ROW_Y(1), 23, KEY_H}, {190, ROW_Y(1), 23, KEY_H},
    {215, ROW_Y(1), 23, KEY_H},
    /* row 2: shift, 7 letters, apostrophe, delete */
    {3, ROW_Y(2), 30, KEY_H}, {36, ROW_Y(2), 23, KEY_H}, {61, ROW_Y(2), 23, KEY_H}, {86, ROW_Y(2), 23, KEY_H},
    {111, ROW_Y(2), 23, KEY_H}, {136, ROW_Y(2), 23, KEY_H}, {161, ROW_Y(2), 23, KEY_H}, {186, ROW_Y(2), 23, KEY_H},
    {211, ROW_Y(2), 18, KEY_H}, {231, ROW_Y(2), 22, KEY_H},
    /* row 3: symbols page, comma, space, period, GO */
    {3, ROW_Y(3), 36, KEY_H}, {42, ROW_Y(3), 23, KEY_H}, {68, ROW_Y(3), 123, KEY_H}, {194, ROW_Y(3), 23, KEY_H},
    {220, ROW_Y(3), 33, KEY_H},
};

/* Characters per page; '\1'..'\4' stand for the KC_* keys. */
static const char pages[3][KB_KEYS + 1] = {
    "qwertyuiop" "asdfghjkl" "\1zxcvbnm'\3" "\2, .\4",
    "QWERTYUIOP" "ASDFGHJKL" "\1ZXCVBNM'\3" "\2, .\4",
    "1234567890" "!?\":;-()&" "%@#/+=*_'\3" "\2, .\4",
};

void kb_init(kb_state *s)
{
    s->mode = KB_LOWER;
    s->focus = 0;
}

kb_rect kb_key_rect(int key)
{
    return keys[key];
}

int kb_key_code(kb_mode mode, int key)
{
    char c = pages[mode][key];
    switch (c) {
    case '\1': return KC_SHIFT;
    case '\2': return KC_SYM;
    case '\3': return KC_DEL;
    case '\4': return KC_GO;
    default: return c;
    }
}

const char *kb_key_label(kb_mode mode, int key)
{
    static char one[2];
    switch (kb_key_code(mode, key)) {
    case KC_SHIFT: return "Aa";
    case KC_SYM: return mode == KB_SYMBOLS ? "abc" : "?123";
    case KC_DEL: return "DEL";
    case KC_GO: return "GO";
    case ' ': return "space";
    default:
        one[0] = pages[mode][key];
        one[1] = '\0';
        return one;
    }
}

int kb_hit(int x, int y)
{
    for (int i = 0; i < KB_KEYS; i++) {
        kb_rect r = keys[i];
        /* the gaps count for the key on their left/top, so no tap is lost */
        if (x >= r.x - 1 && x < r.x + r.w + 2 && y >= r.y - 1 && y < r.y + r.h + 3)
            return i;
    }
    return -1;
}

static int row_of(int key)
{
    return (keys[key].y - KB_TOP) / 29;
}

static int row_first(int row)
{
    for (int i = 0; i < KB_KEYS; i++)
        if (row_of(i) == row)
            return i;
    return 0;
}

static int row_len(int row)
{
    int n = 0;
    for (int i = 0; i < KB_KEYS; i++)
        n += row_of(i) == row;
    return n;
}

int kb_move(int key, int dx, int dy)
{
    int row = row_of(key);
    if (dx) {
        int first = row_first(row), n = row_len(row);
        return first + ((key - first) + (dx > 0 ? 1 : n - 1)) % n;
    }
    if (dy) {
        int target = (row + (dy > 0 ? 1 : 3)) % 4;
        int cx = keys[key].x + keys[key].w / 2;
        int best = row_first(target), best_d = 1 << 30;
        for (int i = row_first(target); i < KB_KEYS && row_of(i) == target; i++) {
            int d = abs(keys[i].x + keys[i].w / 2 - cx);
            if (d < best_d) {
                best = i;
                best_d = d;
            }
        }
        return best;
    }
    return key;
}

int kb_press(kb_state *s, int key)
{
    int code = kb_key_code(s->mode, key);
    switch (code) {
    case KC_SHIFT:
        s->mode = s->mode == KB_LOWER ? KB_UPPER : KB_LOWER;
        return KC_NONE;
    case KC_SYM:
        s->mode = s->mode == KB_SYMBOLS ? KB_LOWER : KB_SYMBOLS;
        return KC_NONE;
    default:
        if (s->mode == KB_UPPER && code >= 'A' && code <= 'Z')
            s->mode = KB_LOWER; /* one-shot shift */
        return code;
    }
}
