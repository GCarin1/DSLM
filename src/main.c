/*
 * DSLM app: load the model from the card, then loop over the screens
 * Prompt -> Generating -> Story (and Settings). Input arrives as ui events,
 * from buttons or touch alike (ui.h). Each run also writes /dslm/tokens.txt
 * so a prompt from the golden list can be checked with `dslm.py parity`.
 * Two model kinds: llama2.c story models (fp32 or int8) and LittleKedi-style
 * chat models (MLQ1, src/engine/mlq_chat.h), whose prompt is a question.
 */
#include <dirent.h>
#include <malloc.h>
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

#include "engine/bpe.h"
#include "engine/llama2.h"
#include "engine/mlq_chat.h"
#include "engine/topk.h"
#include "storage/history.h"
#include "storage/ini.h"
#include "storage/sdcard.h"
#include "ui/mic.h"
#include "ui/sfx.h"
#include "ui/textview.h"
#include "ui/ui.h"
#include "version.h"

#define INI_PATH "/dslm/dslm.ini"
#define INI_MAX_SIZE 4096
#define TOKENS_PATH "/dslm/tokens.txt"
#define BENCH_PATH "/dslm/bench.txt"
#define STORY_CAP 8192
#define HEAP_MARGIN (64 * 1024) /* left free for the UI, file I/O and stack growth */
#define MIN_CHAT_CONTEXT 128

typedef enum { KIND_STORY, KIND_CHAT } model_kind;

#define N_EXAMPLES 4
static const char *examples[2][N_EXAMPLES] = { /* [0][2] gets the user's name at boot */
    {"Once upon a time", "The little dog", "Lily wanted to", "Tom and his mom went to the park"},
    /* the first one is a golden prompt: the melonDS parity run presses START on it */
    {"What causes the seasons?", "Hello! Who are you?", "Give me three tips to sleep better.",
     "Write a short story about a cat."},
};

typedef struct {
    model_kind kind;
    llama2 model;             /* KIND_STORY */
    mlq_model mlq;            /* KIND_CHAT */
    bpe_tokenizer bpe;
    void *tok_file;           /* KIND_CHAT: the tokenizer pieces point into it */
    mlq_sampling sampling;    /* KIND_CHAT: its generator state lasts across runs */
    int seq_len;              /* context of the loaded model */
    dslm_settings settings;
    const char *model_name;
    char ckpt_path[DSLM_PATH_MAX]; /* the checkpoint loaded: settings.checkpoint or chat_checkpoint */
    int chat_q;                    /* KIND_CHAT: length of the question at the start of story */
    int benchmark;                 /* the run is the Benchmark card's: not saved to history */
    int gen_tokens;                /* n_tokens when the generation ended (paths may add more) */
    int mic;                       /* voice mode is on (prompt screen) */
    int voice_milli;               /* > 0: temperature x1000 set by a blow, for the next run only */

    char prompt[UI_PROMPT_MAX + 1];
    char run_prompt[UI_PROMPT_MAX + 64]; /* what the engine reads: chat markers added */
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
    mlq_free(&a->mlq);
    bpe_free(&a->bpe);
    free(a->tok_file);
    free(a->ckpt);
    free(a->tokens);
    memset(&a->model, 0, sizeof a->model);
    memset(&a->mlq, 0, sizeof a->mlq);
    memset(&a->bpe, 0, sizeof a->bpe);
    a->tok_file = NULL;
    a->ckpt = NULL;
    a->tokens = NULL;
}

/* "/dslm/models/x.bin" + "x.tok" -> "/dslm/models/x.tok" */
static int sibling_path(char *out, size_t cap, const char *path, const char *name)
{
    const char *slash = strrchr(path, '/');
    int dir = slash ? (int)(slash - path) : 0;
    int n = snprintf(out, cap, "%.*s/%s", dir, path, name);
    return n > 0 && (size_t)n < cap ? 0 : -1;
}

/* An MLQ1 chat model and the tokenizer its header names. Same contract as load_model. */
static const char *load_chat_model(app *a, const unsigned char *header, size_t ckpt_size)
{
    static char err[320];
    static char tok_path[DSLM_PATH_MAX];
    const char *ckpt_path = a->ckpt_path;
    unsigned char tok_head[BPE_HEADER_BYTES];
    char name[48];
    size_t tok_size;
    mla_config c;
    int gs;

    if (mlq_read_config(header, MLA_HEADER_BYTES, &c, &gs) != MLA_OK) {
        snprintf(err, sizeof err, "%s: unsupported chat model header.", ckpt_path);
        return err;
    }
    if (mlq_file_bytes(&c, gs) != ckpt_size) {
        snprintf(err, sizeof err, "%s: %u bytes, but its header implies %u.", ckpt_path, (unsigned)ckpt_size,
                 (unsigned)mlq_file_bytes(&c, gs));
        return err;
    }
    if (mlq_tokenizer_name(header, MLA_HEADER_BYTES, name, sizeof name) != 0 ||
        sibling_path(tok_path, sizeof tok_path, ckpt_path, name) != 0) {
        snprintf(err, sizeof err, "%s does not name its tokenizer.", ckpt_path);
        return err;
    }
    sd_status st = sd_read_head(tok_path, tok_head, sizeof tok_head, &tok_size);
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s. Copy it next to %s (see README).", tok_path, sd_status_text(st),
                 ckpt_path);
        return err;
    }
    size_t bpe_bytes = bpe_runtime_bytes(tok_head);
    if (!bpe_bytes) {
        snprintf(err, sizeof err, "%s is not a chat tokenizer (BPE1).", tok_path);
        return err;
    }
    ui_log("chat int8, dim %d layers %d experts %d vocab %d", c.dim, c.n_layers, c.n_experts, c.vocab);

    /* the context takes whatever RAM is left after the files and the tokenizer */
    size_t fixed = ckpt_size + 1 + tok_size + 1 + bpe_bytes + (size_t)(c.max_seq + 1) * sizeof(int) + HEAP_MARGIN;
    size_t avail = heap_available();
    int seq = avail > fixed ? mlq_fit_seq(&c, avail - fixed) : 0;
    ui_log("RAM files+tok %u / free %u, context %d", (unsigned)fixed, (unsigned)avail, seq);
    if (seq < MIN_CHAT_CONTEXT) {
        snprintf(err, sizeof err, "Model does not fit: needs %u bytes plus its context, %u available.",
                 (unsigned)fixed, (unsigned)avail);
        return err;
    }

    cpuStartTiming(0);
    st = sd_read_file_progress(ckpt_path, ckpt_size, &a->ckpt, &ckpt_size, load_progress, "model");
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s", ckpt_path, sd_status_text(st));
        a->ckpt = NULL;
        return err;
    }
    st = sd_read_file_progress(tok_path, tok_size, &a->tok_file, &tok_size, load_progress, "tokenizer");
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s", tok_path, sd_status_text(st));
        a->tok_file = NULL;
        unload_model(a);
        return err;
    }
    a->load_ms = ticks_to_ms(cpuGetTiming());

    mla_status ms = mlq_init(&a->mlq, a->ckpt, ckpt_size, seq);
    if (ms != MLA_OK || bpe_init(&a->bpe, a->tok_file, tok_size) != 0) {
        snprintf(err, sizeof err, "Chat model init failed (%d); is %s the right tokenizer?", (int)ms, tok_path);
        unload_model(a);
        return err;
    }
    a->tokens = malloc((size_t)(seq + 1) * sizeof(int));
    if (!a->tokens) {
        unload_model(a);
        return "No memory for the token buffer.";
    }
    mlq_sampling_init(&a->sampling, a->settings.temperature, a->settings.topp, MLQ_REPEAT_PENALTY,
                      a->settings.seed ? a->settings.seed : 1);
    a->kind = KIND_CHAT;
    a->seq_len = seq;
    ui_log("loaded in %u ms", a->load_ms);
    return NULL;
}

