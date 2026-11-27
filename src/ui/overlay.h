/*
 * A timed full-screen overlay: a fixed script of draw and sound commands
 * played frame by frame (60 Hz) on top of whatever the screens show. Pure C,
 * tested on the host; ui_gfx.c draws the commands and restores the screens.
 */
#ifndef DSLM_OVERLAY_H
#define DSLM_OVERLAY_H

#include <stddef.h>
#include <stdint.h>

enum { OV_CLEAR, OV_LINE, OV_TONE, OV_NOISE, OV_SFX, OV_HUSH, OV_GLITCH, OV_END };
enum { OV_TOP = 1, OV_BOT = 2, OV_BOTH = 3 };
enum { OV_WHITE, OV_RED, OV_BLACK, OV_GREY };

#define OV_CENTRE 1 /* centred on the screen */
#define OV_TYPED 2  /* revealed a character at a time */
#define OV_LARGE 4  /* drawn at scale 2 */

#define OV_TYPE_FRAMES 2 /* frames per revealed character */
#define OV_ROW_PX 12     /* text row pitch */
#define OV_TEXTS 24
#define OV_TEXT_MAX 48
#define OV_LOG_TEXT 22   /* two lines kept for the About panel after a run */

/* One command for the renderer. */
typedef struct {
    uint8_t op;     /* OV_* */
    uint8_t screen; /* OV_TOP, OV_BOT or both */
    uint8_t colour; /* OV_WHITE... */
    uint8_t flags;  /* OV_CENTRE | OV_TYPED | OV_LARGE */
    uint8_t row;    /* OV_LINE: y = row * OV_ROW_PX */
    uint8_t arg;    /* OV_LINE text, OV_SFX sound, OV_TONE/OV_NOISE level (0 = off), OV_GLITCH frames */
    uint8_t len;    /* OV_LINE: characters of the text to draw */
} ov_cmd;

typedef struct {
    int i;          /* next script step */
    int wait;       /* frames until the next command */
    int typing;     /* script step being revealed, or -1 */
    int shown, len; /* characters revealed of it */
    int frames;     /* frames played */
    int done;
} ov_player;

void ov_start(ov_player *p);
/* The next command due in this frame (1), or none left for it (0). */
int ov_poll(ov_player *p, ov_cmd *c);
/* End of a frame; 0 once the script is over. */
int ov_advance(ov_player *p);
/* Length of the whole script in frames. */
int ov_length(void);

/* A screen glitch burst lasts at most this many frames. */
#define OV_GLITCH_FRAMES 12

/* One shape of a glitch frame; the shapes of a frame cover under a quarter of the screen. */
enum { OV_G_TEAR, OV_G_BLOCK, OV_G_SCAN };
typedef struct {
    uint8_t kind;  /* OV_G_TEAR: rows shifted by dx and tinted; OV_G_BLOCK: striped box; OV_G_SCAN: every other row dark */
    uint8_t tint;  /* 0 or 1: which of the two glitch colours */
    int16_t x, y, w, h;
    int16_t dx;
} ov_shape;
#define OV_SHAPES_MAX 8

/* xorshift32; the state must not be 0. */
uint32_t ov_rand(uint32_t *state);
/* The shapes of one glitch frame (returns how many, at most OV_SHAPES_MAX). */
int ov_glitch(uint32_t *state, ov_shape *out);
/* Whole-screen shake for one glitch frame, -3..3 px. */
int ov_jitter(uint32_t *state);

/* Text `id` into out (NUL-terminated, truncated to cap); returns its length. */
size_t ov_text(int id, char *out, size_t cap);
/* Decode n bytes stored with key `id` (the inverse of the table's encoding). */
void ov_decode(int id, const uint8_t *in, size_t n, char *out);
/* The prompt starts the overlay. */
int ov_match(const char *prompt);

#endif
