/*
 * A timed full-screen overlay: a fixed script of draw and sound commands
 * played frame by frame (60 Hz) on top of whatever the screens show. Pure C,
 * tested on the host; ui_gfx.c draws the commands and restores the screens.
 */
#ifndef DSLM_OVERLAY_H
#define DSLM_OVERLAY_H

#include <stddef.h>
#include <stdint.h>

enum { OV_CLEAR, OV_LINE, OV_TONE, OV_NOISE, OV_SFX, OV_HUSH, OV_END };
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
    uint8_t arg;    /* OV_LINE text, OV_SFX sound, OV_TONE/OV_NOISE level (0 = off) */
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

/* Text `id` into out (NUL-terminated, truncated to cap); returns its length. */
size_t ov_text(int id, char *out, size_t cap);
/* Decode n bytes stored with key `id` (the inverse of the table's encoding). */
void ov_decode(int id, const uint8_t *in, size_t n, char *out);
/* The prompt starts the overlay. */
int ov_match(const char *prompt);

#endif