/*
 * Load the mode's checkpoint (settings.checkpoint with settings.tokenizer, or
 * settings.chat_checkpoint with the tokenizer it names). Returns NULL, or an error
 * message (and leaves nothing allocated) so the caller can recover.
 */
static const char *load_model(app *a)
{
    static char err[320];
    unsigned char header[MLA_HEADER_BYTES], tok_head[4];
    size_t ckpt_size, tok_size;
    llama2_config cfg;
    llama2_format fmt;
    dslm_settings *s = &a->settings;

    snprintf(a->ckpt_path, sizeof a->ckpt_path, "%s",
             s->mode == DSLM_MODE_CHAT ? s->chat_checkpoint : s->checkpoint);
    a->model_name = model_label(a->ckpt_path);
    ui_splash(a->model_name);

    sd_status st = sd_read_head(a->ckpt_path, header, sizeof header, &ckpt_size);
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s. Copy the model to /dslm/models/ on the microSD card (see README).",
                 a->ckpt_path, sd_status_text(st));
        return err;
    }
    if (memcmp(header, "MLQ1", 4) == 0)
        return load_chat_model(a, header, ckpt_size);
    st = sd_read_head(s->tokenizer, tok_head, sizeof tok_head, &tok_size);
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s. Copy the tokenizer to /dslm/models/ on the microSD card (see README).",
                 s->tokenizer, sd_status_text(st));
        return err;
    }
    ui_log("found %s", a->ckpt_path);

    if (llama2_probe(header, sizeof header, &cfg, &fmt) != LLAMA2_OK) {
        snprintf(err, sizeof err, "%s is not a llama2.c checkpoint (fp32 or int8 v2).", a->ckpt_path);
        return err;
    }
    if (llama2_file_bytes(&cfg, &fmt) != ckpt_size) {
        snprintf(err, sizeof err, "%s: %u bytes, but its header implies %u.", a->ckpt_path,
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
    st = sd_read_file_progress(a->ckpt_path, ckpt_size, &a->ckpt, &ckpt_size, load_progress, "model");
    if (st != SD_OK) {
        snprintf(err, sizeof err, "%s: %s", a->ckpt_path, sd_status_text(st));
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
    a->kind = KIND_STORY;
    a->seq_len = cfg.seq_len;
    ui_log("loaded in %u ms", a->load_ms);
    return NULL;
}

static void boot(app *a)
{
    ui_splash("");
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
    const char *kind; /* "fp32", "int8" or "chat" */
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
        unsigned char head[MLA_HEADER_BYTES];
        size_t size;
        llama2_config cfg;
        llama2_format fmt;
        mla_config mc;
        int gs;
        if (sd_read_head(path, head, sizeof head, &size) != SD_OK)
            continue;
        if (llama2_probe(head, sizeof head, &cfg, &fmt) == LLAMA2_OK && llama2_file_bytes(&cfg, &fmt) == size)
            list[n].kind = fmt.quantized ? "int8" : "fp32";
        else if (mlq_read_config(head, sizeof head, &mc, &gs) == MLA_OK && mlq_file_bytes(&mc, gs) == size)
            list[n].kind = "chat";
        else
            continue; /* tokenizers and anything else that is not a model */
        strcpy(list[n].path, path);
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
    /* peek: the likely tokens of the position being generated */
    int peek_n, peek_prev;
    int peek_ids[5], peek_pm[5];
    int cowrite;   /* the user picks the next tokens */
    int stop_req;  /* B while choosing: stop at the next token */
} gen_ctx;

static void peek_texts(gen_ctx *g, const char **pieces);
static void gen_panel(gen_ctx *g);

/* Story engine step hook: note the five likely tokens; never changes the pick. */
/* Co-writing: offer the likely tokens and wait for the user. Returns the
 * token to force, or -1 to let the model sample (pen given back, or stop). */
static int choose_token(gen_ctx *g)
{
    const char *pieces[5];
    int sel = 0;
    g->ticks += cpuGetTiming(); /* the wait is not generation time */
    peek_texts(g, pieces);
    ui_choices(pieces, g->peek_pm, g->peek_n, sel);
    for (;;) {
        ui_frame();
        ui_event e = ui_poll(UI_CHOOSE);
        int pick = -2;
        switch (e.type) {
        case UI_EV_UP:
            sel = sel > 0 ? sel - 1 : g->peek_n - 1;
            break;
        case UI_EV_DOWN:
            sel = sel + 1 < g->peek_n ? sel + 1 : 0;
            break;
        case UI_EV_PICK:
            pick = e.value >= 0 && e.value < g->peek_n ? e.value : sel;
            break;
        case UI_EV_COWRITE:
            g->cowrite = 0;
            pick = -1;
            break;
        case UI_EV_STOP:
            g->stop_req = 1;
            pick = -1;
            break;
        default:
            continue;
        }
        if (pick == -2) {
            ui_choices(pieces, g->peek_pm, g->peek_n, sel);
            continue;
        }
        if (!g->cowrite || g->stop_req) {
            ui_peek_mode(g->a->settings.peek ? 2 : 1);
            gen_panel(g);
        }
        cpuStartTiming(0);
        return pick >= 0 ? g->peek_ids[pick] : -1;
    }
}

static int peek_step(const llama2_step *s, void *user)
{
    gen_ctx *g = user;
    if (!g->a->settings.peek && !g->cowrite) {
        g->peek_n = 0;
        return -1;
    }
    g->peek_prev = s->prev;
    g->peek_n = s->ilogits ? topk_q16(s->ilogits, s->vocab, 5, g->peek_ids, g->peek_pm)
                           : topk_f32(s->logits, s->vocab, 5, g->peek_ids, g->peek_pm);
    return g->cowrite && g->peek_n ? choose_token(g) : -1;
}

static void peek_texts(gen_ctx *g, const char **pieces)
{
    static char text[5][16];
    for (int i = 0; i < g->peek_n; i++) {
        const char *t = g->peek_ids[i] == LLAMA2_BOS ? "(the end)"
                                                     : llama2_token_text(&g->a->model, g->peek_prev, g->peek_ids[i]);
        size_t k = 0;
        text[i][k++] = '"';
        for (const char *c = t; *c && k < sizeof text[i] - 2; c++)
            text[i][k++] = *c == '\n' ? '/' : *c;
        text[i][k++] = '"';
        text[i][k] = '\0';
        pieces[i] = text[i];
    }
}

static void show_peek(gen_ctx *g, int token)
{
    const char *pieces[5];
    int chosen = -1;
    peek_texts(g, pieces);
    for (int i = 0; i < g->peek_n; i++)
        if (g->peek_ids[i] == token)
            chosen = i;
    ui_peek(pieces, g->peek_pm, g->peek_n, chosen);
}

static void gen_panel(gen_ctx *g)
{
    unsigned ms = g->produced ? ticks_to_ms(g->ticks) / (unsigned)g->produced : 0;
    ui_panel_generating(g->produced, g->steps, ms);
}

static int on_token(int token, const char *piece, void *user);
static void save_history(app *a);
static int save_settings(const dslm_settings *s);

/* chat tokens arrive as raw bytes; the story view wants a C string */
static int on_chat_token(int token, const uint8_t *piece, int len, void *user)
{
    char text[64];
    if (len > (int)sizeof text - 1)
        len = (int)sizeof text - 1;
    memcpy(text, piece, (size_t)len);
    text[len] = '\0';
    return on_token(token, text, user);
}

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
    if (g->a->kind == KIND_STORY && g->a->settings.peek && g->peek_n)
        show_peek(g, token);
    ui_event e = g->cowrite ? (ui_event){UI_EV_NONE, 0} : ui_poll(UI_GENERATING);
    if (e.type == UI_EV_PEEK) { /* only offered for story models */
        g->a->settings.peek = !g->a->settings.peek;
        g->peek_n = 0;
        ui_peek_mode(g->a->settings.peek ? 2 : 1);
        gen_panel(g);
    } else if (e.type == UI_EV_COWRITE) { /* the next step asks the user */
        g->cowrite = 1;
        ui_peek_mode(3);
        gen_panel(g);
    }
    g->stopped = e.type == UI_EV_STOP || g->stop_req;
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
                       DSLM_VERSION,
                       a->kind == KIND_CHAT ? "chat-int8" : a->model.format.quantized ? "int8" : "fp32",
                       a->ckpt_path, a->prompt, steps, a->stopped, a->load_ms, a->n_tokens,
                       a->gen_ms, tps100 / 100, tps100 % 100, heap_peak);
    if (len > 0 && (size_t)len < sizeof buf)
        sd_append_file(BENCH_PATH, buf, (size_t)len);
}

