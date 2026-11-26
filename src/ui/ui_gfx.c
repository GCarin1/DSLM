/*
 * Graphical backend for ui.h (ADR 0011): both screens are 8bpp bitmaps drawn
 * with the DSLM palette and pixel font (gfx.h). The libnds keyboard sits on
 * the bottom screen's BG1 above the bitmap, on the prompt screen only.
 *
 * Top: title bar (y 0-17), 14 text lines (y 21-174), hint bar (y 178-191).
 * Bottom: a header strip (y 0-17) and the panel of the current screen. Every
 * touch target is a rectangle drawn by the same constant it is hit-tested by.
 */
#include "ui.h"

#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "gfx.h"
#include "splash.h"
#include "textview.h"

typedef struct {
    int x, y, w, h;
} rect;

static int hit(rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

/* ---- shared pieces ------------------------------------------------------ */

#define TITLE_H 18
#define BODY_X 6
#define BODY_Y 21
#define BODY_W 244
#define HINT_Y 178

typedef enum { BTN_NORMAL, BTN_PRIMARY, BTN_DANGER } btn_style;

/* A DS button name in a small light chip, e.g. "A" or "START". Returns the x after it. */
static int chip(gfx_surface *s, int x, int y, const char *key)
{
    int w = gfx_text_width(key, strlen(key), 1) + 3;
    gfx_round(s, x, y, w, 11, COL_GREY);
    gfx_str(s, x + 2, y + 1, key, COL_BG, 1);
    return x + w;
}

static void button(rect r, const char *label, const char *key, btn_style st, int scale)
{
    static const uint8_t fill[] = {COL_PANEL2, COL_CYAN_D, COL_RED_D};
    static const uint8_t edge[] = {COL_LINE, COL_CYAN, COL_RED};
    gfx_round(&gfx_bot, r.x, r.y, r.w, r.h, edge[st]);
    gfx_round(&gfx_bot, r.x, r.y + 1, r.w, r.h - 1, fill[st]);
    int keyw = key ? gfx_text_width(key, strlen(key), 1) + 3 : 0;
    int lw = gfx_text_width(label, strlen(label), scale) - scale;
    int gap = key ? 6 : 0;
    int x = r.x + (r.w - lw - gap - keyw) / 2;
    int ty = r.y + (r.h - 7 * scale) / 2;
    x = gfx_str(&gfx_bot, x, ty, label, COL_WHITE, scale);
    if (key)
        chip(&gfx_bot, x - scale + gap, r.y + (r.h - 11) / 2, key);
}

static void header(const char *title, uint8_t colour, const char *right)
{
    gfx_fill(&gfx_bot, 0, 0, GFX_W, TITLE_H - 1, COL_PANEL);
    gfx_fill(&gfx_bot, 0, TITLE_H - 1, GFX_W, 1, COL_LINE);
    gfx_str(&gfx_bot, 8, 5, title, colour, 1);
    if (right) {
        int w = gfx_text_width(right, strlen(right), 1);
        gfx_str(&gfx_bot, GFX_W - 8 - w, 5, right, COL_DIM, 1);
    }
}

static void clear_bottom(void)
{
    gfx_fill(&gfx_bot, 0, 0, GFX_W, GFX_H, COL_BG);
}

static void bar(gfx_surface *s, int x, int y, int w, int h, int done, int total)
{
    int fill = total > 0 ? (int)((long long)done * (w - 2) / total) : 0;
    if (fill > w - 2)
        fill = w - 2;
    gfx_round(s, x, y, w, h, COL_PANEL2);
    if (fill > 0)
        gfx_fill(s, x + 1, y + 1, fill, h - 2, COL_CYAN);
}

static void show_keyboard(int on)
{
    if (on && !keyboardIsVisible())
        keyboardShow();
    else if (!on && keyboardIsVisible())
        keyboardHide();
}

/* ---- top screen state --------------------------------------------------- */

#define ROW_CHARS 128

static struct {
    int splash;                        /* the splash covers the top screen */
    int loading_panel;                 /* the bottom shows the loading panel */
    char rows[UI_TEXT_ROWS][ROW_CHARS]; /* what each body line shows now */
    unsigned char row_len[UI_TEXT_ROWS];
    int body_valid;                    /* rows[] matches the screen */
    int kb_armed;                      /* the stylus was lifted since the prompt panel appeared */
} st;

static void body_invalidate(void)
{
    st.body_valid = 0;
}

void ui_init(void)
{
    gfx_init();
    /* Same layer layout the keyboard had on the console build, moved to BG1
     * because mode 5 makes BG3 a bitmap layer. No slide animation:
     * keyboardShow/Hide would block for several frames and swallow a short
     * button press made right after a screen change. */
    Keyboard *kb = keyboardInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x512, 20, 0, false, true);
    kb->scrollSpeed = 0;
    keyboardHide();
    gfx_load_palette();
    keysSetRepeat(15, 4);
}

