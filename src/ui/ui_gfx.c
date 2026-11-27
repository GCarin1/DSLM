/*
 * Graphical backend for ui.h (ADR 0011): both screens are 8bpp bitmaps drawn
 * with the DSLM palette and pixel font (gfx.h). The prompt screen carries the
 * QWERTY keyboard (keyboard.h), typed by touch or by D-pad focus and A.
 * Input is sampled in the VBlank interrupt and latched, so a press shorter
 * than a token is still seen by the next ui_poll().
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
#include "keyboard.h"
#include "sfx.h"
#include "theme.h"
#include "splash.h"
#include "textview.h"
#include "version.h"

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

/* ---- top screen state --------------------------------------------------- */

#define ROW_CHARS 128

static struct {
    int splash;                        /* the splash covers the top screen */
    int loading_panel;                 /* the bottom shows the loading panel */
    char rows[UI_TEXT_ROWS][ROW_CHARS]; /* what each body line shows now */
    unsigned char row_len[UI_TEXT_ROWS];
    unsigned char row_style[UI_TEXT_ROWS];
    int body_valid;                    /* rows[] matches the screen */
    int chat_q;                        /* question length in chat layout, or -1 */
    kb_state kb;                       /* page and D-pad focus of the keyboard */
    int home_focus;
    char home_sub[2][40];              /* subtitles of the Stories and Chat cards */
    int list_first, list_n;            /* History list scroll */
    volatile int pulse;                /* animate COL_PULSE in the VBlank interrupt */
    uint16_t acc, acc_d, acc_l;        /* the accent and its shades now in the palette */
    volatile int busy;                 /* a generation is running: the lid does not sleep */
    volatile int dark;                 /* backlights off because the lid closed while busy */
    int sleep_pending;                 /* sleep after the done sound, if the lid is still closed */
    unsigned pulse_frame;
} st;

/* ---- sound: one PSG voice stepped in the VBlank interrupt ---------------- */

static sfx_player voice;
static volatile int sound_on = 1;
static volatile int sound_next = SFX_NONE; /* started by the next VBlank */
static int psg = -1;                       /* libnds sound id of the playing note */

static void sound_tick(void)
{
    int next = sound_next;
    if (next != SFX_NONE) {
        sound_next = SFX_NONE;
        sfx_start(&voice, (sfx_id)next);
    }
    sfx_step s = sfx_tick(&voice);
    switch (s.action) {
    case SFX_NOTE:
        if (psg >= 0)
            soundKill(psg);
        /* a PSG square wave takes 8 timer steps per cycle: the rate is 8x the pitch */
        psg = soundPlayPSG((DutyCycle)s.duty, (u16)(s.freq * 8), s.vol, 64);
        break;
    case SFX_VOLUME:
        if (psg >= 0)
            soundSetVolume(psg, s.vol);
        break;
    case SFX_SILENCE:
    case SFX_END:
        if (psg >= 0)
            soundKill(psg);
        psg = -1;
        break;
    default:
        break;
    }
}

void ui_sound(int sfx)
{
    if (sound_on)
        sound_next = sfx;
}

void ui_sound_enable(int on)
{
    sound_on = on;
}

/* ---- input latched in the VBlank interrupt ------------------------------ */

static volatile uint32_t latch_down, latch_rep;
static volatile int latch_tx = -1, latch_ty = -1; /* where the last touch began */