static void write_tokens(app *a, int steps)
{
    size_t cap = 256 + DSLM_PATH_MAX + strlen(a->run_prompt) + (size_t)a->n_tokens * 6;
    char *buf = malloc(cap);
    if (!buf)
        return;
    int len = snprintf(buf, cap, "checkpoint=%s\nprompt=%s\nsteps=%d\nprompt_tokens=%d\ntokens=",
                       a->ckpt_path, a->run_prompt, steps, a->n_prompt_tokens);
    for (int i = 0; i < a->n_tokens; i++)
        len += snprintf(buf + len, cap - len, i ? " %d" : "%d", a->tokens[i]);
    len += snprintf(buf + len, cap - len, "\nstopped=%d\nload_ms=%u\ngen_ms=%u\n", a->stopped,
                    a->load_ms, a->gen_ms);
    sd_write_file(TOKENS_PATH, buf, (size_t)len);
    free(buf);
}

static void generate(app *a)
{
    int seq_len = a->seq_len;
    int steps = a->settings.steps > seq_len ? seq_len : a->settings.steps;
    gen_ctx g = {a, 0, steps, 0, 0, 0, 0, {0}, {0}, 0, 0};
    int peek_before = a->settings.peek;
    ui_peek_mode(a->kind == KIND_STORY ? (a->settings.peek ? 2 : 1) : 0);

    a->story_len = 0;
    a->story[0] = '\0';
    a->chat_q = a->kind == KIND_CHAT ? (int)strlen(a->prompt) : -1;
    ui_set_chat(a->chat_q);
    ui_title(a->model_name, "generating");
    float user_temp = a->settings.temperature;
    if (a->voice_milli > 0) { /* this run only: the saved settings keep theirs */
        char hint[40];
        a->settings.temperature = (float)a->voice_milli / 1000.0f;
        snprintf(hint, sizeof hint, "B:stop  voice:temp %d.%d", a->voice_milli / 1000, a->voice_milli % 1000 / 100);
        ui_hint(hint);
    } else {
        ui_hint("B:stop");
    }
    ui_text("", 0, 0);
    ui_panel_generating(0, steps, 0);
    ui_generating(1);

    if (a->kind == KIND_CHAT) {
        /* steps counts the answer's tokens; the question's bubble is shown above it */
        if (mlq_chat_prompt(a->run_prompt, sizeof a->run_prompt, a->prompt) < 0)
            a->run_prompt[0] = '\0';
        tv_append(a->story, &a->story_len, STORY_CAP, a->prompt);
        tv_append(a->story, &a->story_len, STORY_CAP, "\n\n");
        ui_text(a->story, a->story_len, -1);
        a->sampling.inv_temp =
            a->settings.temperature > 0.0f ? fxk_to_fixed(1.0f / a->settings.temperature, 16) : 0;
        a->sampling.topp = fxk_to_fixed(a->settings.topp, 16);
        cpuStartTiming(0);
        a->n_tokens = mlq_generate(&a->mlq, &a->bpe, a->run_prompt, steps, &a->sampling, a->tokens, seq_len,
                                   &a->n_prompt_tokens, on_chat_token, &g);
        if (a->n_tokens < 0) {
            a->n_tokens = a->n_prompt_tokens = 0;
            tv_append(a->story, &a->story_len, STORY_CAP, "(the question does not fit the context)");
        }
    } else {
        snprintf(a->run_prompt, sizeof a->run_prompt, "%s", a->prompt);
        a->model.temperature = a->settings.temperature;
        a->model.topp = a->settings.topp;
        a->model.on_step = peek_step;
        a->model.step_user = &g;
        cpuStartTiming(0);
        a->n_tokens = llama2_generate(&a->model, a->prompt, steps, a->tokens, seq_len + 1,
                                      &a->n_prompt_tokens, on_token, &g);
    }
    g.ticks += cpuGetTiming();
    a->gen_ms = ticks_to_ms(g.ticks);
    a->stopped = g.stopped;
    ui_generating(0);
    a->settings.temperature = user_temp;
    a->voice_milli = 0;
    a->gen_tokens = a->n_tokens;
    a->model.on_step = NULL; /* g lives on this stack frame */
    if (a->settings.peek != peek_before)
        save_settings(&a->settings);
    write_tokens(a, steps);
    append_bench(a, steps);
    if (!a->benchmark)
        save_history(a);
}