/* ---- boot, loading and errors ------------------------------------------- */

#define LOG_LINES 7
#define LOG_Y 104
static char log_ring[LOG_LINES][48];
static int log_count;

static void draw_log(void)
{
    gfx_fill(&gfx_bot, 8, LOG_Y, GFX_W - 16, LOG_LINES * GFX_LINE_H + 6, COL_PANEL);
    int first = log_count > LOG_LINES ? log_count - LOG_LINES : 0;
    for (int i = first; i < log_count; i++)
        gfx_str(&gfx_bot, 12, LOG_Y + 3 + (i - first) * GFX_LINE_H, log_ring[i % LOG_LINES], COL_DIM, 1);
}

void ui_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(log_ring[log_count % LOG_LINES], sizeof log_ring[0], fmt, ap);
    va_end(ap);
    log_count++;
    if (st.loading_panel)
        draw_log();
}

static void tagline(void)
{
    static const char tag[] = "RUN LLMs ON NINTENDO DS";
    int w = gfx_text_width(tag, sizeof tag - 1, 1) - 1;
    int x = (GFX_W - w) / 2, y = 168;
    gfx_fill(&gfx_top, 14, y + 3, x - 22, 1, COL_DIM);
    gfx_fill(&gfx_top, x + w + 8, y + 3, GFX_W - 14 - (x + w + 8), 1, COL_DIM);
    gfx_str(&gfx_top, x, y, tag, COL_GREY, 1);
}

void ui_splash(const char *model)
{
    show_keyboard(0);
    decompress(splashBitmap, (void *)gfx_top.px, LZ77Vram);
    tagline();
    st.splash = 1;
    body_invalidate();

    clear_bottom();
    header("Loading", COL_CYAN, model && model[0] ? model : NULL);
    st.loading_panel = 1;
    draw_log();
}

void ui_loading(const char *what, size_t done, size_t total)
{
    unsigned pct = total ? (unsigned)(done * 100 / total) : 100;
    char msg[48];
    snprintf(msg, sizeof msg, "Loading %s...  %u%%", what, pct);
    gfx_fill(&gfx_bot, 0, 40, GFX_W, 12, COL_BG);
    gfx_str(&gfx_bot, 16, 41, msg, COL_WHITE, 1);
    bar(&gfx_bot, 16, 56, GFX_W - 32, 10, (int)pct, 100);
}

void ui_halt(void)
{
    for (;;)
        swiWaitForVBlank();
}

/* ---- top screen --------------------------------------------------------- */

static uint8_t state_colour(const char *state)
{
    if (!strcmp(state, "ready") || !strcmp(state, "done"))
        return COL_GREEN;
    if (!strcmp(state, "generating") || !strcmp(state, "stopped"))
        return COL_AMBER;
    if (!strcmp(state, "error"))
        return COL_RED;
    if (!strcmp(state, "settings"))
        return COL_GREY;
    return COL_CYAN;
}

