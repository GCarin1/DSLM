/*
 * DSLM app: load the model from the card, then loop over the screens
 * Prompt -> Generating -> Story (and Settings). Input arrives as ui events,
 * from buttons or touch alike (ui.h). Each run also writes /dslm/tokens.txt
 * so a prompt from the golden list can be checked with `dslm.py parity`.
 */
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/llama2.h"
#include "storage/ini.h"
#include "storage/sdcard.h"
#include "ui/textview.h"
#include "ui/ui.h"
#include "version.h"

#define INI_PATH "/dslm/dslm.ini"
#define INI_MAX_SIZE 4096
#define TOKENS_PATH "/dslm/tokens.txt"
#define BENCH_PATH "/dslm/bench.txt"
#define STORY_CAP 8192

static const char *const examples[] = {
    "Once upon a time",
    "The little dog",
    "Lily wanted to",
    "Tom and his mom went to the park",
};
#define N_EXAMPLES ((int)(sizeof examples / sizeof examples[0]))

typedef struct {
    llama2 model;
    dslm_settings settings;
    const char *model_name;

    char prompt[UI_PROMPT_MAX + 1];
    int example;

    char story[STORY_CAP];
    size_t story_len;
    int scroll;          /* first story line shown on the top screen */

    int *tokens;         /* last run, BOS first */
    int n_tokens;
    int n_prompt_tokens;
    int stopped;
    unsigned gen_ms;
    unsigned load_ms;
} app;

/* ---- loading ------------------------------------------------------------ */

static void load_progress(size_t done, size_t total, void *user)
{
    ui_loading((const char *)user, done, total);
}

static unsigned ticks_to_ms(uint64_t ticks)
{
    return (unsigned)(ticks * 1000 / BUS_CLOCK);
}

static void read_settings(dslm_settings *s)
{
    void *text = NULL;
    size_t len = 0;
    dslm_settings_defaults(s);
    sd_status st = sd_read_file(INI_PATH, INI_MAX_SIZE, &text, &len);
    if (st == SD_OK) {
        dslm_ini_parse(text, len, s);
        free(text);
        ui_log("read %s (%u bytes)", INI_PATH, (unsigned)len);
    } else {
        ui_log("%s: %s, using defaults", INI_PATH, sd_status_text(st));
    }
    ui_log("temp x1000: %d  topp x1000: %d", (int)(s->temperature * 1000), (int)(s->topp * 1000));
    ui_log("steps: %d  seed: %u", s->steps, (unsigned)s->seed);
}

/* "/dslm/models/stories260K.bin" -> "stories260K" for the title bar. */
static const char *model_label(const char *path)
{
    static char label[DSLM_PATH_MAX];
    const char *slash = strrchr(path, '/');
    snprintf(label, sizeof label, "%s", slash ? slash + 1 : path);
    char *dot = strrchr(label, '.');
    if (dot && dot != label)
        *dot = '\0';
    return label;
}