/* ---- screens ------------------------------------------------------------ */

static const char welcome[] =
    "Welcome to DSLM " DSLM_VERSION ", a tiny Llama 2 running offline on your Nintendo DS.\n\n"
    "Type the start of a story with the stylus, or move on the keyboard with the D-pad and press A. "
    "L/R pick an example. START or GO writes the rest.\n\n"
    "The model only knows simple children's stories in English.";

static const char welcome_chat[] =
    "Welcome to DSLM " DSLM_VERSION ". This is LittleKedi, a 2M-parameter chat model running offline on your "
    "Nintendo DS.\n\n"
    "Type a question with the stylus, or with the D-pad and A. L/R pick an example. START or GO asks it.\n\n"
    "It is tiny: its answers are often wrong or off-topic. Each question starts a new chat.";

#define DEFAULT_STORY_MODEL "/dslm/models/stories260K_q80.bin"
#define DEFAULT_CHAT_MODEL "/dslm/models/littlekedi_q80.bin"

static void show_welcome(const app *a)
{
    static char text[sizeof welcome_chat + 32];
    const char *name = ui_user_name();
    const char *body = a->kind == KIND_CHAT ? welcome_chat : welcome;
    int n = name[0] ? snprintf(text, sizeof text, "Hi, %s! %s", name, body) : snprintf(text, sizeof text, "%s", body);
    ui_set_chat(-1);
    ui_text(text, (size_t)n < sizeof text ? (size_t)n : sizeof text - 1, 0);
}

/* "Lily wanted to" becomes "<name> wanted to" when the DS knows the user's name. */
static void personal_example(void)
{
    static char line[32];
    const char *name = ui_user_name();
    if (name[0]) {
        snprintf(line, sizeof line, "%s wanted to", name);
        examples[KIND_STORY][2] = line;
    }
}

/* The last run's text, in bubbles when it was a chat. */
static int show_story(app *a, int first_line)
{
    ui_set_chat(a->kind == KIND_CHAT ? a->chat_q : -1);
    return ui_text(a->story, a->story_len, first_line);
}

static void enter_prompt(app *a, int show_last)
{
    ui_title(a->model_name, "ready");
    ui_hint(a->mic ? "blow:go  START:go  X:mic off" : "START:go  A:key  B:del  Y:space  X:mic");
    if (show_last && a->story_len)
        show_story(a, a->scroll);
    else
        show_welcome(a);
    ui_panel_prompt(a->prompt);
}

/* ---- book mode ---------------------------------------------------------- */

/* Read `text` as a book until B or START; the caller redraws its screen. */
static void book_screen(const char *text, size_t len, const char *title)
{
    ui_fade_out();
    int spreads = ui_book_open(text, len, title), spread = 0;
    ui_book_show(spread);
    ui_fade_in();
    for (;;) {
        ui_frame();
        ui_event e = ui_poll(UI_BOOK);
        int was = spread;
        if (e.type == UI_EV_PAGE_DOWN && spread + 1 < spreads)
            spread++;
        else if (e.type == UI_EV_PAGE_UP && spread > 0)
            spread--;
        else if (e.type == UI_EV_BACK)
            break;
        if (spread != was)
            ui_book_show(spread);
    }
    ui_fade_out();
    ui_book_close();
}

/* ---- choose-your-path ---------------------------------------------------- */

#define PATHS 3
#define PATH_TOKENS 24

static int can_branch(const app *a)
{
    return a->kind == KIND_STORY && a->n_tokens > 1 && a->n_tokens + PATH_TOKENS < a->seq_len;
}

static void enter_story(app *a);

/*
 * Three continuations from the end of the story (KV cache valid up to
 * n_tokens - 2), the user picks one, repeat. Paths are written one after the
 * other over the same cache positions, so a pick other than the last one is
 * fed again before the next round (llama2_feed).
 */
