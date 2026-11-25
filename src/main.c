/*
 * DSLM app: load the model from the card, then loop over the screens
 * Prompt -> Generating -> Story (and Settings). Input arrives as ui events,
 * from buttons or touch alike (ui.h). Each run also writes /dslm/tokens.txt
 * so a prompt from the golden list can be checked with `dslm.py parity`.
 */
#include <dirent.h>
#include <malloc.h>
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

    void *ckpt;          /* checkpoint buffer (weights used in place) */
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

/* Free memory for a new allocation: never-claimed heap plus blocks malloc holds free. */
static size_t heap_available(void)
{
    struct mallinfo mi = mallinfo();
    return (size_t)(getHeapLimit() - getHeapEnd()) + (size_t)mi.fordblks;
}

static void unload_model(app *a)
{
    llama2_free(&a->model);
    free(a->ckpt);
    free(a->tokens);
    a->ckpt = NULL;
    a->tokens = NULL;
}

/*
 * Load settings.checkpoint and settings.tokenizer. Returns NULL, or an error
 * message (and leaves nothing allocated) so the caller can recover.
 */
static const char *load_model(app *a)
{
    static char err[320];
    unsigned char header[LLAMA2_PROBE_BYTES], tok_head[4];
    size_t ckpt_size, tok_size;
    llama2_config cfg;
    llama2_format fmt;
    dslm_settings *s = &a->settings;

    a->model_name = model_label(s->checkpoint);
    ui_title(a->model_name, "loading");

    sd_status st = sd_read_head(s->checkpoint, header, sizeof header, &ckpt_size);
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s. Copy the model to /dslm/models/ on the microSD card (see README).",
                 s->checkpoint, sd_status_text(st));
        return err;
    }
    st = sd_read_head(s->tokenizer, tok_head, sizeof tok_head, &tok_size);
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s. Copy the tokenizer to /dslm/models/ on the microSD card (see README).",
                 s->tokenizer, sd_status_text(st));
        return err;
    }
    ui_log("found %s", s->checkpoint);

    if (llama2_probe(header, sizeof header, &cfg, &fmt) != LLAMA2_OK) {
        snprintf(err, sizeof err, "%s is not a llama2.c checkpoint (fp32 or int8 v2).", s->checkpoint);
        return err;
    }
    if (llama2_file_bytes(&cfg, &fmt) != ckpt_size) {
        snprintf(err, sizeof err, "%s: %u bytes, but its header implies %u.", s->checkpoint,
                 (unsigned)ckpt_size, (unsigned)llama2_file_bytes(&cfg, &fmt));
        return err;
    }
    ui_log("%s, dim %d layers %d vocab %d seq %d", fmt.quantized ? "int8" : "fp32", cfg.dim,
           cfg.n_layers, cfg.vocab_size, cfg.seq_len);

    size_t need = ckpt_size + 1 + tok_size + 1 + llama2_runtime_bytes(&cfg, &fmt, tok_size) +
                  (size_t)(cfg.seq_len + 1) * sizeof(int);
    size_t avail = heap_available();
    ui_log("RAM need %u / free %u", (unsigned)need, (unsigned)avail);
    if (need > avail) {
        snprintf(err, sizeof err, "Model does not fit: needs %u bytes, %u available.", (unsigned)need,
                 (unsigned)avail);
        return err;
    }

    void *tok;
    cpuStartTiming(0);
    st = sd_read_file_progress(s->checkpoint, ckpt_size, &a->ckpt, &ckpt_size, load_progress, "model");
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s", s->checkpoint, sd_status_text(st));
        a->ckpt = NULL;
        return err;
    }
    st = sd_read_file_progress(s->tokenizer, tok_size, &tok, &tok_size, load_progress, "tokenizer");
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s", s->tokenizer, sd_status_text(st));
        unload_model(a);
        return err;
    }
    a->load_ms = ticks_to_ms(cpuGetTiming());

    llama2_status ls = llama2_init(&a->model, a->ckpt, ckpt_size, tok, tok_size, s->temperature, s->topp,
                                   s->seed ? s->seed : 1);
    free(tok);
    if (ls != LLAMA2_OK) {
        snprintf(err, sizeof err, "Model init failed (%d); is %s the right tokenizer?", (int)ls, s->tokenizer);
        unload_model(a);
        return err;
    }
    a->tokens = malloc((size_t)(cfg.seq_len + 1) * sizeof(int));
    if (!a->tokens) {
        unload_model(a);
        return "No memory for the token buffer.";
    }
    ui_log("loaded in %u ms", a->load_ms);
    return NULL;
}

static void boot(app *a)
{
    ui_title("", "loading");
    ui_hint("Reading the microSD card...");
    if (!sd_mount())
        ui_fatal("FAT mount failed. The binary may need a manual DLDI patch (dlditool).");
    ui_log("DSLM %s - FAT mounted", DSLM_VERSION);
    read_settings(&a->settings);
    const char *err = load_model(a);
    if (err)
        ui_fatal("%s", err);
}

