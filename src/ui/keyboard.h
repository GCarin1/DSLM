/*
 * The on-screen QWERTY keyboard's layout and logic: key rectangles on the
 * bottom screen, D-pad focus movement, shift and symbol pages. Pure C (no
 * libnds), tested on the host; ui_gfx.c draws it and feeds it input.
 */
#ifndef DSLM_KEYBOARD_H
#define DSLM_KEYBOARD_H

typedef enum { KB_LOWER, KB_UPPER, KB_SYMBOLS } kb_mode;

/* Key codes below zero; printable ASCII keys use their character. */
enum { KC_NONE = 0, KC_SHIFT = -1, KC_SYM = -2, KC_DEL = -3, KC_GO = -4 };

typedef struct {
    short x, y, w, h;
} kb_rect;

#define KB_TOP 76    /* first pixel row of the keyboard on the bottom screen */
#define KB_KEYS 34

typedef struct {
    kb_mode mode;
    int focus; /* key index the D-pad moves */
} kb_state;

void kb_init(kb_state *s);
kb_rect kb_key_rect(int key);
/* Character or KC_* the key produces in `mode`. */
int kb_key_code(kb_mode mode, int key);
/* Label to draw (at most 7 characters). */
const char *kb_key_label(kb_mode mode, int key);
/* Key under (x, y), or -1. */
int kb_hit(int x, int y);
/* Key reached from `key` by one D-pad step; rows and columns wrap. */
int kb_move(int key, int dx, int dy);
/*
 * Press a key: returns the character to type, KC_DEL or KC_GO, or KC_NONE
 * when only the page changed (redraw the keys). An upper-case letter turns
 * shift off again.
 */
int kb_press(kb_state *s, int key);

#endif