void ui_title(const char *model, const char *state)
{
    if (st.splash) { /* leaving the splash: the body is drawn from scratch */
        st.splash = 0;
        gfx_fill(&gfx_top, 0, 0, GFX_W, GFX_H, COL_BG);
        body_invalidate();
    }
    st.loading_panel = 0;
    gfx_fill(&gfx_top, 0, 0, GFX_W, TITLE_H - 1, COL_PANEL);
    gfx_fill(&gfx_top, 0, TITLE_H - 1, GFX_W, 1, COL_CYAN_D);
    gfx_blit(&gfx_top, 5, 3, ICON_BRAIN_W, ICON_BRAIN_H, icon_brain);
    int x = gfx_str(&gfx_top, 26, 5, "DSLM", COL_WHITE, 1);

    int sw = gfx_text_width(state, strlen(state), 1) + 7;
    int px = GFX_W - 5 - sw;
    gfx_round(&gfx_top, px, 3, sw, 12, state_colour(state));
    gfx_str(&gfx_top, px + 4, 5, state, COL_BG, 1);

    /* model name between the two, clipped to the room left */
    size_t n = strlen(model);
    while (n && x + 6 + gfx_text_width(model, n, 1) > px - 4)
        n--;
    gfx_text(&gfx_top, x + 6, 5, model, n, COL_DIM, 1);

    gfx_fill(&gfx_top, 0, HINT_Y, GFX_W, 1, COL_LINE);
}

/* "START:go  L/R:examples": each "KEY:label" becomes a chip and a label. */
void ui_hint(const char *hint)
{
    if (st.splash) { /* boot messages go to the loading panel */
        gfx_fill(&gfx_bot, 0, 24, GFX_W, 12, COL_BG);
        gfx_str(&gfx_bot, 16, 25, hint, COL_GREY, 1);
        return;
    }
    gfx_fill(&gfx_top, 0, HINT_Y + 1, GFX_W, GFX_H - HINT_Y - 1, COL_PANEL);
    int x = 5, y = HINT_Y + 2;
    const char *p = hint;
    while (*p && x < GFX_W - 8) {
        const char *end = strstr(p, "  ");
        size_t n = end ? (size_t)(end - p) : strlen(p);
        const char *colon = memchr(p, ':', n);
        if (colon) {
            char key[16];
            size_t k = (size_t)(colon - p) < sizeof key - 1 ? (size_t)(colon - p) : sizeof key - 1;
            memcpy(key, p, k);
            key[k] = '\0';
            const char *label = colon + 1;
            while (*label == ' ')
                label++;
            x = chip(&gfx_top, x, y, key) + 3;
            x = gfx_text(&gfx_top, x, y + 1, label, n - (size_t)(label - p), COL_DIM, 1);
        } else {
            x = gfx_text(&gfx_top, x, y + 1, p, n, COL_DIM, 1);
        }
        x += 8;
        p += n;
        while (*p == ' ')
            p++;
    }
}

int ui_text_lines(const char *text, size_t len)
{
    return tv_wrap_px(text, len, BODY_W, gfx_advance, NULL, 0);
}

/* Redraws only the body lines whose text changed, so streaming a token
 * usually repaints one line. */
static void body_line(int r, const char *s, size_t n, uint8_t colour)
{
    if (n > ROW_CHARS)
        n = ROW_CHARS;
    if (st.body_valid && st.row_len[r] == n && memcmp(st.rows[r], s, n) == 0)
        return;
    int y = BODY_Y + r * GFX_LINE_H;
    gfx_fill(&gfx_top, 0, y, GFX_W, GFX_LINE_H, COL_BG);
    gfx_text(&gfx_top, BODY_X, y, s, n, colour, 1);
    memcpy(st.rows[r], s, n);
    st.row_len[r] = (unsigned char)n;
}

int ui_text(const char *text, size_t len, int first_line)
{
    static tv_line lines[512];
    if (st.splash)
        ui_title("", "ready");
    int total = tv_wrap_px(text, len, BODY_W, gfx_advance, lines, 512);
    int shown = total < 512 ? total : 512;
    if (first_line < 0 || first_line > shown - UI_TEXT_ROWS)
        first_line = shown > UI_TEXT_ROWS ? shown - UI_TEXT_ROWS : 0;
    for (int r = 0; r < UI_TEXT_ROWS; r++) {
        int i = first_line + r;
        if (i < shown)
            body_line(r, text + lines[i].start, lines[i].len, COL_WHITE);
        else
            body_line(r, "", 0, COL_WHITE);
    }
    st.body_valid = 1;
    return total;
}

