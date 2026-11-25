/*
 * The only boundary to screens and input. Storage and engine code never
 * include <nds.h>. Top screen: title bar, text body, hint bar. Bottom
 * (touch) screen: one panel per app screen. Every action has a button and a
 * touch target; ui_poll() maps both to the same events.
 */
#ifndef DSLM_UI_H
#define DSLM_UI_H

#include <stddef.h>

#include "storage/ini.h"

typedef enum {
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
    UI_EV_SAVE
} ui_event_type;

typedef struct {
    ui_event_type type;
    int value;
} ui_event;

#define UI_SETTINGS_ROWS 3 /* temperature, top-p, steps */
#define UI_PROMPT_MAX 90   /* fits the three prompt lines on the bottom screen */

void ui_init(void);

/* Boot and errors */
void ui_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void ui_loading(const char *what, size_t done, size_t total);
void ui_halt(void) __attribute__((noreturn));
void ui_fatal(const char *fmt, ...) __attribute__((format(printf, 1, 2), noreturn));

/* Top screen */
void ui_title(const char *model, const char *state);
void ui_hint(const char *hint);
/* Word-wrapped body; first_line < 0 follows the end. Returns the line count. */
int ui_text(const char *text, size_t len, int first_line);
#define UI_TEXT_ROWS 20

/* Bottom screen panels */
void ui_panel_prompt(const char *prompt);
void ui_panel_generating(int tokens, int steps, unsigned ms_per_token);
void ui_panel_story(int tokens, unsigned total_ms, int stopped);
void ui_panel_settings(const dslm_settings *s, int cursor, const char *note);

/* One frame of input for the given screen. */
ui_event ui_poll(ui_screen screen);

/* Wait for the next frame. */
void ui_frame(void);

#endif
