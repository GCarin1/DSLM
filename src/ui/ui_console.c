/*
 * libnds text-console backend for ui.h.
 *
 * Top screen (32x24): row 0 title, row 1 rule, rows 2-21 text, row 22 rule,
 * row 23 hint. Bottom screen: a console on BG0 for the panels and the libnds
 * keyboard on BG3 (prompt screen only). Touch targets are character cells
 * (8x8 pixels), so every box drawn below is also its hit area.
 */
#include "ui.h"

#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "textview.h"

#define COLS 32
#define TEXT_TOP 2

/* ANSI colours understood by the libnds console */
#define C_RESET "\x1b[39m"
#define C_TITLE "\x1b[33;1m"
#define C_DIM "\x1b[37m"
#define C_ACCENT "\x1b[36;1m"
#define C_OK "\x1b[32;1m"
#define C_ERR "\x1b[31;1m"

static PrintConsole top;
static PrintConsole bottom;

static void at(PrintConsole *c, int row, int col)
{
    consoleSelect(c);
    c->cursorY = row;
    c->cursorX = col;
}

/* Print `s` padded with spaces to `width` columns at (row, 0). Rows 22-23
 * use at most 31 columns so the cursor never wraps past the last row. */
static void line(PrintConsole *c, int row, const char *color, const char *s, size_t n)
{
    int width = row >= 22 ? COLS - 1 : COLS;
    at(c, row, 0);
    fputs(color, stdout);
    if ((int)n > width)
        n = (size_t)width;
    fwrite(s, 1, n, stdout);
    for (int i = (int)n; i < width; i++)
        putchar(' ');
    fputs(C_RESET, stdout);
}