static void paths_screen(app *a)
{
    static int path_tok[PATHS][PATH_TOKENS];
    static char path_text[PATHS][PATH_TOKENS * 16 + 1];
    static char shown[PATHS][PATH_TOKENS * 16 + 4];
    const char *texts[PATHS];
    int path_len[PATHS], added = 0;
    float saved_t = a->model.temperature;
    llama2 *m = &a->model;

    while (can_branch(a)) {
        int base = a->n_tokens, cancelled = 0;
        ui_title(a->model_name, "paths");
        ui_hint("Up/Down:pick  A:choose  B:finish");
        ui_set_chat(-1);
        ui_text(a->story, a->story_len, -1);
        /* paths need some randomness to differ */
        m->temperature = a->settings.temperature < 0.8f ? 0.8f : a->settings.temperature;
        m->topp = a->settings.topp;
        ui_generating(1);
        for (int b = 0; b < PATHS && !cancelled; b++) {
            char status[40];
            snprintf(status, sizeof status, "Writing path %d of %d...", b + 1, PATHS);
            ui_paths(NULL, 0, 0, status);
            int tok = a->tokens[base - 1], pos = base - 1, len = 0;
            size_t tl = 0;
            path_text[b][0] = '\0';
            while (len < PATH_TOKENS) {
                int next = llama2_next(m, tok, pos++);
                if (next == LLAMA2_BOS)
                    break;
                path_tok[b][len++] = next;
                tv_append(path_text[b], &tl, sizeof path_text[b], llama2_token_text(m, tok, next));
                tok = next;
                if (ui_poll(UI_PATHS).type == UI_EV_BACK) {
                    cancelled = 1;
                    break;
                }
            }
            path_len[b] = len;
            /* a path that goes on inside the last word reads better after "..." */
            if (path_text[b][0] == ' ' || path_text[b][0] == '\n' || !path_text[b][0])
                snprintf(shown[b], sizeof shown[b], "%.384s", path_text[b][0] ? path_text[b] + 1 : "");
            else
                snprintf(shown[b], sizeof shown[b], "...%.384s", path_text[b]);
            texts[b] = shown[b];
        }
        ui_generating(0);
        m->temperature = saved_t;
        if (cancelled)
            break;
        ui_sound(SFX_DONE);

        int sel = 0, pick = -1;
        ui_paths(texts, PATHS, sel, NULL);
        while (pick < 0) {
            ui_frame();
            ui_event e = ui_poll(UI_PATHS);
            if (e.type == UI_EV_UP)
                sel = (sel + PATHS - 1) % PATHS;
            else if (e.type == UI_EV_DOWN)
                sel = (sel + 1) % PATHS;
            else if (e.type == UI_EV_PICK)
                pick = e.value >= 0 ? e.value : sel;
            else if (e.type == UI_EV_BACK)
                break;
            else
                continue;
            if (pick < 0)
                ui_paths(texts, PATHS, sel, NULL);
        }
        if (pick < 0 || path_len[pick] == 0)
            break; /* finished, or the chosen path ends the story */
        if (pick != PATHS - 1) { /* the cache holds the last path: feed the chosen one */
            llama2_feed(m, a->tokens[base - 1], base - 1);
            for (int i = 0; i + 1 < path_len[pick]; i++)
                llama2_feed(m, path_tok[pick][i], base + i);
        }
        for (int i = 0; i < path_len[pick]; i++)
            a->tokens[a->n_tokens++] = path_tok[pick][i];
        tv_append(a->story, &a->story_len, STORY_CAP, path_text[pick]);
        added = 1;
    }
    if (added)
        save_history(a);
}

static void enter_story(app *a)
{
    ui_title(a->model_name, a->stopped ? "stopped" : "done");
    ui_hint("Up/Down:scroll  A:new  Y:again  SEL:menu");
    int total = show_story(a, -1);
    a->scroll = total > UI_TEXT_ROWS ? total - UI_TEXT_ROWS : 0;
    if (!a->stopped) /* a stop already played its own sound */
        ui_sound(SFX_DONE);
    ui_panel_story(a->n_tokens, a->gen_tokens, a->gen_ms, a->stopped, can_branch(a));
}

static void scroll_story(app *a, int delta)
{
    ui_set_chat(a->kind == KIND_CHAT ? a->chat_q : -1);
    int total = ui_text_lines(a->story, a->story_len);
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
    case UI_SETTINGS_SOUND_ROW:
        s->sound = !s->sound;
        ui_sound_enable(s->sound); /* hear the choice before saving it */
        break;
    case UI_SETTINGS_ACCENT_ROW:
        s->accent = !s->accent;
        ui_accent(s->accent); /* see the choice before saving it */
        break;
    }
}

static const char *model_note(const model_entry *m)
{
    static char note[40];
    const char *slash = strrchr(m->path, '/');
    snprintf(note, sizeof note, "%.26s %s", slash ? slash + 1 : m->path, m->kind);
    return note;
}

static int save_settings(const dslm_settings *s)
{
    char buf[768];
    size_t n = dslm_ini_format(s, buf, sizeof buf);
    return n && sd_write_file(INI_PATH, buf, n) == SD_OK;
}

/*
 * The settings say which mode and checkpoints to use, but an old dslm.ini may
 * name a chat model as `checkpoint` (or a story model as `chat_checkpoint`):
 * the mode follows the kind that actually loaded.
 */
static void settle_mode(app *a)
{
    dslm_settings *s = &a->settings;
    if (a->kind == KIND_CHAT && s->mode != DSLM_MODE_CHAT) {
        strcpy(s->chat_checkpoint, a->ckpt_path);
        strcpy(s->checkpoint, DEFAULT_STORY_MODEL);
        s->mode = DSLM_MODE_CHAT;
    } else if (a->kind == KIND_STORY && s->mode != DSLM_MODE_STORIES) {
        strcpy(s->checkpoint, a->ckpt_path);
        strcpy(s->chat_checkpoint, DEFAULT_CHAT_MODEL);
        s->mode = DSLM_MODE_STORIES;
    }
}

/* Load edit's mode and checkpoint; on failure reload the previous one. Returns
 * NULL when loaded, else why not (the previous model is back). */
static const char *switch_model(app *a, const dslm_settings *edit)
{
    static char why[64];
    dslm_settings previous = a->settings;
    a->settings = *edit;
    unload_model(a);
    a->story_len = 0;
    a->story[0] = '\0';
    const char *err = load_model(a);
    if (err) {
        a->settings = previous;
        const char *again = load_model(a);
        if (again)
            ui_fatal("%s", again);
        /* the reason is longer than a note line: it goes on the top screen */
        snprintf(a->story, STORY_CAP, "Could not load the model:\n\n%s\n\nThe previous model is loaded again.", err);
        a->story_len = strlen(a->story);
        a->chat_q = -1;
        snprintf(why, sizeof why, "Not loaded, kept %.30s", a->model_name);
        return why;
    }
    settle_mode(a);
    save_settings(&a->settings);
    return NULL;
}

