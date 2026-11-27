/*
 * Host driver for src/engine/llama2.c. Prints the same record as
 * tools/golden/golden.c so tools/tests/test_engine_host.py can compare them.
 */
#include <stdio.h>
#include <stdlib.h>

#include "llama2.h"
#include "topk.h"

static void *slurp(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "cannot read %s\n", path);
        exit(1);
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

typedef struct {
    FILE *text;
    int seen;
    int stop_after; /* 0 = never stop early */
} run_ctx;

static int on_token(int token, const char *piece, void *user)
{
    run_ctx *ctx = user;
    (void)token;
    for (const unsigned char *p = (const unsigned char *)piece; *p; p++)
        fprintf(ctx->text, "%02x", *p);
    ctx->seen++;
    return ctx->stop_after > 0 && ctx->seen >= ctx->stop_after;
}

/* DSLM_HOOK_TOP1=1: the step hook forces the most likely token (topk), which
 * must reproduce the temperature-0 output whatever the temperature. */
static int force_top1(const llama2_step *s, void *user)
{
    int id, pm;
    (void)user;
    if (s->ilogits)
        topk_q16(s->ilogits, s->vocab, 1, &id, &pm);
    else
        topk_f32(s->logits, s->vocab, 1, &id, &pm);
    return id;
}

int main(int argc, char **argv)
{
    if (argc < 5 || argc > 9) {
        fprintf(stderr, "usage: engine_main <checkpoint> <tokenizer> <steps> <prompt>"
                        " [stop_after [temperature [topp [seed]]]]\n");
        return 2;
    }
    size_t ckpt_size, tok_size;
    void *ckpt = slurp(argv[1], &ckpt_size);
    void *tok = slurp(argv[2], &tok_size);

    llama2 m;
    float temperature = argc > 6 ? (float)atof(argv[6]) : 0.0f;
    float topp = argc > 7 ? (float)atof(argv[7]) : 0.9f;
    unsigned long long seed = argc > 8 ? strtoull(argv[8], NULL, 10) : 0;
    llama2_status st = llama2_init(&m, ckpt, ckpt_size, tok, tok_size, temperature, topp, seed);
    if (st != LLAMA2_OK) {
        fprintf(stderr, "llama2_init failed: %d\n", (int)st);
        return 1;
    }
    free(tok);
    if (getenv("DSLM_HOOK_TOP1"))
        m.on_step = force_top1;

    int steps = atoi(argv[3]);
    int *out = malloc((size_t)(m.config.seq_len + 1) * sizeof(int));
    int n_prompt = 0;

    /* Text first into a temp file, so the token line can come before it. */
    FILE *text = tmpfile();
    run_ctx ctx = {text, 0, argc > 5 ? atoi(argv[5]) : 0};
    int n = llama2_generate(&m, argv[4], steps, out, m.config.seq_len + 1, &n_prompt, on_token, &ctx);
    if (n < 0) {
        fprintf(stderr, "prompt too long\n");
        return 1;
    }

    /* DSLM_CONTINUE: the run above stopped after stop_after tokens; carry on
     * with llama2_next to `steps` positions. Must equal the uninterrupted run. */
    if (getenv("DSLM_CONTINUE")) {
        int pos = n - 1, token = out[n - 1];
        while (pos < steps && pos < m.config.seq_len) {
            int next = llama2_next(&m, token, pos);
            pos++;
            if (next == 1) /* BOS ends the story, as in llama2_generate */
                break;
            out[n++] = next;
            token = next;
        }
    }

    printf("P %d\nT", n_prompt);
    for (int i = 0; i < n; i++)
        printf(" %d", out[i]);
    printf("\nS ");
    rewind(text);
    for (int c; (c = fgetc(text)) != EOF;)
        putchar(c);
    printf("\n");

    fclose(text);
    free(out);
    llama2_free(&m);
    free(ckpt);
    return 0;
}