static void linef(PrintConsole *c, int row, const char *color, const char *fmt, ...)
{
    char buf[COLS + 1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    line(c, row, color, buf, strlen(buf));
}

static void rule(PrintConsole *c, int row)
{
    static const char dashes[] = "--------------------------------";
    line(c, row, C_DIM, dashes, COLS);
}

/* A box of 3 rows spanning columns [c0, c1] with a centred label. */
static void box(int row, int c0, int c1, const char *label, const char *color)
{
    int w = c1 - c0 + 1;
    char edge[COLS + 1], mid[COLS + 1];
    memset(edge, '-', (size_t)w);
    edge[0] = edge[w - 1] = '+';
    edge[w] = '\0';
    memset(mid, ' ', (size_t)w);
    mid[0] = mid[w - 1] = '|';
    mid[w] = '\0';
    int len = (int)strlen(label);
    int start = (w - len) / 2;
    if (start < 1)
        start = 1;
    memcpy(mid + start, label, (size_t)(len < w - 2 ? len : w - 2));

    for (int r = 0; r < 3; r++) {
        at(&bottom, row + r, c0);
        fputs(r == 1 ? color : C_DIM, stdout);
        fputs(r == 1 ? mid : edge, stdout);
        fputs(C_RESET, stdout);
    }
}

static void clear_bottom(void)
{
    consoleSelect(&bottom);
    consoleClear();
}

static void show_keyboard(int on)
{
    if (on && !keyboardIsVisible())
        keyboardShow();
    else if (!on && keyboardIsVisible())
        keyboardHide();
}

void ui_init(void)
{
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(&top, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    /* keyboardDemoInit() takes BG3 with map base 20 and tile base 0 on the
     * sub screen, so the bottom console uses tile base 3 and map base 31. */
    consoleInit(&bottom, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 3, false, true);
    /* No slide animation: keyboardShow/Hide would block for several frames
     * and swallow a short button press made right after a screen change. */
    Keyboard *kb = keyboardDemoInit();
    kb->scrollSpeed = 0;
    keyboardHide();
    keysSetRepeat(15, 4);
    consoleSelect(&top);
}

/* ---- boot and errors ---------------------------------------------------- */

void ui_log(const char *fmt, ...)
{
    va_list ap;
    consoleSelect(&bottom);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    consoleSelect(&top);
}

void ui_loading(const char *what, size_t done, size_t total)
{
    unsigned pct = total ? (unsigned)(done * 100 / total) : 100;
    char bar[COLS + 1];
    int filled = (int)(pct * 30 / 100);
    bar[0] = '[';
    for (int i = 0; i < 30; i++)
        bar[1 + i] = i < filled ? '#' : '-';
    bar[31] = ']';
    bar[32] = '\0';
    linef(&top, 10, C_ACCENT, "Loading %s... %u%%", what, pct);
    line(&top, 12, C_OK, bar, COLS);
    consoleSelect(&top);
}

void ui_halt(void)
{
    for (;;)
        swiWaitForVBlank();
}

void ui_fatal(const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    show_keyboard(0);
    ui_title("DSLM", "error");
    for (int r = TEXT_TOP; r < TEXT_TOP + UI_TEXT_ROWS; r++)
        line(&top, r, C_RESET, "", 0);
    line(&top, TEXT_TOP + 1, C_ERR, "ERROR", 5);
    tv_line lines[UI_TEXT_ROWS - 3];
    int n = tv_wrap(msg, strlen(msg), COLS, lines, UI_TEXT_ROWS - 3);
    for (int i = 0; i < n && i < UI_TEXT_ROWS - 3; i++)
        line(&top, TEXT_TOP + 3 + i, C_RESET, msg + lines[i].start, lines[i].len);
    ui_hint("Fix the card, then restart.");
    ui_halt();
}

/* ---- top screen --------------------------------------------------------- */

void ui_title(const char *model, const char *state)
{
    char buf[COLS + 1];
    int right = (int)strlen(state) + 2;
    int room = COLS - 8 - right; /* " DSLM  " + model + " " + "[state]" */
    snprintf(buf, sizeof buf, " DSLM  %-*.*s [%s]", room, room, model, state);
    line(&top, 0, C_TITLE, buf, strlen(buf));
    rule(&top, 1);
    rule(&top, 22);
}

void ui_hint(const char *hint)
{
    line(&top, 23, C_ACCENT, hint, strlen(hint));
}

int ui_text(const char *text, size_t len, int first_line)
{
    static tv_line lines[512];
    int total = tv_wrap(text, len, COLS, lines, 512);
    int shown = total < 512 ? total : 512;
    if (first_line < 0 || first_line > shown - UI_TEXT_ROWS)
        first_line = shown > UI_TEXT_ROWS ? shown - UI_TEXT_ROWS : 0;
    for (int r = 0; r < UI_TEXT_ROWS; r++) {
        int i = first_line + r;
        if (i < shown)
            line(&top, TEXT_TOP + r, C_RESET, text + lines[i].start, lines[i].len);
        else
            line(&top, TEXT_TOP + r, C_RESET, "", 0);
    }
    return total;
}

/* ---- bottom panels ------------------------------------------------------ */

/* Prompt panel: rows 0-4 text, rows 6-8 buttons, keyboard below. */
#define PROMPT_BUTTON_ROW 6

void ui_panel_prompt(const char *prompt)
{
    char shown[UI_PROMPT_MAX + 2];
    snprintf(shown, sizeof shown, "%s_", prompt);
    clear_bottom();
    linef(&bottom, 0, C_ACCENT, " Prompt          L/R: examples");
    rule(&bottom, 1);
    tv_line lines[3];
    int n = tv_wrap(shown, strlen(shown), COLS, lines, 3);
    if (n > 3)
        n = 3;
    for (int i = 0; i < n; i++)
        line(&bottom, 2 + i, C_RESET, shown + lines[i].start, lines[i].len);
    box(PROMPT_BUTTON_ROW, 0, 5, "<", C_ACCENT);
    box(PROMPT_BUTTON_ROW, 7, 12, ">", C_ACCENT);
    box(PROMPT_BUTTON_ROW, 14, 20, "SET", C_ACCENT);
    box(PROMPT_BUTTON_ROW, 22, 31, "GO", C_OK);
    show_keyboard(1);
    consoleSelect(&top);
}

#define STOP_ROW 12

void ui_panel_generating(int tokens, int steps, unsigned ms_per_token)
{
    char bar[COLS + 1];
    int filled = steps > 0 ? tokens * 20 / steps : 0;
    if (filled > 20)
        filled = 20;
    show_keyboard(0);
    if (tokens == 0) {
        clear_bottom();
        box(STOP_ROW, 0, 31, "STOP  (B)", C_ERR);
        linef(&bottom, 20, C_DIM, " Hold B or touch STOP to stop.");
    }
    linef(&bottom, 0, C_ACCENT, " Generating...");
    bar[0] = '[';
    for (int i = 0; i < 20; i++)
        bar[1 + i] = i < filled ? '#' : '-';
    snprintf(bar + 21, sizeof bar - 21, "] %d/%d", tokens, steps);
    line(&bottom, 2, C_OK, bar, strlen(bar));
    if (ms_per_token)
        linef(&bottom, 4, C_RESET, " %u ms per token", ms_per_token);
    consoleSelect(&top);
}

#define STORY_NEW_ROW 5
#define STORY_AGAIN_ROW 9
#define STORY_SET_ROW 13

void ui_panel_story(int tokens, unsigned total_ms, int stopped)
{
    show_keyboard(0);
    clear_bottom();
    linef(&bottom, 0, stopped ? C_TITLE : C_OK, stopped ? " Stopped: %d tokens" : " Done: %d tokens", tokens);
    /* positions per second (the BOS token is not generated), integer math */
    unsigned tps10 = total_ms && tokens > 1 ? (unsigned)((uint64_t)(tokens - 1) * 10000 / total_ms) : 0;
    linef(&bottom, 1, C_DIM, " %u.%u s  %u.%u tokens/s", total_ms / 1000, (total_ms % 1000) / 100,
          tps10 / 10, tps10 % 10);
    linef(&bottom, 3, C_DIM, " Up/Down scrolls the story");
    box(STORY_NEW_ROW, 0, 31, "New prompt  (A)", C_OK);
    box(STORY_AGAIN_ROW, 0, 31, "Same prompt again  (Y)", C_ACCENT);
    box(STORY_SET_ROW, 0, 31, "Settings  (SELECT)", C_ACCENT);
    consoleSelect(&top);
}

#define SET_FIRST_ROW 3
#define SET_NOTE_ROW 11
#define SET_BUTTON_ROW 13

static void milli(char *out, size_t cap, float v)
{
    unsigned m = (unsigned)(v * 1000.0f + 0.5f);
    snprintf(out, cap, "%u.%03u", m / 1000, m % 1000);
}

void ui_panel_settings(const dslm_settings *s, int cursor, const char *note)
{
    char t[12], p[12], st[12];
    milli(t, sizeof t, s->temperature);
    milli(p, sizeof p, s->topp);
    snprintf(st, sizeof st, "%d", s->steps);
    /* model row: file name without the directory and ".bin" */
    char model[DSLM_PATH_MAX];
    const char *slash = strrchr(s->checkpoint, '/');
    snprintf(model, sizeof model, "%s", slash ? slash + 1 : s->checkpoint);
    char *dot = strrchr(model, '.');
    if (dot && dot != model)
        *dot = '\0';
    const char *labels[UI_SETTINGS_ROWS] = {"Temperature", "Top-p", "Steps", "Model"};
    const char *values[UI_SETTINGS_ROWS] = {t, p, st, model};

    show_keyboard(0);
    clear_bottom();
    linef(&bottom, 0, C_ACCENT, " Settings");
    rule(&bottom, 1);
    for (int i = 0; i < UI_SETTINGS_ROWS; i++) {
        if (i == UI_SETTINGS_MODEL_ROW)
            linef(&bottom, SET_FIRST_ROW + 2 * i, i == cursor ? C_TITLE : C_RESET, "%c%-12s [<] %-10.10s [>]",
                  i == cursor ? '>' : ' ', labels[i], values[i]);
        else
            linef(&bottom, SET_FIRST_ROW + 2 * i, i == cursor ? C_TITLE : C_RESET, "%c%-12s [-] %6s [+]",
                  i == cursor ? '>' : ' ', labels[i], values[i]);
    }
    if (note)
        linef(&bottom, SET_NOTE_ROW, C_OK, " %s", note);
    box(SET_BUTTON_ROW, 0, 15, "Save (START)", C_OK);
    box(SET_BUTTON_ROW, 17, 31, "Back (B)", C_ACCENT);
    linef(&bottom, 17, C_DIM, " Up/Down: select");
    linef(&bottom, 18, C_DIM, " Left/Right, -/+ or </>: change");
    consoleSelect(&top);
}

/* ---- input -------------------------------------------------------------- */

static ui_event ev(ui_event_type t, int v)
{
    ui_event e = {t, v};
    return e;
}

static int in(int v, int lo, int hi)
{
    return v >= lo && v <= hi;
}

ui_event ui_poll(ui_screen screen)
{
    scanKeys();
    uint32_t down = keysDown();
    uint32_t held = keysHeld();
    uint32_t rep = keysDownRepeat();
    touchPosition tp;
    int row = -1, col = -1;
    if (down & KEY_TOUCH) {
        touchRead(&tp);
        row = tp.py / 8;
        col = tp.px / 8;
    }

    switch (screen) {
    case UI_PROMPT:
        if (down & (KEY_START | KEY_A))
            return ev(UI_EV_GO, 0);
        if (rep & KEY_B)
            return ev(UI_EV_BACKSPACE, 0);
        if (down & KEY_L)
            return ev(UI_EV_EXAMPLE_PREV, 0);
        if (down & KEY_R)
            return ev(UI_EV_EXAMPLE_NEXT, 0);
        if (down & KEY_SELECT)
            return ev(UI_EV_SETTINGS, 0);
        if (rep & KEY_UP)
            return ev(UI_EV_UP, 0);
        if (rep & KEY_DOWN)
            return ev(UI_EV_DOWN, 0);
        if (row >= 0 && in(row, PROMPT_BUTTON_ROW, PROMPT_BUTTON_ROW + 2)) {
            if (in(col, 0, 5))
                return ev(UI_EV_EXAMPLE_PREV, 0);
            if (in(col, 7, 12))
                return ev(UI_EV_EXAMPLE_NEXT, 0);
            if (in(col, 14, 20))
                return ev(UI_EV_SETTINGS, 0);
            if (in(col, 22, 31))
                return ev(UI_EV_GO, 0);
            return ev(UI_EV_NONE, 0);
        }
        {
            int k = keyboardUpdate();
            if (k == DVK_ENTER)
                return ev(UI_EV_GO, 0);
            if (k == DVK_BACKSPACE)
                return ev(UI_EV_BACKSPACE, 0);
            if (k >= 32 && k < 127)
                return ev(UI_EV_CHAR, k);
        }
        break;

    case UI_GENERATING:
        if (held & KEY_B)
            return ev(UI_EV_STOP, 0);
        if (held & KEY_TOUCH) {
            touchRead(&tp);
            if (in(tp.py / 8, STOP_ROW - 1, STOP_ROW + 3))
                return ev(UI_EV_STOP, 0);
        }
        break;

    case UI_STORY:
        if (down & KEY_A)
            return ev(UI_EV_NEW, 0);
        if (down & KEY_Y)
            return ev(UI_EV_AGAIN, 0);
        if (down & KEY_SELECT)
            return ev(UI_EV_SETTINGS, 0);
        if (rep & KEY_UP)
            return ev(UI_EV_UP, 0);
        if (rep & KEY_DOWN)
            return ev(UI_EV_DOWN, 0);
        if (in(row, STORY_NEW_ROW, STORY_NEW_ROW + 2))
            return ev(UI_EV_NEW, 0);
        if (in(row, STORY_AGAIN_ROW, STORY_AGAIN_ROW + 2))
            return ev(UI_EV_AGAIN, 0);
        if (in(row, STORY_SET_ROW, STORY_SET_ROW + 2))
            return ev(UI_EV_SETTINGS, 0);
        break;

    case UI_SETTINGS:
        if (rep & KEY_UP)
            return ev(UI_EV_UP, 0);
        if (rep & KEY_DOWN)
            return ev(UI_EV_DOWN, 0);
        if (rep & KEY_LEFT)
            return ev(UI_EV_LEFT, -1);
        if (rep & KEY_RIGHT)
            return ev(UI_EV_RIGHT, -1);
        if (down & KEY_START)
            return ev(UI_EV_SAVE, 0);
        if (down & KEY_B)
            return ev(UI_EV_BACK, 0);
        if (in(row, SET_FIRST_ROW - 1, SET_FIRST_ROW + 2 * UI_SETTINGS_ROWS - 2)) {
            int item = (row - (SET_FIRST_ROW - 1)) / 2;
            if (in(col, 12, 17))
                return ev(UI_EV_LEFT, item);
            if (in(col, 24, 31))
                return ev(UI_EV_RIGHT, item);
            return ev(UI_EV_SELECT_ROW, item);
        }
        if (in(row, SET_BUTTON_ROW, SET_BUTTON_ROW + 2))
            return ev(col <= 15 ? UI_EV_SAVE : UI_EV_BACK, 0);
        break;
    }
    return ev(UI_EV_NONE, 0);
}

void ui_frame(void)
{
    swiWaitForVBlank();
}