/* Home card -> the mode it needs loaded. Returns NULL when ready. */
static const char *enter_mode(app *a, dslm_mode mode)
{
    model_kind want = mode == DSLM_MODE_CHAT ? KIND_CHAT : KIND_STORY;
    if (a->kind == want)
        return NULL;
    dslm_settings edit = a->settings;
    edit.mode = mode;
    model_kind before = a->kind;
    const char *why = switch_model(a, &edit);
    if (a->kind != before) { /* story starters and questions do not mix */
        a->example = 0;
        strcpy(a->prompt, examples[a->kind][0]);
    }
    if (!why && a->kind != want) /* the file named for this mode is the other kind */
        return mode == DSLM_MODE_CHAT ? "That is not a chat model" : "That is not a story model";
    return why;
}

/* Settings, opened from Home. The Model row lists the story models; saving
 * a new one loads it now in Stories mode, or on the next Stories visit. */
static const char *settings_screen(app *a)
{
    static char done[48];
    dslm_settings edit = a->settings;
    model_entry all[MAX_MODELS], models[MAX_MODELS];
    int n_all = scan_models(all, MAX_MODELS), n_models = 0;
    for (int i = 0; i < n_all; i++)
        if (strcmp(all[i].kind, "chat") != 0)
            models[n_models++] = all[i];
    int current = -1;
    for (int i = 0; i < n_models; i++)
        if (strcmp(models[i].path, edit.checkpoint) == 0)
            current = i;
    int cursor = 0;
    const char *note = n_models > 1 ? NULL : "Only one story model in /dslm/models";
    ui_fade_out();
    ui_title(a->model_name, "settings");
    ui_hint("START:save  B:back");
    if (a->story_len)
        show_story(a, 0);
    else
        show_welcome(a);
    ui_panel_settings(&edit, cursor, note);
    ui_fade_in();
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
                adjust(&edit, row, e.type == UI_EV_RIGHT ? 1 : -1, a->seq_len);
            }
            break;
        case UI_EV_SAVE:
            if (a->kind == KIND_STORY && strcmp(edit.checkpoint, a->settings.checkpoint) != 0) {
                const char *why = switch_model(a, &edit);
                edit = a->settings;
                note = why ? why : "Loaded and saved";
                ui_title(a->model_name, "settings");
                ui_hint("START:save  B:back");
                if (a->story_len)
                    show_story(a, 0);
                else
                    show_welcome(a);
            } else {
                a->settings = edit;
                note = save_settings(&edit) ? "Saved to /dslm/dslm.ini" : "Applied (could not save)";
            }
            break;
        case UI_EV_BACK:
            ui_sound_enable(a->settings.sound); /* unsaved switches do not stick */
            ui_accent(a->settings.accent);
            snprintf(done, sizeof done, "%s", note && strncmp(note, "Not loaded", 10) == 0 ? note : "");
            return done[0] ? done : NULL;
        default:
            continue;
        }
        ui_panel_settings(&edit, cursor, note);
    }
}

static void about_screen(app *a)
{
    char model[64], kind[48], ram[48];
    snprintf(model, sizeof model, "Model: %s", a->model_name);
    snprintf(kind, sizeof kind, "Kind: %s, context %d tokens",
             a->kind == KIND_CHAT ? "chat (MLQ1 int8)" : a->model.format.quantized ? "story (int8)" : "story (fp32)",
             a->seq_len);
    snprintf(ram, sizeof ram, "RAM free: %u KB", (unsigned)(heap_available() / 1024));
    const char *lines[] = {
        "Tiny language models, 100% offline.",
        "Runs on the Nintendo DS from an R4 card.",
        "",
        model,
        kind,
        ram,
        "",
        "Engine after llama2.c (MIT); BlocksDS.",
        "Models: see THIRD_PARTY.txt on the card.",
    };
    ui_fade_out();
    ui_about(lines, (int)(sizeof lines / sizeof lines[0]));
    ui_fade_in();
    for (;;) {
        ui_frame();
        if (ui_poll(UI_ABOUT).type == UI_EV_BACK)
            return;
    }
}

/* ---- history ------------------------------------------------------------ */

#define HISTORY_MAX 64        /* entries listed, newest first */
#define HISTORY_FILE_MAX 16384

/* Up to `max` history numbers, newest first. */
static int scan_history(int *nums, int max)
{
    DIR *dir = opendir(HISTORY_DIR);
    int n = 0;
    if (!dir)
        return 0;
    for (struct dirent *e; (e = readdir(dir)) != NULL;) {
        int num = history_number(e->d_name);
        if (num < 0)
            continue;
        int at = n;
        while (at > 0 && nums[at - 1] < num) /* insertion, descending */
            at--;
        if (at >= max)
            continue;
        if (n < max)
            n++;
        memmove(nums + at + 1, nums + at, (size_t)(n - 1 - at) * sizeof nums[0]);
        nums[at] = num;
    }
    closedir(dir);
    return n;
}

static void history_path(int num, char *out, size_t cap)
{
    char name[16];
    history_name(num, name);
    snprintf(out, cap, "%s/%s", HISTORY_DIR, name);
}

/* The finished run, as a new file numbered after the newest one. */
static void save_history(app *a)
{
    if (!a->story_len || !a->n_tokens)
        return;
    int newest = 0;
    mkdir("/dslm", 0777);
    mkdir(HISTORY_DIR, 0777);
    scan_history(&newest, 1);
    history_entry e;
    memset(&e, 0, sizeof e);
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm && tm->tm_year >= 100)
        strftime(e.date, sizeof e.date, "%Y-%m-%d %H:%M", tm);
    snprintf(e.model, sizeof e.model, "%s", a->model_name);
    snprintf(e.prompt, sizeof e.prompt, "%s", a->prompt);
    e.chat = a->kind == KIND_CHAT;
    e.question = e.chat ? a->chat_q : 0;
    e.tokens = a->n_tokens;
    e.stopped = a->stopped;
    e.text = a->story;
    e.text_len = a->story_len;
    size_t cap = a->story_len + 512;
    char *buf = malloc(cap);
    if (!buf)
        return;
    size_t n = history_format(&e, buf, cap);
    char path[64];
    history_path(newest + 1, path, sizeof path);
    if (n)
        sd_write_file(path, buf, n);
    free(buf);
}