void ui_fatal(const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    show_keyboard(0);
    ui_title("", "error");
    gfx_fill(&gfx_top, 0, TITLE_H, GFX_W, HINT_Y - TITLE_H, COL_BG);
    body_invalidate();
    gfx_str(&gfx_top, BODY_X, BODY_Y + 2, "ERROR", COL_RED, 2);
    tv_line lines[UI_TEXT_ROWS - 2];
    int n = tv_wrap_px(msg, strlen(msg), BODY_W, gfx_advance, lines, UI_TEXT_ROWS - 2);
    for (int i = 0; i < n && i < UI_TEXT_ROWS - 2; i++)
        gfx_text(&gfx_top, BODY_X, BODY_Y + (i + 2) * GFX_LINE_H, msg + lines[i].start, lines[i].len, COL_WHITE, 1);
    ui_hint("Fix the card, then restart.");
    ui_halt();
}

/* ---- bottom panels ------------------------------------------------------ */

/* Prompt: header, three-line field, four buttons; the keyboard below y 72. */
static const rect PR_FIELD = {4, 20, 248, 35};
static const rect PR_PREV = {4, 57, 34, 15};
static const rect PR_NEXT = {41, 57, 34, 15};
static const rect PR_SET = {78, 57, 56, 15};
static const rect PR_GO = {137, 57, 115, 15};

void ui_panel_prompt(const char *prompt)
{
    clear_bottom();
    header("Prompt", COL_CYAN, "L/R: examples");
    gfx_round(&gfx_bot, PR_FIELD.x, PR_FIELD.y, PR_FIELD.w, PR_FIELD.h, COL_LINE);
    gfx_round(&gfx_bot, PR_FIELD.x + 1, PR_FIELD.y + 1, PR_FIELD.w - 2, PR_FIELD.h - 2, COL_PANEL);

    tv_line lines[3];
    size_t len = strlen(prompt);
    int fw = PR_FIELD.w - 10 - 4; /* room for the cursor */
    int n = tv_wrap_px(prompt, len, fw, gfx_advance, lines, 3);
    int cx = PR_FIELD.x + 5, cy = PR_FIELD.y + 2;
    if (n > 3)
        n = 3;
    for (int i = 0; i < n; i++) {
        int y = PR_FIELD.y + 2 + i * GFX_LINE_H;
        cx = gfx_text(&gfx_bot, PR_FIELD.x + 5, y, prompt + lines[i].start, lines[i].len, COL_WHITE, 1);
        cy = y;
    }
    if (n && len && prompt[len - 1] == ' ')
        cx += gfx_advance[' '];
    gfx_fill(&gfx_bot, cx, cy, 2, 8, COL_CYAN); /* cursor */

    button(PR_PREV, "<", NULL, BTN_NORMAL, 1);
    button(PR_NEXT, ">", NULL, BTN_NORMAL, 1);
    button(PR_SET, "SET", NULL, BTN_NORMAL, 1);
    button(PR_GO, "GO", "START", BTN_PRIMARY, 1);
    if (!keyboardIsVisible()) /* a touch that opened this screen must not type */
        st.kb_armed = 0;
    show_keyboard(1);
}

static const rect GEN_STOP = {16, 96, 224, 52};