/* Checkpoints in /dslm/models that the loader accepts (header and size checked). */
#define MODELS_DIR "/dslm/models"
#define MAX_MODELS 16

typedef struct {
    char path[DSLM_PATH_MAX];
    int quantized;
} model_entry;

static int scan_models(model_entry *list, int max)
{
    DIR *dir = opendir(MODELS_DIR);
    int n = 0;
    if (!dir)
        return 0;
    for (struct dirent *e; n < max && (e = readdir(dir)) != NULL;) {
        size_t len = strlen(e->d_name);
        if (len < 5 || strcasecmp(e->d_name + len - 4, ".bin") != 0)
            continue;
        char path[DSLM_PATH_MAX];
        if (snprintf(path, sizeof path, "%s/%s", MODELS_DIR, e->d_name) >= (int)sizeof path)
            continue;
        unsigned char head[LLAMA2_PROBE_BYTES];
        size_t size;
        llama2_config cfg;
        llama2_format fmt;
        if (sd_read_head(path, head, sizeof head, &size) != SD_OK ||
            llama2_probe(head, sizeof head, &cfg, &fmt) != LLAMA2_OK || llama2_file_bytes(&cfg, &fmt) != size)
            continue; /* the tokenizer and anything else that is not a checkpoint */
        strcpy(list[n].path, path);
        list[n].quantized = fmt.quantized;
        n++;
    }
    closedir(dir);
    return n;
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

static const char *model_note(const model_entry *m)
{
    static char note[40];
    const char *slash = strrchr(m->path, '/');
    snprintf(note, sizeof note, "%.26s %s", slash ? slash + 1 : m->path, m->quantized ? "int8" : "fp32");
    return note;
}

/* Swap to edit's checkpoint; on failure reload the previous one. Returns the note to show. */
static const char *switch_model(app *a, const dslm_settings *edit)
{
    static char note[64];
    char previous[DSLM_PATH_MAX];
    strcpy(previous, a->settings.checkpoint);
    a->settings = *edit;
    unload_model(a);
    a->story_len = 0;
    a->story[0] = '\0';
    const char *err = load_model(a);
    if (err) {
        strcpy(a->settings.checkpoint, previous);
        const char *again = load_model(a);
        if (again)
            ui_fatal("%s", again);
        /* the reason is longer than the note line: show it on the top screen */
        snprintf(a->story, STORY_CAP, "Could not switch model:\n\n%s\n\nThe previous model is loaded again.", err);
        a->story_len = strlen(a->story);
        return "Not loaded, kept previous model";
    }
    char buf[512];
    size_t n = dslm_ini_format(&a->settings, buf, sizeof buf);
    snprintf(note, sizeof note, n && sd_write_file(INI_PATH, buf, n) == SD_OK ? "Loaded and saved" : "Loaded (not saved)");
    return note;
}

/* Returns when the user leaves the settings screen. */
static void settings_screen(app *a)
{
    dslm_settings edit = a->settings;
    model_entry models[MAX_MODELS];
    int n_models = scan_models(models, MAX_MODELS);
    int current = -1;
    for (int i = 0; i < n_models; i++)
        if (strcmp(models[i].path, edit.checkpoint) == 0)
            current = i;
    int cursor = 0;
    const char *note = n_models > 1 ? NULL : "Only one model in /dslm/models";
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
            note = NULL;
            if (row == UI_SETTINGS_MODEL_ROW) {
                if (n_models > 0) {
                    int dir = e.type == UI_EV_RIGHT ? 1 : -1;
                    current = current < 0 ? 0 : (current + dir + n_models) % n_models;
                    strcpy(edit.checkpoint, models[current].path);
                    note = model_note(&models[current]);
                }
            } else {
                adjust(&edit, row, e.type == UI_EV_RIGHT ? 1 : -1, a->model.config.seq_len);
            }
            break;
        case UI_EV_SAVE:
            if (strcmp(edit.checkpoint, a->settings.checkpoint) != 0) {
                note = switch_model(a, &edit);
                edit = a->settings;
                /* clear the loading bar; after a failure, show why */
                if (a->story_len)
                    ui_text(a->story, a->story_len, 0);
                else
                    ui_text(welcome, sizeof welcome - 1, 0);
                ui_title(a->model_name, "settings");
                ui_hint("START:save  B:back");
            } else {
                char buf[512];
                size_t n = dslm_ini_format(&edit, buf, sizeof buf);
                a->settings = edit;
                note = n && sd_write_file(INI_PATH, buf, n) == SD_OK ? "Saved to /dslm/dslm.ini"
                                                                      : "Applied (could not save)";
            }
            break;
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
    boot(&a);
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