typedef struct {
    int nums[HISTORY_MAX];
    char left[HISTORY_MAX][12];
    char right[HISTORY_MAX][HISTORY_PROMPT_MAX + 1];
    ui_list_item items[HISTORY_MAX];
    int n;
} history_list;

static void load_history_list(history_list *l)
{
    char head[512];
    l->n = scan_history(l->nums, HISTORY_MAX);
    for (int i = 0; i < l->n; i++) {
        char path[64];
        size_t got = 0;
        history_entry e;
        history_path(l->nums[i], path, sizeof path);
        if (sd_read_upto(path, head, sizeof head, &got) != SD_OK || history_parse(head, got, &e) != 0)
            memset(&e, 0, sizeof e);
        /* "2026-11-26 20:15" -> "11-26 20:15" */
        snprintf(l->left[i], sizeof l->left[i], "%.11s", strlen(e.date) > 5 ? e.date + 5 : "-----");
        snprintf(l->right[i], sizeof l->right[i], "%s", e.prompt[0] ? e.prompt : "(no prompt)");
        l->items[i].left = l->left[i];
        l->items[i].right = l->right[i];
        l->items[i].accent = e.chat;
    }
}

static void history_screen(app *a)
{
    /* allocated only while the screen is open: static RAM would shrink the chat context */
    history_list *lp = malloc(sizeof *lp);
    if (!lp)
        return;
#define list (*lp)
    void *file = NULL;
    history_entry e;
    int sel = 0, scroll = 0, confirm = 0, loaded = -1, refade = 0;
    (void)a;
    load_history_list(&list);
    ui_fade_out();
    for (int fade = 1;; fade = 0) {
        if (loaded != sel) { /* show the selected entry on the top screen */
            free(file);
            file = NULL;
            memset(&e, 0, sizeof e);
            size_t size = 0;
            char path[64];
            if (list.n) {
                history_path(list.nums[sel], path, sizeof path);
                if (sd_read_file(path, HISTORY_FILE_MAX, &file, &size) != SD_OK ||
                    history_parse(file, size, &e) != 0)
                    memset(&e, 0, sizeof e);
            }
            loaded = sel;
            scroll = 0;
            ui_title(e.model[0] ? e.model : "", "history");
            ui_hint(list.n ? "Up/Down:pick  A:book  L/R:scroll  Y:del  B:back" : "B:back");
        }
        if (list.n && e.text) {
            ui_set_chat(e.chat ? e.question : -1);
            ui_text(e.text, e.text_len, scroll);
        } else {
            static const char none[] = "Nothing saved yet.\n\nEvery finished story or answer is saved to "
                                       "/dslm/history on the microSD card and listed here, newest first.";
            ui_set_chat(-1);
            ui_text(none, sizeof none - 1, 0);
        }
        ui_history(list.items, list.n, sel, confirm);
        if (fade || refade) {
            ui_fade_in();
            refade = 0;
        }
        for (int redraw = 0; !redraw;) {
            ui_frame();
            ui_event ev = ui_poll(UI_HISTORY);
            if (ev.type == UI_EV_NONE)
                continue;
            int was_confirm = confirm;
            confirm = 0;
            redraw = 1;
            switch (ev.type) {
            case UI_EV_UP:
                sel = sel > 0 ? sel - 1 : 0;
                break;
            case UI_EV_DOWN:
                sel = sel + 1 < list.n ? sel + 1 : sel;
                break;
            case UI_EV_SELECT_ROW:
                sel = ev.value;
                break;
            case UI_EV_PAGE_UP:
            case UI_EV_PAGE_DOWN:
                if (e.text) {
                    int max = ui_text_lines(e.text, e.text_len) - UI_TEXT_ROWS;
                    scroll += (ev.type == UI_EV_PAGE_DOWN ? 1 : -1) * (UI_TEXT_ROWS - 2);
                    scroll = scroll > max ? max : scroll;
                    scroll = scroll < 0 ? 0 : scroll;
                }
                break;
            case UI_EV_DELETE:
                if (!list.n)
                    break;
                if (!was_confirm) {
                    confirm = 1;
                    break;
                }
                {
                    char path[64];
                    history_path(list.nums[sel], path, sizeof path);
                    remove(path);
                }
                load_history_list(&list);
                sel = sel < list.n ? sel : (list.n ? list.n - 1 : 0);
                loaded = -1;
                break;
            case UI_EV_BOOK:
                if (e.text) {
                    book_screen(e.text, e.text_len, e.date[0] ? e.date : e.model);
                    loaded = -1; /* redraw the preview */
                    refade = 1;
                }
                break;
            case UI_EV_BACK:
                free(file);
                free(lp);
                return;
            default:
                redraw = 0;
                break;
            }
        }
    }
}

#undef list

/* ---- benchmark ---------------------------------------------------------- */

#define BENCH_STEPS 128

/* The standard run on the loaded model: first example, temperature 0, 128
 * steps. The user's settings, prompt and last story come back afterwards. */