static void load(app *a)
{
    unsigned char header[LLAMA2_PROBE_BYTES], tok_head[4];
    size_t ckpt_size, tok_size;
    llama2_config cfg;
    llama2_format fmt;
    dslm_settings *s = &a->settings;

    ui_title("", "loading");
    ui_hint("Reading the microSD card...");
    if (!sd_mount())
        ui_fatal("FAT mount failed. The binary may need a manual DLDI patch (dlditool).");
    ui_log("DSLM %s - FAT mounted", DSLM_VERSION);
    read_settings(s);
    a->model_name = model_label(s->checkpoint);
    ui_title(a->model_name, "loading");

    sd_status st = sd_read_head(s->checkpoint, header, sizeof header, &ckpt_size);
    if (st != SD_OK)
        ui_fatal("%s: %s. Copy the model to /dslm/models/ on the microSD card (see README).",
                 s->checkpoint, sd_status_text(st));
    st = sd_read_head(s->tokenizer, tok_head, sizeof tok_head, &tok_size);
    if (st != SD_OK)
        ui_fatal("%s: %s. Copy the tokenizer to /dslm/models/ on the microSD card (see README).",
                 s->tokenizer, sd_status_text(st));
    ui_log("found %s", s->checkpoint);
    ui_log("found %s", s->tokenizer);

    if (llama2_probe(header, sizeof header, &cfg, &fmt) != LLAMA2_OK)
        ui_fatal("%s is not a llama2.c checkpoint (fp32 or int8 v2).", s->checkpoint);
    if (llama2_file_bytes(&cfg, &fmt) != ckpt_size)
        ui_fatal("%s: %u bytes, but its header implies %u.", s->checkpoint, (unsigned)ckpt_size,
                 (unsigned)llama2_file_bytes(&cfg, &fmt));
    if (fmt.quantized)
        ui_log("int8 checkpoint, group size %d", fmt.group_size);
    else
        ui_log("fp32 checkpoint");

    size_t need = ckpt_size + 1 + tok_size + 1 + llama2_runtime_bytes(&cfg, &fmt, tok_size) +
                  (size_t)(cfg.seq_len + 1) * sizeof(int);
    size_t avail = (size_t)(getHeapLimit() - getHeapEnd());
    ui_log("dim %d layers %d heads %d vocab %d seq %d", cfg.dim, cfg.n_layers, cfg.n_heads,
           cfg.vocab_size, cfg.seq_len);
    ui_log("RAM need %u / free %u", (unsigned)need, (unsigned)avail);
    if (need > avail)
        ui_fatal("Model does not fit: needs %u bytes, %u available.", (unsigned)need, (unsigned)avail);

    void *ckpt, *tok;
    cpuStartTiming(0);
    st = sd_read_file_progress(s->checkpoint, ckpt_size, &ckpt, &ckpt_size, load_progress, "model");
    if (st != SD_OK)
        ui_fatal("%s: %s", s->checkpoint, sd_status_text(st));
    st = sd_read_file_progress(s->tokenizer, tok_size, &tok, &tok_size, load_progress, "tokenizer");
    if (st != SD_OK)
        ui_fatal("%s: %s", s->tokenizer, sd_status_text(st));
    a->load_ms = ticks_to_ms(cpuGetTiming());

    llama2_status ls = llama2_init(&a->model, ckpt, ckpt_size, tok, tok_size, s->temperature, s->topp,
                                   s->seed ? s->seed : 1);
    free(tok);
    if (ls != LLAMA2_OK)
        ui_fatal("Model init failed (%d).", (int)ls);
    a->tokens = malloc((size_t)(cfg.seq_len + 1) * sizeof(int));
    if (!a->tokens)
        ui_fatal("No memory for the token buffer.");
    ui_log("loaded in %u ms", a->load_ms);
}

/* ---- generation --------------------------------------------------------- */

typedef struct {
    app *a;
    int produced;
    int steps;
    int stopped;
    uint64_t ticks;
} gen_ctx;

static int on_token(int token, const char *piece, void *user)
{
    gen_ctx *g = user;
    (void)token;
    g->ticks += cpuGetTiming();
    cpuStartTiming(0);
    g->produced++;
    if (piece[0]) {
        tv_append(g->a->story, &g->a->story_len, STORY_CAP, piece);
        ui_text(g->a->story, g->a->story_len, -1);
    }
    ui_panel_generating(g->produced, g->steps, ticks_to_ms(g->ticks) / (unsigned)g->produced);
    g->stopped = ui_poll(UI_GENERATING).type == UI_EV_STOP;
    return g->stopped;
}

/* contracts/model-file-format: one block per run appended to /dslm/bench.txt */
static void append_bench(app *a, int steps)
{
    char buf[512];
    int positions = a->n_tokens > 1 ? a->n_tokens - 1 : 0;
    unsigned tps100 = a->gen_ms ? (unsigned)((uint64_t)positions * 100000 / a->gen_ms) : 0;
    unsigned heap_peak = (unsigned)(getHeapEnd() - getHeapStart());
    int len = snprintf(buf, sizeof buf,
                       "version=%s\nbuild=%s\nemulated=unknown\ncheckpoint=%s\nprompt=%s\n"
                       "steps=%d\nstopped=%d\nload_ms=%u\ntokens=%d\ngen_ms=%u\n"
                       "tok_per_s=%u.%02u\nheap_peak_bytes=%u\n\n",
                       DSLM_VERSION, a->model.format.quantized ? "int8" : "fp32",
                       a->settings.checkpoint, a->prompt, steps, a->stopped, a->load_ms, a->n_tokens,
                       a->gen_ms, tps100 / 100, tps100 % 100, heap_peak);
    if (len > 0 && (size_t)len < sizeof buf)
        sd_append_file(BENCH_PATH, buf, (size_t)len);
}

