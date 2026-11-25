/*
 * The only boundary to screens and input. Storage and engine code never
 * include <nds.h>; the M6 interface replaces the backend behind this header.
 */
#ifndef DSLM_UI_H
#define DSLM_UI_H

void ui_init(void);

/* printf-style line on the top screen. */
void ui_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Append generated text to the top screen, no newline (token streaming). */
void ui_stream(const char *text);

/* One status line on the bottom screen, replacing the previous one. */
void ui_status(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Stop without an error. Never returns. */
void ui_halt(void) __attribute__((noreturn));

/* Show an error and stop. Never returns. */
void ui_fatal(const char *fmt, ...)
    __attribute__((format(printf, 1, 2), noreturn));

#endif