static void on_vblank(void)
{
    scanKeys();
    uint32_t down = keysDown();
    latch_down |= down;
    latch_rep |= keysDownRepeat();
    if (down & KEY_TOUCH) {
        touchPosition tp;
        touchRead(&tp);
        latch_tx = tp.px;
        latch_ty = tp.py;
    }
    sound_tick();
    if (st.busy) { /* lid closed while writing: screens off, keep going */
        int lid = (keysHeld() & KEY_LID) != 0;
        if (lid && !st.dark) {
            powerOff(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
            st.dark = 1;
        } else if (!lid && st.dark) {
            powerOn(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
            st.dark = 0;
        }
    }
    if (st.pulse) { /* brain icons breathe between dark and light cyan, 64 frames a cycle */
        unsigned ph = st.pulse_frame++ & 63, t = ph < 32 ? ph : 63 - ph;
        uint16_t a = st.acc_d, b = st.acc_l, c = 0;
        for (int sh = 0; sh < 15; sh += 5) {
            int ca = (a >> sh) & 31, cb = (b >> sh) & 31;
            c |= (uint16_t)((ca + (cb - ca) * (int)t / 31) << sh);
        }
        BG_PALETTE[COL_PULSE] = c;
        BG_PALETTE_SUB[COL_PULSE] = c;
    }
}

static void ui_pulse(int on)
{
    st.pulse = on;
    if (!on) {
        BG_PALETTE[COL_PULSE] = st.acc;
        BG_PALETTE_SUB[COL_PULSE] = st.acc;
    }
}

void ui_generating(int on)
{
    ui_pulse(on);
    if (on) {
        disableSleep();
        st.busy = 1;
    } else {
        st.busy = 0;
        enableSleep();
        st.sleep_pending = st.dark;
    }
}

void ui_accent(int from_ds)
{
    if (from_ds) {
        st.acc = theme_colour(PersonalData->theme & 15);
        theme_shades(st.acc, &st.acc_d, &st.acc_l);
    } else {
        st.acc = art_palette[COL_CYAN];
        st.acc_d = art_palette[COL_CYAN_D];
        st.acc_l = art_palette[COL_CYAN_L];
    }
    static const uint8_t idx[] = {COL_CYAN, COL_CYAN_D, COL_CYAN_L, COL_PULSE};
    uint16_t val[] = {st.acc, st.acc_d, st.acc_l, st.acc};
    for (int i = 0; i < 4; i++) {
        BG_PALETTE[idx[i]] = val[i];
        BG_PALETTE_SUB[idx[i]] = val[i];
    }
}

const char *ui_user_name(void)
{
    static char name[12];
    theme_name_ascii((const uint16_t *)PersonalData->name, PersonalData->nameLen, name, sizeof name);
    return name;
}

/* Master brightness steps of 4 per frame: about 70 ms each way. */
void ui_fade_out(void)
{
    for (int v = 4; v <= 16; v += 4) {
        swiWaitForVBlank();
        setBrightness(3, -v);
    }
}

void ui_fade_in(void)
{
    for (int v = 12; v >= 0; v -= 4) {
        swiWaitForVBlank();
        setBrightness(3, -v);
    }
}

void ui_set_chat(int question_len)
{
    if (st.chat_q != question_len)
        st.body_valid = 0;
    st.chat_q = question_len;
}

static void body_invalidate(void)
{
    st.body_valid = 0;
}

void ui_init(void)
{
    gfx_init();
    kb_init(&st.kb);
    st.chat_q = -1;
    soundEnable();
    ui_accent(0);
    keysSetRepeat(15, 4);
    irqSet(IRQ_VBLANK, on_vblank);
    irqEnable(IRQ_VBLANK);
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

static void logo_top(void)
{
    decompress(splashBitmap, (void *)gfx_top.px, LZ77Vram);
    tagline();
    st.splash = 1;
    body_invalidate();
}

void ui_splash(const char *model)
{
    logo_top();

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
    if ((getBatteryLevel() & BATTERY_LEVEL_MASK) <= BATTERY_LEVEL_DS_LOW) { /* red battery, 13x8 */
        px -= 18;
        gfx_fill(&gfx_top, px, 5, 12, 8, COL_RED);
        gfx_fill(&gfx_top, px + 1, 6, 10, 6, COL_PANEL);
        gfx_fill(&gfx_top, px + 2, 7, 2, 4, COL_RED);
        gfx_fill(&gfx_top, px + 12, 7, 1, 4, COL_RED);
    }

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

/* Chat layout: bubbles 206 px wide, text inset 5 px. */
#define CHAT_TEXT_W 196
#define BUBBLE_W 206
#define Q_X (GFX_W - 4 - BUBBLE_W)
#define A_X 4

enum { ROW_PLAIN, ROW_Q, ROW_A, ROW_KIND = 3, ROW_TOP = 4, ROW_BOTTOM = 8 };

static int wrap_width(void)
{
    return st.chat_q >= 0 ? CHAT_TEXT_W : BODY_W;
}

int ui_text_lines(const char *text, size_t len)
{
    return tv_wrap_px(text, len, wrap_width(), gfx_advance, NULL, 0);
}

/* Redraws only the body lines whose text or bubble shape changed, so
 * streaming a token usually repaints one line. */
static void body_line(int r, const char *s, size_t n, int style)
{
    if (n > ROW_CHARS)
        n = ROW_CHARS;
    if (st.body_valid && st.row_len[r] == n && st.row_style[r] == style && memcmp(st.rows[r], s, n) == 0)
        return;
    int y = BODY_Y + r * GFX_LINE_H;
    gfx_fill(&gfx_top, 0, y, GFX_W, GFX_LINE_H, COL_BG);
    int kind = style & ROW_KIND;
    if (kind == ROW_PLAIN) {
        gfx_text(&gfx_top, BODY_X, y, s, n, COL_WHITE, 1);
    } else {
        int x0 = kind == ROW_Q ? Q_X : A_X;
        uint8_t fill = kind == ROW_Q ? COL_CYAN_D : COL_PANEL2;
        gfx_fill(&gfx_top, x0, y, BUBBLE_W, GFX_LINE_H, fill);
        for (int side = 0; side < 2; side++) { /* 2 px rounded corners */
            int xa = side ? x0 + BUBBLE_W - 2 : x0, xb = side ? x0 + BUBBLE_W - 1 : x0;
            if (style & ROW_TOP) {
                gfx_fill(&gfx_top, xa, y, 2, 1, COL_BG);
                gfx_fill(&gfx_top, xb, y + 1, 1, 1, COL_BG);
            }
            if (style & ROW_BOTTOM) {
                gfx_fill(&gfx_top, xa, y + GFX_LINE_H - 1, 2, 1, COL_BG);
                gfx_fill(&gfx_top, xb, y + GFX_LINE_H - 2, 1, 1, COL_BG);
            }
        }
        gfx_text(&gfx_top, x0 + 5, y + 1, s, n, COL_WHITE, 1);
    }
    memcpy(st.rows[r], s, n);
    st.row_len[r] = (unsigned char)n;
    st.row_style[r] = (unsigned char)style;
}

static int line_kind(const tv_line *l)
{
    if (st.chat_q < 0)
        return ROW_PLAIN;
    if (l->start < (size_t)st.chat_q)
        return ROW_Q;
    return l->len ? ROW_A : ROW_PLAIN;
}

int ui_text(const char *text, size_t len, int first_line)
{
    static tv_line lines[512];
    if (st.splash)
        ui_title("", "ready");
    int total = tv_wrap_px(text, len, wrap_width(), gfx_advance, lines, 512);
    int shown = total < 512 ? total : 512;
    if (first_line < 0 || first_line > shown - UI_TEXT_ROWS)
        first_line = shown > UI_TEXT_ROWS ? shown - UI_TEXT_ROWS : 0;
    for (int r = 0; r < UI_TEXT_ROWS; r++) {
        int i = first_line + r;
        if (i >= shown) {
            body_line(r, "", 0, ROW_PLAIN);
            continue;
        }
        int kind = line_kind(&lines[i]), style = kind;
        if (kind != ROW_PLAIN) {
            if (i == 0 || line_kind(&lines[i - 1]) != kind)
                style |= ROW_TOP;
            if (i == shown - 1 || line_kind(&lines[i + 1]) != kind)
                style |= ROW_BOTTOM;
        }
        body_line(r, text + lines[i].start, lines[i].len, style);
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

    ui_title("", "error");
    gfx_fill(&gfx_top, 0, TITLE_H, GFX_W, HINT_Y - TITLE_H, COL_BG);
    body_invalidate();
    gfx_str(&gfx_top, BODY_X, BODY_Y + 2, "ERROR", COL_RED, 2);
    tv_line lines[UI_TEXT_ROWS - 2];
    int n = tv_wrap_px(msg, strlen(msg), BODY_W, gfx_advance, lines, UI_TEXT_ROWS - 2);
    for (int i = 0; i < n && i < UI_TEXT_ROWS - 2; i++)
        gfx_text(&gfx_top, BODY_X, BODY_Y + (i + 2) * GFX_LINE_H, msg + lines[i].start, lines[i].len, COL_WHITE, 1);
    ui_hint("Fix the card, then restart.");
    ui_sound(SFX_ERROR);
    ui_halt();
}

/* ---- bottom panels ------------------------------------------------------ */

/* Prompt: header, three-line field, four buttons; the keyboard below y 76. */
static const rect PR_FIELD = {4, 20, 248, 35};
static const rect PR_PREV = {4, 57, 34, 15};
static const rect PR_NEXT = {41, 57, 34, 15};
static const rect PR_SET = {78, 57, 56, 15};
static const rect PR_GO = {137, 57, 115, 15};

static void draw_key(int i)
{
    kb_rect r = kb_key_rect(i);
    int code = kb_key_code(st.kb.mode, i);
    int special = code < 0;
    int focus = i == st.kb.focus;
    uint8_t fill = code == KC_GO ? COL_CYAN_D : special ? COL_PANEL : COL_PANEL2;
    uint8_t edge = code == KC_GO ? COL_CYAN : COL_LINE;
    uint8_t ink = special && code != KC_GO ? COL_GREY : COL_WHITE;
    if (code == KC_SHIFT && st.kb.mode == KB_UPPER) {
        fill = COL_CYAN_D;
        ink = COL_WHITE;
    }
    gfx_fill(&gfx_bot, r.x - 1, r.y - 1, r.w + 2, r.h + 2, COL_BG); /* old focus ring */
    if (focus) {
        gfx_round(&gfx_bot, r.x - 1, r.y - 1, r.w + 2, r.h + 2, COL_CYAN);
        gfx_round(&gfx_bot, r.x + 1, r.y + 1, r.w - 2, r.h - 2, COL_CYAN_L);
        ink = COL_BG;
    } else {
        gfx_round(&gfx_bot, r.x, r.y, r.w, r.h, edge);
        gfx_round(&gfx_bot, r.x, r.y + 1, r.w, r.h - 1, fill);
    }
    gfx_str_centered(&gfx_bot, r.x, r.y + (r.h - 7) / 2, r.w, kb_key_label(st.kb.mode, i), ink, 1);
}

static void focus_key(int key)
{
    ui_sound(SFX_MOVE);
    int old = st.kb.focus;
    st.kb.focus = key;
    if (old != key)
        draw_key(old);
    draw_key(key);
}

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
    button(PR_SET, "MENU", NULL, BTN_NORMAL, 1);
    button(PR_GO, "GO", "START", BTN_PRIMARY, 1);
    for (int i = 0; i < KB_KEYS; i++)
        draw_key(i);
}

static const rect GEN_STOP_BIG = {16, 96, 224, 52};
static const rect GEN_STOP_SMALL = {16, 140, 224, 32}; /* peek on: the list takes the middle */
static int peek_mode;
static int gen_redraw;

static rect gen_stop(void)
{
    return peek_mode >= 2 ? GEN_STOP_SMALL : GEN_STOP_BIG;
}

void ui_peek_mode(int mode)
{
    if (mode != peek_mode)
        gen_redraw = 1;
    peek_mode = mode;
}

#define PEEK_Y 66
#define PEEK_ROW 14

static rect choice_row(int i)
{
    rect r = {8, PEEK_Y + i * PEEK_ROW - 1, GFX_W - 16, PEEK_ROW};
    return r;
}

static void odds_rows(const char *const *pieces, const int *permille, int n, int chosen, int cursor)
{
    gfx_fill(&gfx_bot, 0, PEEK_Y - 2, GFX_W, 5 * PEEK_ROW + 2, COL_BG);
    for (int i = 0; i < n && i < 5; i++) {
        int y = PEEK_Y + i * PEEK_ROW;
        int pick = i == chosen || i == cursor;
        if (i == cursor) {
            rect r = choice_row(i);
            gfx_round(&gfx_bot, r.x, r.y, r.w, r.h, COL_PANEL2);
            gfx_fill(&gfx_bot, r.x, r.y + 2, 2, r.h - 4, COL_CYAN);
        }
        size_t len = strlen(pieces[i]);
        while (len && gfx_text_width(pieces[i], len, 1) > 84)
            len--;
        gfx_text(&gfx_bot, 16, y + 1, pieces[i], len, pick ? COL_WHITE : COL_GREY, 1);
        int w = permille[i] * 100 / 1000;
        gfx_round(&gfx_bot, 106, y + 1, 102, 10, COL_PANEL);
        if (w > 0)
            gfx_fill(&gfx_bot, 107, y + 2, w, 8, pick ? COL_CYAN : COL_CYAN_D);
        char pct[16];
        snprintf(pct, sizeof pct, "%d%%", (permille[i] + 5) / 10);
        gfx_str(&gfx_bot, 214, y + 1, pct, pick ? COL_WHITE : COL_DIM, 1);
    }
}

void ui_peek(const char *const *pieces, const int *permille, int n, int chosen)
{
    if (peek_mode == 2)
        odds_rows(pieces, permille, n, chosen, -1);
}

static int choice_n;

void ui_choices(const char *const *pieces, const int *permille, int n, int sel)
{
    choice_n = n;
    odds_rows(pieces, permille, n, -1, sel);
}

void ui_panel_generating(int tokens, int steps, unsigned ms_per_token)
{
    char buf[48];
    if (tokens == 0 || gen_redraw) {
        gen_redraw = 0;
        clear_bottom();
        if (peek_mode == 3)
            header("Your turn: pick the next token", COL_CYAN, NULL);
        else
            header("Writing...", COL_AMBER, peek_mode == 2 ? "next token odds" : NULL);
        if (peek_mode < 2)
            gfx_blit_scaled(&gfx_bot, (GFX_W - 2 * ICON_BRAIN_W) / 2, 68, ICON_BRAIN_W, ICON_BRAIN_H, icon_brain, 2);
        button(gen_stop(), "STOP", "B", BTN_DANGER, 2);
        const char *foot = peek_mode == 1 ? "X: peek   Y: write together   B: stop"
                         : peek_mode == 2 ? "X: hide   Y: write together   B: stop"
                         : peek_mode == 3 ? "A: use it   Y: let it write   B: stop"
                                          : "Press B or touch STOP to stop.";
        gfx_str_centered(&gfx_bot, 0, peek_mode >= 2 ? 178 : 166, GFX_W, foot, COL_DIM, 1);
    }
    gfx_fill(&gfx_bot, 0, 24, GFX_W, 42, COL_BG);
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
    button(STORY_SET, "Menu", "SELECT", BTN_NORMAL, 1);
}

#define SET_Y 22
#define SET_ROW_H 20
#define SET_MINUS_X 100
#define SET_VALUE_X 124
#define SET_VALUE_W 96
#define SET_PLUS_X 222
#define SET_STEP_W 22
static const rect SET_SAVE = {8, 156, 118, 22};
static const rect SET_BACK = {130, 156, 118, 22};

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
    const char *labels[UI_SETTINGS_ROWS] = {"Temperature", "Top-p", "Steps", "Model", "Sound", "Colour"};
    const char *values[UI_SETTINGS_ROWS] = {t, p, stp, model, s->sound ? "on" : "off",
                                            s->accent ? "My DS" : "DSLM cyan"};

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
        int pick_row = i >= UI_SETTINGS_MODEL_ROW;
        step_button(SET_MINUS_X, r.y + 1, pick_row ? "<" : "-");
        step_button(SET_PLUS_X, r.y + 1, pick_row ? ">" : "+");
        size_t n = strlen(values[i]);
        while (n && gfx_text_width(values[i], n, 1) > SET_VALUE_W - 4)
            n--;
        int vw = gfx_text_width(values[i], n, 1) - 1;
        gfx_text(&gfx_bot, SET_VALUE_X + (SET_VALUE_W - vw) / 2, ty, values[i], n, sel ? COL_CYAN_L : COL_WHITE,
                 1);
    }
    if (note)
        gfx_str(&gfx_bot, 10, 144, note, COL_GREEN, 1);
    button(SET_SAVE, "Save", "START", BTN_PRIMARY, 1);
    button(SET_BACK, "Back", "B", BTN_NORMAL, 1);
    gfx_str_centered(&gfx_bot, 0, 182, GFX_W, "Up/Down select   Left/Right change", COL_DIM, 1);
}

/* ---- Home and About ----------------------------------------------------- */

/* Two columns, three rows of 120x48 cards. */
static rect card_rect(int i)
{
    rect r = {6 + (i % 2) * 124, 22 + (i / 2) * 52, 120, 48};
    return r;
}

/* The card one D-pad step away; rows and columns wrap over the cards that exist. */
static int card_move(int i, int dx, int dy)
{
    int rows = (UI_CARDS + 1) / 2, col = i % 2, row = i / 2;
    if (dx)
        col ^= 1;
    if (dy)
        row = (row + dy + rows) % rows;
    int j = row * 2 + col;
    return j < UI_CARDS ? j : row * 2;
}

static void draw_card(int i)
{
    static const char *const titles[UI_CARDS] = {"Stories", "Chat", "History", "Settings", "Benchmark", "About"};
    static const char *const subs[UI_CARDS] = {NULL, NULL, "Saved stories", "Sampling and models",
                                               "Speed test", "DSLM " DSLM_VERSION};
    static const uint8_t *const icons[UI_CARDS] = {icon_book, icon_bubble, icon_clock,
                                                   icon_gear, icon_gauge, icon_info};
    static const int iw[UI_CARDS] = {ICON_BOOK_W, ICON_BUBBLE_W, ICON_CLOCK_W, ICON_GEAR_W, ICON_GAUGE_W, ICON_INFO_W};
    static const int ih[UI_CARDS] = {ICON_BOOK_H, ICON_BUBBLE_H, ICON_CLOCK_H, ICON_GEAR_H, ICON_GAUGE_H, ICON_INFO_H};
    rect r = card_rect(i);
    int focus = i == st.home_focus;
    gfx_fill(&gfx_bot, r.x - 1, r.y - 1, r.w + 2, r.h + 2, COL_BG);
    if (focus) {
        gfx_round(&gfx_bot, r.x - 1, r.y - 1, r.w + 2, r.h + 2, COL_CYAN);
        gfx_round(&gfx_bot, r.x + 1, r.y + 1, r.w - 2, r.h - 2, COL_PANEL2);
    } else {
        gfx_round(&gfx_bot, r.x, r.y, r.w, r.h, COL_LINE);
        gfx_round(&gfx_bot, r.x, r.y + 1, r.w, r.h - 1, COL_PANEL);
    }
    /* icon on the left, title and subtitle on the right */
    gfx_blit_scaled(&gfx_bot, r.x + 6 + (32 - 2 * iw[i]) / 2, r.y + (r.h - 2 * ih[i]) / 2, iw[i], ih[i], icons[i], 2);
    gfx_str(&gfx_bot, r.x + 44, r.y + 12, titles[i], focus ? COL_WHITE : COL_GREY, 1);
    const char *sub = i < 2 ? st.home_sub[i] : subs[i];
    size_t n = strlen(sub);
    while (n && gfx_text_width(sub, n, 1) > r.w - 48)
        n--;
    gfx_text(&gfx_bot, r.x + 44, r.y + 27, sub, n, COL_DIM, 1);
}

void ui_home(int focus, const char *stories, const char *chat, const char *note)
{
    logo_top();
    st.home_focus = focus;
    snprintf(st.home_sub[0], sizeof st.home_sub[0], "%s", stories);
    snprintf(st.home_sub[1], sizeof st.home_sub[1], "%s", chat);
    clear_bottom();
    char hello[24];
    const char *name = ui_user_name();
    snprintf(hello, sizeof hello, "Hi, %s!", name);
    header("DSLM", COL_CYAN, name[0] ? hello : "D-pad + A or touch");
    for (int i = 0; i < UI_CARDS; i++)
        draw_card(i);
    if (note)
        gfx_str_centered(&gfx_bot, 0, 179, GFX_W, note, COL_AMBER, 1);
}

static const rect ABOUT_BACK = {68, 160, 120, 24};
static const rect REPORT_AGAIN = {8, 160, 118, 26};
static const rect REPORT_BACK = {130, 160, 118, 26};

void ui_report(const char *title, const char *const *lines, int n)
{
    clear_bottom();
    header(title, COL_CYAN, "DSLM " DSLM_VERSION);
    for (int i = 0; i < n && i < 11; i++)
        gfx_str(&gfx_bot, 10, 24 + i * 12, lines[i], i == 0 ? COL_WHITE : COL_GREY, 1);
    button(REPORT_AGAIN, "Run again", "A", BTN_PRIMARY, 1);
    button(REPORT_BACK, "Back", "B", BTN_NORMAL, 1);
}

/* History: six 22 px rows, then Delete and Back. */
#define LIST_Y 21
#define LIST_ROW_H 22
#define LIST_ROWS 6
static const rect HIST_DELETE = {8, 160, 118, 26};
static const rect HIST_BACK = {130, 160, 118, 26};

static rect list_row(int k)
{
    rect r = {4, LIST_Y + k * LIST_ROW_H, GFX_W - 8, LIST_ROW_H - 2};
    return r;
}

void ui_history(const ui_list_item *items, int n, int sel, int confirm)
{
    char right[24];
    if (sel < st.list_first)
        st.list_first = sel;
    if (sel >= st.list_first + LIST_ROWS)
        st.list_first = sel - LIST_ROWS + 1;
    if (st.list_first > (n > LIST_ROWS ? n - LIST_ROWS : 0) || st.list_first < 0)
        st.list_first = 0;
    st.list_n = n;
    clear_bottom();
    snprintf(right, sizeof right, n ? "%d / %d" : "empty", sel + 1, n);
    header("History", COL_CYAN, right);
    for (int k = 0; k < LIST_ROWS && st.list_first + k < n; k++) {
        int i = st.list_first + k;
        rect r = list_row(k);
        if (i == sel) {
            gfx_round(&gfx_bot, r.x, r.y, r.w, r.h, COL_PANEL2);
            gfx_fill(&gfx_bot, r.x, r.y + 3, 2, r.h - 6, COL_CYAN);
        }
        int ty = r.y + (r.h - 7) / 2;
        int x = gfx_str(&gfx_bot, r.x + 7, ty, items[i].left, COL_DIM, 1) + 6;
        const char *t = items[i].right;
        size_t len = strlen(t);
        while (len && x + gfx_text_width(t, len, 1) > r.x + r.w - 4)
            len--;
        gfx_text(&gfx_bot, x, ty, t, len, items[i].accent ? COL_CYAN_L : (i == sel ? COL_WHITE : COL_GREY), 1);
    }
    if (!n)
        gfx_str_centered(&gfx_bot, 0, 70, GFX_W, "Finished stories and answers appear here.", COL_DIM, 1);
    else
        button(HIST_DELETE, confirm ? "Again: delete" : "Delete", "Y", BTN_DANGER, 1);
    button(HIST_BACK, "Back", "B", BTN_NORMAL, 1);
}

void ui_about(const char *const *lines, int n)
{
    logo_top();
    clear_bottom();
    header("About", COL_CYAN, "DSLM " DSLM_VERSION);
    for (int i = 0; i < n && i < 11; i++)
        gfx_str(&gfx_bot, 10, 24 + i * 12, lines[i], i == 0 ? COL_WHITE : COL_GREY, 1);
    button(ABOUT_BACK, "Back", "B", BTN_NORMAL, 1);
}

/* ---- input -------------------------------------------------------------- */

static ui_event ev(ui_event_type t, int v)
{
    ui_event e = {t, v};
    return e;
}

/* What a keyboard key press means to the app; a page change redraws the keys. */
static ui_event key_event(int key)
{
    int code = kb_press(&st.kb, key);
    if (code == KC_DEL)
        return ev(UI_EV_BACKSPACE, 0);
    if (code == KC_GO)
        return ev(UI_EV_GO, 0);
    for (int i = 0; i < KB_KEYS; i++) /* the page or shift may have changed */
        draw_key(i);
    if (code <= 0)
        ui_sound(SFX_PAGE);
    return code > 0 ? ev(UI_EV_CHAR, code) : ev(UI_EV_NONE, 0);
}

static ui_event poll_input(ui_screen screen)
{
    int ime = REG_IME;
    REG_IME = 0;
    uint32_t down = latch_down;
    uint32_t rep = latch_rep;
    int tx = latch_tx, ty = latch_ty;
    latch_down = latch_rep = 0;
    latch_tx = latch_ty = -1;
    REG_IME = ime;
    uint32_t held = keysHeld();
    if (!(down & KEY_TOUCH))
        tx = ty = -1;
    touchPosition tp;

    switch (screen) {
    case UI_PROMPT:
        if (down & KEY_START)
            return ev(UI_EV_GO, 0);
        if (down & KEY_A)
            return key_event(st.kb.focus);
        if (rep & KEY_B)
            return ev(UI_EV_BACKSPACE, 0);
        if (down & KEY_Y)
            return ev(UI_EV_CHAR, ' ');
        if (down & KEY_X) {
            st.kb.mode = st.kb.mode == KB_LOWER ? KB_UPPER : KB_LOWER;
            for (int i = 0; i < KB_KEYS; i++)
                draw_key(i);
            ui_sound(SFX_PAGE);
            return ev(UI_EV_NONE, 0);
        }
        if (down & KEY_L)
            return ev(UI_EV_EXAMPLE_PREV, 0);
        if (down & KEY_R)
            return ev(UI_EV_EXAMPLE_NEXT, 0);
        if (down & KEY_SELECT)
            return ev(UI_EV_MENU, 0);
        if (rep & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
            int dx = rep & KEY_LEFT ? -1 : rep & KEY_RIGHT ? 1 : 0;
            int dy = dx ? 0 : rep & KEY_UP ? -1 : 1;
            focus_key(kb_move(st.kb.focus, dx, dy));
            return ev(UI_EV_NONE, 0);
        }
        if (ty >= 0) {
            if (hit(PR_PREV, tx, ty))
                return ev(UI_EV_EXAMPLE_PREV, 0);
            if (hit(PR_NEXT, tx, ty))
                return ev(UI_EV_EXAMPLE_NEXT, 0);
            if (hit(PR_SET, tx, ty))
                return ev(UI_EV_MENU, 0);
            if (hit(PR_GO, tx, ty))
                return ev(UI_EV_GO, 0);
            int key = kb_hit(tx, ty);
            if (key >= 0) {
                focus_key(key);
                return key_event(key);
            }
        }
        break;

    case UI_GENERATING:
        /* a tap latched since the last token counts, not only a held button */
        if ((down | held) & KEY_B)
            return ev(UI_EV_STOP, 0);
        if (ty >= 0 && hit(gen_stop(), tx, ty))
            return ev(UI_EV_STOP, 0);
        if (held & KEY_TOUCH) {
            touchRead(&tp);
            if (hit(gen_stop(), tp.px, tp.py))
                return ev(UI_EV_STOP, 0);
        }
        if ((down & KEY_X) && peek_mode)
            return ev(UI_EV_PEEK, 0);
        if ((down & KEY_Y) && peek_mode)
            return ev(UI_EV_COWRITE, 0);
        break;

    case UI_CHOOSE:
        if (rep & KEY_UP)
            return ev(UI_EV_UP, 0);
        if (rep & KEY_DOWN)
            return ev(UI_EV_DOWN, 0);
        if (down & KEY_A)
            return ev(UI_EV_PICK, -1);
        if (down & KEY_Y)
            return ev(UI_EV_COWRITE, 0);
        if ((down & KEY_B) || (ty >= 0 && hit(gen_stop(), tx, ty)))
            return ev(UI_EV_STOP, 0);
        for (int i = 0; i < choice_n; i++)
            if (hit(choice_row(i), tx, ty))
                return ev(UI_EV_PICK, i);
        break;

    case UI_HOME:
        if (rep & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
            int old = st.home_focus;
            st.home_focus = card_move(old, rep & (KEY_LEFT | KEY_RIGHT) ? 1 : 0,
                                      rep & KEY_UP ? -1 : rep & KEY_DOWN ? 1 : 0);
            ui_sound(SFX_MOVE);
            draw_card(old);
            draw_card(st.home_focus);
            return ev(UI_EV_NONE, 0);
        }
        if (down & (KEY_A | KEY_START))
            return ev(UI_EV_OPEN, st.home_focus);
        for (int i = 0; i < UI_CARDS; i++)
            if (hit(card_rect(i), tx, ty)) {
                int old = st.home_focus;
                st.home_focus = i;
                draw_card(old);
                draw_card(i);
                return ev(UI_EV_OPEN, i);
            }
        break;

    case UI_HISTORY:
        if (rep & KEY_UP)
            return ev(UI_EV_UP, 0);
        if (rep & KEY_DOWN)
            return ev(UI_EV_DOWN, 0);
        if (rep & KEY_L)
            return ev(UI_EV_PAGE_UP, 0);
        if (rep & KEY_R)
            return ev(UI_EV_PAGE_DOWN, 0);
        if (down & KEY_Y)
            return ev(UI_EV_DELETE, 0);
        if (down & KEY_B)
            return ev(UI_EV_BACK, 0);
        for (int k = 0; k < LIST_ROWS && st.list_first + k < st.list_n; k++)
            if (hit(list_row(k), tx, ty))
                return ev(UI_EV_SELECT_ROW, st.list_first + k);
        if (st.list_n && hit(HIST_DELETE, tx, ty))
            return ev(UI_EV_DELETE, 0);
        if (hit(HIST_BACK, tx, ty))
            return ev(UI_EV_BACK, 0);
        break;

    case UI_REPORT:
        if ((down & KEY_A) || hit(REPORT_AGAIN, tx, ty))
            return ev(UI_EV_AGAIN, 0);
        if ((down & KEY_B) || hit(REPORT_BACK, tx, ty))
            return ev(UI_EV_BACK, 0);
        break;

    case UI_ABOUT:
        if ((down & (KEY_A | KEY_B | KEY_START)) || hit(ABOUT_BACK, tx, ty))
            return ev(UI_EV_BACK, 0);
        break;

    case UI_STORY:
        if (down & KEY_A)
            return ev(UI_EV_NEW, 0);
        if (down & KEY_Y)
            return ev(UI_EV_AGAIN, 0);
        if (down & KEY_SELECT)
            return ev(UI_EV_MENU, 0);
        if (rep & KEY_UP)
            return ev(UI_EV_UP, 0);
        if (rep & KEY_DOWN)
            return ev(UI_EV_DOWN, 0);
        if (hit(STORY_NEW, tx, ty))
            return ev(UI_EV_NEW, 0);
        if (hit(STORY_AGAIN, tx, ty))
            return ev(UI_EV_AGAIN, 0);
        if (hit(STORY_SET, tx, ty))
            return ev(UI_EV_MENU, 0);
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

/* The sound of each event; focus moves and page changes play inside poll_input. */
static const signed char event_sound[] = {
    [UI_EV_NONE] = SFX_NONE,          [UI_EV_CHAR] = SFX_KEY,          [UI_EV_BACKSPACE] = SFX_DELETE,
    [UI_EV_GO] = SFX_START,           [UI_EV_STOP] = SFX_STOP,         [UI_EV_EXAMPLE_PREV] = SFX_PAGE,
    [UI_EV_EXAMPLE_NEXT] = SFX_PAGE,  [UI_EV_SETTINGS] = SFX_OPEN,     [UI_EV_BACK] = SFX_BACK,
    [UI_EV_NEW] = SFX_OPEN,           [UI_EV_AGAIN] = SFX_START,       [UI_EV_UP] = SFX_MOVE,
    [UI_EV_DOWN] = SFX_MOVE,          [UI_EV_LEFT] = SFX_MOVE,         [UI_EV_RIGHT] = SFX_MOVE,
    [UI_EV_SELECT_ROW] = SFX_MOVE,    [UI_EV_SAVE] = SFX_OPEN,         [UI_EV_OPEN] = SFX_OPEN,
    [UI_EV_MENU] = SFX_BACK,          [UI_EV_PAGE_UP] = SFX_MOVE,      [UI_EV_PAGE_DOWN] = SFX_MOVE,
    [UI_EV_DELETE] = SFX_DELETE,      [UI_EV_PEEK] = SFX_PAGE,         [UI_EV_COWRITE] = SFX_OPEN,
    [UI_EV_PICK] = SFX_KEY,
};

ui_event ui_poll(ui_screen screen)
{
    ui_event e = poll_input(screen);
    if (e.type != UI_EV_NONE)
        ui_sound(event_sound[e.type]);
    return e;
}

void ui_frame(void)
{
    swiWaitForVBlank();
    if (st.sleep_pending && !voice.playing) { /* finished in the dark: sleep now */
        st.sleep_pending = 0;
        st.dark = 0;
        powerOn(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
        if (keysHeld() & KEY_LID)
            systemSleep();
    }
}