static void write_tokens(app *a, int steps)
{
    size_t cap = 256 + DSLM_PATH_MAX + strlen(a->prompt) + (size_t)a->n_tokens * 6;
    char *buf = malloc(cap);
    if (!buf)
        return;
    int len = snprintf(buf, cap, "checkpoint=%s\nprompt=%s\nsteps=%d\nprompt_tokens=%d\ntokens=",
                       a->settings.checkpoint, a->prompt, steps, a->n_prompt_tokens);
    for (int i = 0; i < a->n_tokens; i++)
        len += snprintf(buf + len, cap - len, i ? " %d" : "%d", a->tokens[i]);
    len += snprintf(buf + len, cap - len, "\nstopped=%d\nload_ms=%u\ngen_ms=%u\n", a->stopped,
                    a->load_ms, a->gen_ms);
    sd_write_file(TOKENS_PATH, buf, (size_t)len);
    free(buf);
}

static void generate(app *a)
{
    int seq_len = a->model.config.seq_len;
    int steps = a->settings.steps > seq_len ? seq_len : a->settings.steps;
    gen_ctx g = {a, 0, steps, 0, 0};

    a->model.temperature = a->settings.temperature;
    a->model.topp = a->settings.topp;
    a->story_len = 0;
    a->story[0] = '\0';
    ui_title(a->model_name, "generating");
    ui_hint("B or STOP: stop");
    ui_text("", 0, 0);
    ui_panel_generating(0, steps, 0);

    cpuStartTiming(0);
    a->n_tokens = llama2_generate(&a->model, a->prompt, steps, a->tokens, seq_len + 1,
                                  &a->n_prompt_tokens, on_token, &g);
    g.ticks += cpuGetTiming();
    a->gen_ms = ticks_to_ms(g.ticks);
    a->stopped = g.stopped;
    write_tokens(a, steps);
    append_bench(a, steps);
}

/* ---- screens ------------------------------------------------------------ */

static const char welcome[] =
    "Welcome to DSLM " DSLM_VERSION ", a tiny Llama 2 running offline on your Nintendo DS.\n\n"
    "Type the start of a story on the touch keyboard, or press L/R for examples. "
    "START or GO writes the rest.\n\n"
    "The model only knows simple children's stories in English.";

static void enter_prompt(app *a, int show_story)
{
    ui_title(a->model_name, "ready");
    ui_hint("START:go  L/R:examples  SEL:set");
    if (show_story && a->story_len)
        ui_text(a->story, a->story_len, a->scroll);
    else
        ui_text(welcome, sizeof welcome - 1, 0);
    ui_panel_prompt(a->prompt);
}

static void enter_story(app *a)
{
    ui_title(a->model_name, a->stopped ? "stopped" : "done");
    ui_hint("Up/Down:scroll  A:new  Y:again");
    a->scroll = -1;
    int total = ui_text(a->story, a->story_len, a->scroll);
    a->scroll = total > UI_TEXT_ROWS ? total - UI_TEXT_ROWS : 0;
    ui_panel_story(a->n_tokens, a->gen_ms, a->stopped);
}

static void scroll_story(app *a, int delta)
{
    int total = tv_wrap(a->story, a->story_len, 32, NULL, 0);
    int max = total > UI_TEXT_ROWS ? total - UI_TEXT_ROWS : 0;
    a->scroll += delta;
    if (a->scroll < 0)
        a->scroll = 0;
    if (a->scroll > max)
        a->scroll = max;
    ui_text(a->story, a->story_len, a->scroll);
}

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void adjust(dslm_settings *s, int row, int dir, int seq_len)
{
    switch (row) {
    case 0:
        s->temperature = clampf(s->temperature + 0.1f * dir, 0.0f, 2.0f);
        if (s->temperature < 0.05f)
            s->temperature = 0.0f;
        break;
    case 1:
        s->topp = clampf(s->topp + 0.05f * dir, 0.05f, 1.0f);
        break;
    case 2:
        s->steps += 16 * dir;
        if (s->steps < 16)
            s->steps = 16;
        if (s->steps > seq_len)
            s->steps = seq_len;
        break;
    }
}