static void bench_screen(app *a)
{
    dslm_settings saved = a->settings;
    char prompt[sizeof a->prompt];
    strcpy(prompt, a->prompt);
    for (int again = 1; again;) {
        a->settings.temperature = 0.0f;
        a->settings.steps = BENCH_STEPS;
        strcpy(a->prompt, examples[a->kind][0]);
        a->benchmark = 1;
        generate(a);
        a->benchmark = 0;
        ui_title(a->model_name, a->stopped ? "stopped" : "done");
        ui_hint("A:run again  B:back");

        int positions = a->n_tokens > 1 ? a->n_tokens - 1 : 0;
        unsigned tps100 = a->gen_ms ? (unsigned)((uint64_t)positions * 100000 / a->gen_ms) : 0;
        unsigned ms_each = positions ? a->gen_ms / (unsigned)positions : 0;
        char l_model[64], l_prompt[64], l_tokens[48], l_speed[48], l_load[64];
        snprintf(l_model, sizeof l_model, "%s (%s)", a->model_name,
                 a->kind == KIND_CHAT ? "chat int8" : a->model.format.quantized ? "int8" : "fp32");
        snprintf(l_prompt, sizeof l_prompt, "Prompt: %.40s", a->prompt);
        snprintf(l_tokens, sizeof l_tokens, "%d tokens in %u.%u s%s", positions, a->gen_ms / 1000,
                 (a->gen_ms % 1000) / 100, a->stopped ? " (stopped)" : "");
        snprintf(l_speed, sizeof l_speed, "Speed: %u.%02u tokens/s (%u ms each)", tps100 / 100, tps100 % 100, ms_each);
        snprintf(l_load, sizeof l_load, "Load: %u ms   Heap peak: %u KB", a->load_ms,
                 (unsigned)((getHeapEnd() - getHeapStart()) / 1024));
        const char *lines[] = {l_model, l_prompt, l_tokens, l_speed, l_load, "",
                               "Saved to /dslm/bench.txt.",
                               "Real-DS results are welcome: see",
                               "tests/hw/runbook.md in the project."};
        ui_report("Benchmark", lines, (int)(sizeof lines / sizeof lines[0]));
        for (int waiting = 1; waiting;) {
            ui_frame();
            ui_event e = ui_poll(UI_REPORT);
            if (e.type == UI_EV_AGAIN)
                waiting = 0;
            else if (e.type == UI_EV_BACK)
                waiting = again = 0;
        }
    }
    /* the benchmark text is not the user's story */
    a->settings = saved;
    strcpy(a->prompt, prompt);
    a->story_len = 0;
    a->story[0] = '\0';
}

/* Prompt -> Generating -> Story, until the user asks for the menu. */
static void prompt_screen(app *a)
{
    ui_fade_out();
    enter_prompt(a, 1);
    ui_fade_in();
    for (;;) {
        ui_frame();
        ui_event e = ui_poll(UI_PROMPT);
        size_t len = strlen(a->prompt);
        if (e.type == UI_EV_BLOW) { /* a blow starts it, and its strength sets the temperature */
            a->voice_milli = mic_temperature_milli(e.value);
            e.type = UI_EV_GO;
        }
        if (e.type == UI_EV_GO || e.type == UI_EV_MENU) { /* the microphone only listens on this screen */
            ui_mic(0);
            a->mic = 0;
        }
        switch (e.type) {
        case UI_EV_MIC:
            a->mic = !a->mic;
            ui_mic(a->mic);
            enter_prompt(a, 1);
            break;
        case UI_EV_CHAR:
            if (len < UI_PROMPT_MAX) {
                a->prompt[len] = (char)e.value;
                a->prompt[len + 1] = '\0';
                ui_panel_prompt(a->prompt);
            }
            break;
        case UI_EV_BACKSPACE:
            if (len > 0) {
                a->prompt[len - 1] = '\0';
                ui_panel_prompt(a->prompt);
            }
            break;
        case UI_EV_EXAMPLE_PREV:
        case UI_EV_EXAMPLE_NEXT:
            a->example = (a->example + (e.type == UI_EV_EXAMPLE_NEXT ? 1 : N_EXAMPLES - 1)) % N_EXAMPLES;
            strcpy(a->prompt, examples[a->kind][a->example]);
            ui_panel_prompt(a->prompt);
            break;
        case UI_EV_MENU:
            return;
        case UI_EV_GO:
            /* Story loop: generate, then New / Again / Menu. */
            for (int again = 1; again;) {
                generate(a);
                enter_story(a);
                again = 0;
                for (int waiting = 1; waiting;) {
                    ui_frame();
                    ui_event s = ui_poll(UI_STORY);
                    switch (s.type) {
                    case UI_EV_UP:
                    case UI_EV_DOWN:
                        scroll_story(a, s.type == UI_EV_DOWN ? 1 : -1);
                        break;
                    case UI_EV_AGAIN:
                        again = 1;
                        waiting = 0;
                        break;
                    case UI_EV_PATHS:
                        paths_screen(a);
                        enter_story(a);
                        break;
                    case UI_EV_BOOK:
                        book_screen(a->story, a->story_len, a->model_name);
                        enter_story(a);
                        ui_fade_in();
                        break;
                    case UI_EV_MENU:
                        return;
                    case UI_EV_NEW:
                        waiting = 0;
                        break;
                    default:
                        break;
                    }
                }
            }
            enter_prompt(a, 1);
            break;
        default:
            break;
        }
    }
}

static void show_home(app *a, int focus, const char *note)
{
    char stories[DSLM_PATH_MAX], chat[DSLM_PATH_MAX];
    snprintf(stories, sizeof stories, "%s", model_label(a->settings.checkpoint));
    snprintf(chat, sizeof chat, "%s", model_label(a->settings.chat_checkpoint));
    ui_fade_out();
    ui_home(focus, stories, chat, note);
    if (note)
        ui_sound(SFX_ERROR);
    ui_fade_in();
    a->model_name = model_label(a->ckpt_path); /* model_label's buffer was reused */
}

int main(void)
{
    static app a;

    ui_init();
    boot(&a);
    ui_sound_enable(a.settings.sound);
    ui_accent(a.settings.accent);
    personal_example();
    settle_mode(&a);
    a.chat_q = -1;
    strcpy(a.prompt, examples[a.kind][0]);
    a.example = 0;
    a.scroll = 0;

    int focus = a.kind == KIND_CHAT ? UI_CARD_CHAT : UI_CARD_STORIES;
    show_home(&a, focus, NULL);
    ui_sound(SFX_BOOT);
    for (;;) {
        ui_frame();
        ui_event e = ui_poll(UI_HOME);
        if (e.type != UI_EV_OPEN)
            continue;
        const char *note = NULL;
        focus = e.value;
        switch (e.value) {
        case UI_CARD_STORIES:
        case UI_CARD_CHAT:
            note = enter_mode(&a, e.value == UI_CARD_CHAT ? DSLM_MODE_CHAT : DSLM_MODE_STORIES);
            if (!note)
                prompt_screen(&a);
            break;
        case UI_CARD_HISTORY:
            history_screen(&a);
            break;
        case UI_CARD_SETTINGS:
            note = settings_screen(&a);
            break;
        case UI_CARD_BENCH:
            bench_screen(&a);
            break;
        case UI_CARD_ABOUT:
            about_screen(&a);
            break;
        }
        show_home(&a, focus, note);
    }
}
