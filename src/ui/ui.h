/*
 * The only boundary to screens and input. Storage and engine code never
 * include <nds.h>; the M6 interface replaces the backend behind this header.
 */
#ifndef DSLM_UI_H
#define DSLM_UI_H

void ui_init(void);

/* printf-style line on the top screen. */
void ui_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Show an error and stop. Never returns. */
void ui_fatal(const char *fmt, ...)
    __attribute__((format(printf, 1, 2), noreturn));

#endif