/* Returns when the user leaves the settings screen. */
static void settings_screen(app *a)
{
    dslm_settings edit = a->settings;
    int cursor = 0;
    const char *note = NULL;
    ui_title(a->model_name, "settings");
    ui_hint("START:save  B:back");
    ui_panel_settings(&edit, cursor, note);
    for (;;) {
        ui_frame();
        ui_event e = ui_poll(UI_SETTINGS);
        int row = e.value >= 0 ? e.value : cursor;
        switch (e.type) {
        case UI_EV_UP:
            cursor = (cursor + UI_SETTINGS_ROWS - 1) % UI_SETTINGS_ROWS;
            break;
        case UI_EV_DOWN:
            cursor = (cursor + 1) % UI_SETTINGS_ROWS;
            break;
        case UI_EV_SELECT_ROW:
            cursor = e.value;
            break;
        case UI_EV_LEFT:
        case UI_EV_RIGHT:
            cursor = row;
            adjust(&edit, row, e.type == UI_EV_RIGHT ? 1 : -1, a->model.config.seq_len);
            note = NULL;
            break;
        case UI_EV_SAVE: {
            char buf[512];
            size_t n = dslm_ini_format(&edit, buf, sizeof buf);
            a->settings = edit;
            note = n && sd_write_file(INI_PATH, buf, n) == SD_OK ? "Saved to /dslm/dslm.ini"
                                                                  : "Applied (could not save)";
            break;
        }
        case UI_EV_BACK:
            return;
        default:
            continue;
        }
        ui_panel_settings(&edit, cursor, note);
    }
}

int main(void)
{
    static app a;

    ui_init();
    load(&a);
    strcpy(a.prompt, examples[0]);
    a.example = 0;
    a.scroll = 0;

    enter_prompt(&a, 0);
    for (;;) {
        ui_frame();
        ui_event e = ui_poll(UI_PROMPT);
        size_t len = strlen(a.prompt);
        switch (e.type) {
        case UI_EV_CHAR:
            if (len < UI_PROMPT_MAX) {
                a.prompt[len] = (char)e.value;
                a.prompt[len + 1] = '\0';
                ui_panel_prompt(a.prompt);
            }
            break;
        case UI_EV_BACKSPACE:
            if (len > 0) {
                a.prompt[len - 1] = '\0';
                ui_panel_prompt(a.prompt);
            }
            break;
        case UI_EV_EXAMPLE_PREV:
        case UI_EV_EXAMPLE_NEXT:
            a.example = (a.example + (e.type == UI_EV_EXAMPLE_NEXT ? 1 : N_EXAMPLES - 1)) % N_EXAMPLES;
            strcpy(a.prompt, examples[a.example]);
            ui_panel_prompt(a.prompt);
            break;
        case UI_EV_UP:
        case UI_EV_DOWN:
            if (a.story_len)
                scroll_story(&a, e.type == UI_EV_DOWN ? 1 : -1);
            break;
        case UI_EV_SETTINGS:
            settings_screen(&a);
            enter_prompt(&a, 1);
            break;
        case UI_EV_GO:
            /* Story loop: generate, then New / Again / Settings. */
            for (int again = 1; again;) {
                generate(&a);
                enter_story(&a);
                again = 0;
                for (int waiting = 1; waiting;) {
                    ui_frame();
                    ui_event s = ui_poll(UI_STORY);
                    switch (s.type) {
                    case UI_EV_UP:
                    case UI_EV_DOWN:
                        scroll_story(&a, s.type == UI_EV_DOWN ? 1 : -1);
                        break;
                    case UI_EV_AGAIN:
                        again = 1;
                        waiting = 0;
                        break;
                    case UI_EV_SETTINGS:
                        settings_screen(&a);
                        enter_story(&a);
                        break;
                    case UI_EV_NEW:
                        waiting = 0;
                        break;
                    default:
                        break;
                    }
                }
            }
            enter_prompt(&a, 1);
            break;
        default:
            break;
        }
    }
}