void ui_panel_generating(int tokens, int steps, unsigned ms_per_token)
{
    char buf[48];
    show_keyboard(0);
    if (tokens == 0) {
        clear_bottom();
        header("Writing...", COL_AMBER, NULL);
        button(GEN_STOP, "STOP", "B", BTN_DANGER, 2);
        gfx_str_centered(&gfx_bot, 0, 166, GFX_W, "Hold B or touch STOP to stop.", COL_DIM, 1);
    }
    gfx_fill(&gfx_bot, 0, 24, GFX_W, 46, COL_BG);
    snprintf(buf, sizeof buf, "%d / %d tokens", tokens, steps);
    gfx_str(&gfx_bot, 16, 26, buf, COL_WHITE, 1);
    bar(&gfx_bot, 16, 40, GFX_W - 32, 10, tokens, steps);
    if (ms_per_token) {
        snprintf(buf, sizeof buf, "%u ms per token", ms_per_token);
        gfx_str(&gfx_bot, 16, 56, buf, COL_DIM, 1);
    }
}

static const rect STORY_NEW = {12, 54, 232, 32};
static const rect STORY_AGAIN = {12, 92, 232, 32};
static const rect STORY_SET = {12, 130, 232, 32};

void ui_panel_story(int tokens, unsigned total_ms, int stopped)
{
    char right[24], stats[48];
    show_keyboard(0);
    clear_bottom();
    snprintf(right, sizeof right, "%d tokens", tokens);
    header(stopped ? "Stopped" : "Done", stopped ? COL_AMBER : COL_GREEN, right);
    /* positions per second (the BOS token is not generated), integer math */
    unsigned tps10 = total_ms && tokens > 1 ? (unsigned)((uint64_t)(tokens - 1) * 10000 / total_ms) : 0;
    snprintf(stats, sizeof stats, "%u.%u s    %u.%u tokens/s", total_ms / 1000, (total_ms % 1000) / 100,
             tps10 / 10, tps10 % 10);
    gfx_str(&gfx_bot, 16, 24, stats, COL_WHITE, 1);
    gfx_str(&gfx_bot, 16, 37, "Up/Down scrolls the story", COL_DIM, 1);
    button(STORY_NEW, "New prompt", "A", BTN_PRIMARY, 1);
    button(STORY_AGAIN, "Same prompt again", "Y", BTN_NORMAL, 1);
    button(STORY_SET, "Settings", "SELECT", BTN_NORMAL, 1);
}

#define SET_Y 22
#define SET_ROW_H 24
#define SET_MINUS_X 100
#define SET_VALUE_X 124
#define SET_VALUE_W 96
#define SET_PLUS_X 222
#define SET_STEP_W 22
static const rect SET_SAVE = {8, 142, 118, 26};
static const rect SET_BACK = {130, 142, 118, 26};

static void milli(char *out, size_t cap, float v)
{
    unsigned m = (unsigned)(v * 1000.0f + 0.5f);
    snprintf(out, cap, "%u.%03u", m / 1000, m % 1000);
}

static rect set_row(int i)
{
    rect r = {4, SET_Y + i * SET_ROW_H, GFX_W - 8, SET_ROW_H - 2};
    return r;
}

static void step_button(int x, int y, const char *label)
{
    gfx_round(&gfx_bot, x, y, SET_STEP_W, 16, COL_LINE);
    gfx_round(&gfx_bot, x, y + 1, SET_STEP_W, 15, COL_PANEL2);
    gfx_str_centered(&gfx_bot, x, y + 5, SET_STEP_W, label, COL_CYAN, 1);
}

