/*
 * DSLM M3 skeleton: load stories260K into RAM, generate from a fixed prompt,
 * stream the text and write the token ids to /dslm/tokens.txt for parity.
 */
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/llama2.h"
#include "storage/ini.h"
#include "storage/sdcard.h"
#include "ui/ui.h"

#define INI_PATH "/dslm/dslm.ini"
#define INI_MAX_SIZE 4096
#define TOKENS_PATH "/dslm/tokens.txt"
#define SKELETON_PROMPT "Once upon a time"

typedef struct {
    int produced;
    uint64_t ticks;
} gen_stats;

static void load_progress(size_t done, size_t total, void *user)
{
    ui_status("loading %s\n%u%% (%u / %u bytes)", (const char *)user,
              (unsigned)(done * 100 / total), (unsigned)done, (unsigned)total);
}

static void on_token(int token, const char *piece, void *user)
{
    gen_stats *g = user;
    (void)token;
    g->ticks += cpuGetTiming();
    cpuStartTiming(0);
    g->produced++;
    ui_stream(piece);
    ui_status("generating...\n%d tokens", g->produced);
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

static void write_tokens(const char *prompt, int steps, int n_prompt, const int *tokens, int n,
                         unsigned load_ms, unsigned gen_ms)
{
    size_t cap = 256 + (size_t)n * 6;
    char *buf = malloc(cap);
    if (!buf) {
        ui_log("no memory to write %s", TOKENS_PATH);
        return;
    }
    int len = snprintf(buf, cap, "prompt=%s\nsteps=%d\nprompt_tokens=%d\ntokens=", prompt, steps, n_prompt);
    for (int i = 0; i < n; i++)
        len += snprintf(buf + len, cap - len, i ? " %d" : "%d", tokens[i]);
    len += snprintf(buf + len, cap - len, "\nload_ms=%u\ngen_ms=%u\n", load_ms, gen_ms);
    sd_status st = sd_write_file(TOKENS_PATH, buf, (size_t)len);
    free(buf);
    if (st != SD_OK)
        ui_log("could not write %s", TOKENS_PATH);
}

int main(void)
{
    dslm_settings s;
    unsigned char header[LLAMA2_HEADER_BYTES];
    size_t ckpt_size, tok_size;
    llama2_config cfg;
    int shared;

    ui_init();
    ui_log("DSLM - Llama 2 on the DS");

    if (!sd_mount())
        ui_fatal("FAT mount failed.\nThe binary may need a manual DLDI patch (dlditool).");
    ui_log("FAT mounted");

    read_settings(&s);

    /* Both files must exist before anything is allocated. */
    sd_status st = sd_read_head(s.checkpoint, header, sizeof header, &ckpt_size);
    if (st != SD_OK)
        ui_fatal("%s: %s", s.checkpoint, sd_status_text(st));
    unsigned char tok_head[4];
    st = sd_read_head(s.tokenizer, tok_head, sizeof tok_head, &tok_size);
    if (st != SD_OK)
        ui_fatal("%s: %s", s.tokenizer, sd_status_text(st));
    ui_log("found %s\nfound %s", s.checkpoint, s.tokenizer);

    if (llama2_read_config(header, &cfg, &shared) != LLAMA2_OK)
        ui_fatal("%s: not a llama2.c checkpoint", s.checkpoint);
    if (llama2_checkpoint_bytes(&cfg, shared) != ckpt_size)
        ui_fatal("%s: %u bytes, header implies %u", s.checkpoint, (unsigned)ckpt_size,
                 (unsigned)llama2_checkpoint_bytes(&cfg, shared));

    /* Memory budget from the header, before any large allocation. */
    size_t need = ckpt_size + 1 + tok_size + 1 + llama2_runtime_bytes(&cfg, tok_size);
    size_t avail = (size_t)(getHeapLimit() - getHeapEnd());
    ui_log("dim %d layers %d heads %d vocab %d seq %d", cfg.dim, cfg.n_layers, cfg.n_heads,
           cfg.vocab_size, cfg.seq_len);
    ui_log("RAM need %u / free %u", (unsigned)need, (unsigned)avail);
    if (need > avail)
        ui_fatal("model does not fit: needs %u bytes, %u available", (unsigned)need, (unsigned)avail);

    void *ckpt, *tok;
    cpuStartTiming(0);
    st = sd_read_file_progress(s.checkpoint, ckpt_size, &ckpt, &ckpt_size, load_progress, "model");
    if (st != SD_OK)
        ui_fatal("%s: %s", s.checkpoint, sd_status_text(st));
    st = sd_read_file_progress(s.tokenizer, tok_size, &tok, &tok_size, load_progress, "tokenizer");
    if (st != SD_OK)
        ui_fatal("%s: %s", s.tokenizer, sd_status_text(st));
    unsigned load_ms = ticks_to_ms(cpuGetTiming());

    llama2 model;
    llama2_status ls = llama2_init(&model, ckpt, ckpt_size, tok, tok_size, s.temperature, s.topp,
                                   s.seed ? s.seed : 1);
    free(tok);
    if (ls != LLAMA2_OK)
        ui_fatal("model init failed (%d)", (int)ls);
    ui_log("loaded in %u ms\n", load_ms);

    int steps = s.steps > cfg.seq_len ? cfg.seq_len : s.steps;
    int *tokens = malloc((size_t)(steps + 1) * sizeof(int));
    if (!tokens)
        ui_fatal("no memory for the token buffer");

    gen_stats g = {0, 0};
    int n_prompt = 0;
    cpuStartTiming(0);
    int n = llama2_generate(&model, SKELETON_PROMPT, steps, tokens, steps + 1, &n_prompt, on_token, &g);
    g.ticks += cpuGetTiming();
    unsigned gen_ms = ticks_to_ms(g.ticks);

    write_tokens(SKELETON_PROMPT, steps, n_prompt, tokens, n, load_ms, gen_ms);
    ui_status("done: %d tokens\n%u ms total\n%u ms per position\nwrote %s", n, gen_ms,
              n > 1 ? gen_ms / (unsigned)(n - 1) : 0, TOKENS_PATH);
    ui_halt();
}
