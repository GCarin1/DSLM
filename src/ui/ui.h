/*
 * The only boundary to screens and input. Storage and engine code never
 * include <nds.h>. Top screen: title bar, text body, hint bar; during boot
 * and model loads, the DSLM splash. Bottom (touch) screen: one panel per app
 * screen. Every action has a button and a touch target; ui_poll() maps both
 * to the same events.
 */
#ifndef DSLM_UI_H
#define DSLM_UI_H

#include <stddef.h>

#include "storage/ini.h"

typedef enum {
    UI_HOME,
    UI_ABOUT,
    UI_HISTORY,
    UI_REPORT,
    UI_CHOOSE,          /* co-writing: the user picks the next token */
    UI_PATHS,           /* choose-your-path: the user picks a continuation */
    UI_PROMPT,
    UI_GENERATING,
    UI_STORY,
    UI_SETTINGS
} ui_screen;

typedef enum {
    UI_EV_NONE,
    UI_EV_CHAR,         /* value = character */
    UI_EV_BACKSPACE,
    UI_EV_GO,
    UI_EV_STOP,
    UI_EV_EXAMPLE_PREV,
    UI_EV_EXAMPLE_NEXT,
    UI_EV_SETTINGS,
    UI_EV_BACK,
    UI_EV_NEW,
    UI_EV_AGAIN,
    UI_EV_UP,
    UI_EV_DOWN,
    UI_EV_LEFT,         /* value = settings row, or -1 for the cursor row */
    UI_EV_RIGHT,        /* value = settings row, or -1 for the cursor row */
    UI_EV_SELECT_ROW,   /* value = settings row */
    UI_EV_SAVE,
    UI_EV_OPEN,         /* value = Home card */
    UI_EV_MENU,         /* back to Home */
    UI_EV_PAGE_UP,      /* scroll the top screen by a page */
    UI_EV_PAGE_DOWN,
    UI_EV_DELETE,
    UI_EV_PEEK,         /* show or hide the likely next tokens */
    UI_EV_COWRITE,      /* take the pen (generating) or give it back (choosing) */
    UI_EV_PICK,         /* value = choice row, or -1 for the highlighted one */
    UI_EV_PATHS         /* continue the story with choices */
} ui_event_type;

/* Home cards, in reading order of the 2x2 grid */
enum { UI_CARD_STORIES, UI_CARD_CHAT, UI_CARD_HISTORY, UI_CARD_SETTINGS, UI_CARD_BENCH, UI_CARD_ABOUT, UI_CARDS };

/* One row of a list panel. */
typedef struct {
    const char *left;  /* e.g. the date */
    const char *right; /* e.g. the prompt */
    int accent;        /* drawn in the chat colour */
} ui_list_item;

typedef struct {
    ui_event_type type;
    int value;
} ui_event;

#define UI_SETTINGS_ROWS 6      /* temperature, top-p, steps, model, sound, colour */
#define UI_SETTINGS_MODEL_ROW 3 /* Left/Right picks a checkpoint from /dslm/models */
#define UI_SETTINGS_SOUND_ROW 4 /* Left/Right switches the sounds on or off */
#define UI_SETTINGS_ACCENT_ROW 5 /* Left/Right: DSLM cyan or the DS favourite colour */
#define UI_PROMPT_MAX 90   /* fits the three prompt lines on the bottom screen */

void ui_init(void);

/* Boot and errors */
void ui_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/* Splash on the top screen, loading panel (progress and log) on the bottom. */
void ui_splash(const char *model);
void ui_loading(const char *what, size_t done, size_t total);
void ui_halt(void) __attribute__((noreturn));
void ui_fatal(const char *fmt, ...) __attribute__((format(printf, 1, 2), noreturn));

/* Top screen */
void ui_title(const char *model, const char *state);
void ui_hint(const char *hint);
/* Word-wrapped body; first_line < 0 follows the end. Returns the line count. */
int ui_text(const char *text, size_t len, int first_line);
/* Lines the body would wrap `text` into. */
int ui_text_lines(const char *text, size_t len);
#define UI_TEXT_ROWS 14

/* Home: the logo on top, four cards below; `note` (may be NULL) explains a failed load. */
void ui_home(int focus, const char *stories, const char *chat, const char *note);
/* History list: `n` items with `sel` highlighted; confirm asks for a second Y. */
void ui_history(const ui_list_item *items, int n, int sel, int confirm);

/* A results panel: `n` lines, then Run again (A) and Back (B). */
void ui_report(const char *title, const char *const *lines, int n);

/* About: the logo on top, `n` lines and a Back button below. */
void ui_about(const char *const *lines, int n);

/* Chat layout for the body: the first question_len bytes are the question
 * (a bubble on the right), the rest after a blank line the answer (a bubble
 * on the left); -1 is the plain story layout. */
void ui_set_chat(int question_len);
/*
 * A generation is running (on) or over (off): the brain icons pulse, and
 * closing the lid turns the backlights off instead of sleeping, so the model
 * keeps writing. If the lid is still closed when it ends, the DS sleeps once
 * the done sound has played (in ui_frame()) and wakes when it is opened.
 */
void ui_generating(int on);
/* Play one of the sfx.h sounds (the interface plays the ones for input
 * events itself); off silences everything. */
void ui_sound(int sfx);
void ui_sound_enable(int on);

/* Accent colour: 0 the DSLM cyan, 1 the favourite colour in the DS settings. */
void ui_accent(int from_ds);
/* The user's name from the DS settings in ASCII, or "" when it is not set. */
const char *ui_user_name(void);

/* Fade both screens out before drawing a new screen, and in after. */
void ui_fade_out(void);
void ui_fade_in(void);

/* Bottom screen panels */
void ui_panel_prompt(const char *prompt);
void ui_panel_generating(int tokens, int steps, unsigned ms_per_token);
/* tokens: in the story now; gen_tokens: produced in total_ms (the speed figure). */
void ui_panel_story(int tokens, int gen_tokens, unsigned total_ms, int stopped, int can_branch);
/* Choose-your-path: `n` continuations with `sel` highlighted, or, while they
 * are being written, only `status` (texts NULL). */
void ui_paths(const char *const *texts, int n, int sel, const char *status);
/* Generating panel layout: 0 no peek (chat), 1 peek off (X offers it), 2 peek on,
 * 3 co-writing (the likely tokens are choices; Y gives the pen back). */
void ui_peek_mode(int mode);
/* The likely next tokens (most likely first) and which one was picked (-1: none of them). */
void ui_peek(const char *const *pieces, const int *permille, int n, int chosen);
/* Co-writing: the same rows as choices, `sel` highlighted for A. */
void ui_choices(const char *const *pieces, const int *permille, int n, int sel);
void ui_panel_settings(const dslm_settings *s, int cursor, const char *note);

/* One frame of input for the given screen. */
ui_event ui_poll(ui_screen screen);

/* Wait for the next frame. */
void ui_frame(void);

#endif