void ui_panel_settings(const dslm_settings *s, int cursor, const char *note)
{
    char t[12], p[12], stp[12];
    milli(t, sizeof t, s->temperature);
    milli(p, sizeof p, s->topp);
    snprintf(stp, sizeof stp, "%d", s->steps);
    /* model row: file name without the directory and ".bin" */
    char model[DSLM_PATH_MAX];
    const char *slash = strrchr(s->checkpoint, '/');
    snprintf(model, sizeof model, "%s", slash ? slash + 1 : s->checkpoint);
    char *dot = strrchr(model, '.');
    if (dot && dot != model)
        *dot = '\0';
    const char *labels[UI_SETTINGS_ROWS] = {"Temperature", "Top-p", "Steps", "Model"};
    const char *values[UI_SETTINGS_ROWS] = {t, p, stp, model};

    show_keyboard(0);
    clear_bottom();
    header("Settings", COL_CYAN, NULL);
    for (int i = 0; i < UI_SETTINGS_ROWS; i++) {
        rect r = set_row(i);
        int sel = i == cursor;
        if (sel) {
            gfx_round(&gfx_bot, r.x, r.y, r.w, r.h, COL_PANEL2);
            gfx_fill(&gfx_bot, r.x, r.y + 3, 2, r.h - 6, COL_CYAN);
        }
        int ty = r.y + (r.h - 7) / 2;
        gfx_str(&gfx_bot, r.x + 7, ty, labels[i], sel ? COL_WHITE : COL_GREY, 1);
        int model_row = i == UI_SETTINGS_MODEL_ROW;
        step_button(SET_MINUS_X, r.y + 3, model_row ? "<" : "-");
        step_button(SET_PLUS_X, r.y + 3, model_row ? ">" : "+");
        size_t n = strlen(values[i]);
        while (n && gfx_text_width(values[i], n, 1) > SET_VALUE_W - 4)
            n--;
        int vw = gfx_text_width(values[i], n, 1) - 1;
        gfx_text(&gfx_bot, SET_VALUE_X + (SET_VALUE_W - vw) / 2, ty, values[i], n, sel ? COL_CYAN_L : COL_WHITE,
                 1);
    }
    if (note)
        gfx_str(&gfx_bot, 10, 125, note, COL_GREEN, 1);
    button(SET_SAVE, "Save", "START", BTN_PRIMARY, 1);
    button(SET_BACK, "Back", "B", BTN_NORMAL, 1);
    gfx_str_centered(&gfx_bot, 0, 176, GFX_W, "Up/Down select   Left/Right change", COL_DIM, 1);
}

/* ---- input -------------------------------------------------------------- */

static ui_event ev(ui_event_type t, int v)
{
    ui_event e = {t, v};
    return e;
}

ui_event ui_poll(ui_screen screen)
{
    scanKeys();
    uint32_t down = keysDown();
    uint32_t held = keysHeld();
    uint32_t rep = keysDownRepeat();
    touchPosition tp;
    int tx = -1, ty = -1;
    if (down & KEY_TOUCH) {
        touchRead(&tp);
        tx = tp.px;
        ty = tp.py;
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
        if (ty >= 0 && ty < PR_PREV.y + PR_PREV.h) {
            if (hit(PR_PREV, tx, ty))
                return ev(UI_EV_EXAMPLE_PREV, 0);
            if (hit(PR_NEXT, tx, ty))
                return ev(UI_EV_EXAMPLE_NEXT, 0);
            if (hit(PR_SET, tx, ty))
                return ev(UI_EV_SETTINGS, 0);
            if (hit(PR_GO, tx, ty))
                return ev(UI_EV_GO, 0);
            return ev(UI_EV_NONE, 0);
        }
        if (!st.kb_armed) {
            st.kb_armed = !(held & KEY_TOUCH);
            break;
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
            if (hit(GEN_STOP, tp.px, tp.py))
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
        if (hit(STORY_NEW, tx, ty))
            return ev(UI_EV_NEW, 0);
        if (hit(STORY_AGAIN, tx, ty))
            return ev(UI_EV_AGAIN, 0);
        if (hit(STORY_SET, tx, ty))
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
        for (int i = 0; i < UI_SETTINGS_ROWS; i++) {
            rect r = set_row(i);
            if (!hit(r, tx, ty))
                continue;
            if (tx >= SET_MINUS_X - 4 && tx < SET_MINUS_X + SET_STEP_W + 2)
                return ev(UI_EV_LEFT, i);
            if (tx >= SET_PLUS_X - 2)
                return ev(UI_EV_RIGHT, i);
            return ev(UI_EV_SELECT_ROW, i);
        }
        if (hit(SET_SAVE, tx, ty))
            return ev(UI_EV_SAVE, 0);
        if (hit(SET_BACK, tx, ty))
            return ev(UI_EV_BACK, 0);
        break;
    }
    return ev(UI_EV_NONE, 0);
}

void ui_frame(void)
{
    swiWaitForVBlank();
}
