/*
 * Port of run.c from llama2.c (commit 350e04f), MIT License,
 * Copyright (c) 2023 Andrej Karpathy.
 *
 * The forward pass, tokenizer and sampler keep run.c's arithmetic and
 * operation order line for line, so the DS output can match the PC golden
 * output token for token. What changed: no mmap, no stdio, no exit(); the
 * model comes from buffers in RAM and every buffer is allocated once.
 */
#include "llama2.h"
#include "llama2_fixed.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ---- loading ------------------------------------------------------------ */

llama2_status llama2_read_config(const void *header, llama2_config *c, int *shared_weights)
{
    int32_t f[7];
    memcpy(f, header, sizeof f);
    c->dim = f[0];
    c->hidden_dim = f[1];
    c->n_layers = f[2];
    c->n_heads = f[3];
    c->n_kv_heads = f[4];
    c->vocab_size = f[5] < 0 ? -f[5] : f[5];
    c->seq_len = f[6];
    *shared_weights = f[5] > 0;
    if (c->dim <= 0 || c->hidden_dim <= 0 || c->n_layers <= 0 || c->n_heads <= 0 ||
        c->n_kv_heads <= 0 || c->vocab_size <= 0 || c->seq_len <= 0 ||
        c->dim % c->n_heads != 0 || c->n_heads % c->n_kv_heads != 0)
        return LLAMA2_BAD_HEADER;
    return LLAMA2_OK;
}

size_t llama2_checkpoint_bytes(const llama2_config *c, int shared_weights)
{
    size_t d = c->dim, hd = c->hidden_dim, L = c->n_layers, V = c->vocab_size, S = c->seq_len;
    size_t head = d / c->n_heads;
    size_t kv_dim = d * c->n_kv_heads / c->n_heads;
    size_t n = V * d + L * d + L * d * d + 2 * L * d * kv_dim + L * d * d + L * d +
               3 * L * d * hd + d + S * head;
    if (!shared_weights)
        n += V * d;
    return LLAMA2_HEADER_BYTES + 4 * n;
}

static size_t state_floats(const llama2_config *c)
{
    size_t kv_dim = (size_t)c->dim * c->n_kv_heads / c->n_heads;
    return 4 * (size_t)c->dim + 2 * (size_t)c->hidden_dim +
           2 * (size_t)c->n_layers * c->seq_len * kv_dim +
           (size_t)c->n_heads * c->seq_len + c->vocab_size + c->dim /* RoPE table */;
}

#define ALIGN8(n) (((n) + 7u) & ~(size_t)7u)
#define Q80_MAGIC 0x616B3432u

llama2_status llama2_probe(const void *head, size_t n, llama2_config *c, llama2_format *f)
{
    const unsigned char *h = head;
    uint32_t magic;
    memset(f, 0, sizeof *f);
    if (n < 4)
        return LLAMA2_BAD_HEADER;
    memcpy(&magic, h, 4);
    if (magic != Q80_MAGIC) {
        if (n < LLAMA2_HEADER_BYTES)
            return LLAMA2_BAD_HEADER;
        return llama2_read_config(head, c, &f->shared);
    }

    /* runq.c v2: magic, version, 7 x int32 config, uint8 shared, int32 group size */
    int32_t version, cfg[7], gs;
    if (n < 41)
        return LLAMA2_BAD_HEADER;
    memcpy(&version, h + 4, 4);
    memcpy(cfg, h + 8, sizeof cfg);
    memcpy(&gs, h + 37, 4);
    if (version != 2)
        return LLAMA2_BAD_HEADER;
    f->quantized = 1;
    f->shared = h[36] != 0;
    f->group_size = gs;
    llama2_status st = llama2_read_config(cfg, c, &(int){0});
    if (st != LLAMA2_OK)
        return st;
    if (gs <= 0 || c->dim % gs != 0 || c->hidden_dim % gs != 0)
        return LLAMA2_BAD_HEADER;
    return LLAMA2_OK;
}

size_t llama2_file_bytes(const llama2_config *c, const llama2_format *f)
{
    if (!f->quantized)
        return llama2_checkpoint_bytes(c, f->shared);
    size_t d = c->dim, hd = c->hidden_dim, L = c->n_layers, V = c->vocab_size;
    size_t kv = d * c->n_kv_heads / c->n_heads, gs = f->group_size;
#define QT(nn) ((nn) + 4 * ((nn) / gs))
    size_t n = LLAMA2_Q80_HEADER_BYTES + 4 * (2 * L * d + d) + QT(V * d);
    n += L * (QT(d * d) + 2 * QT(d * kv) + QT(d * d) + 3 * QT(d * hd));
    if (!f->shared)
        n += QT(V * d);
#undef QT
    return n;
}

/* int8 extras: dequantized embedding, k/v rows, activation scales */
static size_t qstate_floats(const llama2_config *c, const llama2_format *f)
{
    size_t kv_dim = (size_t)c->dim * c->n_kv_heads / c->n_heads;
    return (size_t)c->vocab_size * c->dim + 2 * kv_dim +
           c->dim / f->group_size + c->hidden_dim / f->group_size;
}

size_t llama2_runtime_bytes(const llama2_config *c, const llama2_format *f, size_t tokenizer_bytes)
{
    size_t V = c->vocab_size;
    size_t n = ALIGN8(state_floats(c) * sizeof(float)) +
               ALIGN8(V * sizeof(char *)) + ALIGN8(V * sizeof(float)) +
               ALIGN8(tokenizer_bytes) + ALIGN8(V * sizeof(llama2_token_index)) +
               ALIGN8(V * sizeof(llama2_prob_index)) +
               ALIGN8(2 * tokenizer_bytes + 3) +
               ALIGN8((LLAMA2_MAX_PROMPT + 3) * sizeof(int));
    if (f->quantized)
        n += ALIGN8(qstate_floats(c, f) * sizeof(float)) + 8 /* carved in two blocks */ +
             ALIGN8((size_t)(c->dim / 2) * sizeof(int32_t)) /* RoPE frequencies */ +
             ALIGN8((size_t)c->dim + c->hidden_dim) +
             ALIGN8((7 * (size_t)c->n_layers + 2) * sizeof(llama2_qtensor));
    return n;
}

static void *carve(unsigned char **p, size_t bytes)
{
    void *r = *p;
    *p += ALIGN8(bytes);
    return r;
}

static int compare_tokens(const void *a, const void *b)
{
    return strcmp(((const llama2_token_index *)a)->str, ((const llama2_token_index *)b)->str);
}

/* runq.c init_quantized_tensors: n tensors of size_each int8 values, each followed by its scales */
static llama2_qtensor *map_qtensors(const unsigned char **ptr, int n, size_t size_each, size_t gs,
                                    llama2_qtensor *out)
{
    const unsigned char *p = *ptr;
    for (int i = 0; i < n; i++) {
        out[i].q = (int8_t *)p;
        p += size_each;
        out[i].s = (float *)p;
        p += 4 * (size_each / gs);
    }
    *ptr = p;
    return out;
}

/* runq.c memory_map_weights; the embedding is dequantized once into the arena */
static void init_qweights(llama2 *m, const void *checkpoint, unsigned char **arena)
{
    llama2_config *p = &m->config;
    llama2_qweights *w = &m->qweights;
    size_t gs = m->format.group_size;
    size_t L = p->n_layers, d = p->dim, hd = p->hidden_dim, V = p->vocab_size;
    size_t head_size = d / p->n_heads;
    size_t kv = p->n_kv_heads * head_size;

    float *fptr = (float *)((const unsigned char *)checkpoint + LLAMA2_Q80_HEADER_BYTES);
    w->rms_att_weight = fptr;
    fptr += L * d;
    w->rms_ffn_weight = fptr;
    fptr += L * d;
    w->rms_final_weight = fptr;
    fptr += d;

    llama2_qtensor *t = carve(arena, (7 * L + 2) * sizeof(llama2_qtensor));
    const unsigned char *ptr = (const unsigned char *)fptr;
    w->q_tokens = map_qtensors(&ptr, 1, V * d, gs, t);
    t += 1;
    w->wq = map_qtensors(&ptr, (int)L, d * (p->n_heads * head_size), gs, t);
    t += L;
    w->wk = map_qtensors(&ptr, (int)L, d * kv, gs, t);
    t += L;
    w->wv = map_qtensors(&ptr, (int)L, d * kv, gs, t);
    t += L;
    w->wo = map_qtensors(&ptr, (int)L, (p->n_heads * head_size) * d, gs, t);
    t += L;
    w->w1 = map_qtensors(&ptr, (int)L, d * hd, gs, t);
    t += L;
    w->w2 = map_qtensors(&ptr, (int)L, hd * d, gs, t);
    t += L;
    w->w3 = map_qtensors(&ptr, (int)L, d * hd, gs, t);
    t += L;
    w->wcls = m->format.shared ? w->q_tokens : map_qtensors(&ptr, 1, d * V, gs, t);

    /* filled with the Q16 embedding by fx_prepare() */
    w->token_embedding_table = carve(arena, V * d * sizeof(float));
}

llama2_status llama2_init(llama2 *m, const void *checkpoint, size_t checkpoint_size,
                          const void *tokenizer, size_t tokenizer_size,
                          float temperature, float topp, unsigned long long seed)
{
    memset(m, 0, sizeof *m);
    llama2_status st = llama2_probe(checkpoint, checkpoint_size, &m->config, &m->format);
    if (st != LLAMA2_OK)
        return st;
    if (llama2_file_bytes(&m->config, &m->format) != checkpoint_size)
        return LLAMA2_BAD_SIZE;
    if (tokenizer_size < 4)
        return LLAMA2_BAD_TOKENIZER;

    llama2_config *p = &m->config;
    int shared = m->format.shared;
    int V = p->vocab_size;
    unsigned char *arena = malloc(llama2_runtime_bytes(p, &m->format, tokenizer_size));
    if (!arena)
        return LLAMA2_NO_MEMORY;
    m->arena = arena;

    int head_size = p->dim / p->n_heads;
    size_t L = p->n_layers;
    if (m->format.quantized) {
        init_qweights(m, checkpoint, &arena);
        goto state;
    }

    /* weights, used in place (memory_map_weights) */
    llama2_weights *w = &m->weights;
    float *ptr = (float *)((const unsigned char *)checkpoint + LLAMA2_HEADER_BYTES);
    w->token_embedding_table = ptr;
    ptr += (size_t)p->vocab_size * p->dim;
    w->rms_att_weight = ptr;
    ptr += L * p->dim;
    w->wq = ptr;
    ptr += L * p->dim * (p->n_heads * head_size);
    w->wk = ptr;
    ptr += L * p->dim * (p->n_kv_heads * head_size);
    w->wv = ptr;
    ptr += L * p->dim * (p->n_kv_heads * head_size);
    w->wo = ptr;
    ptr += L * (p->n_heads * head_size) * p->dim;
    w->rms_ffn_weight = ptr;
    ptr += L * p->dim;
    w->w1 = ptr;
    ptr += L * p->dim * p->hidden_dim;
    w->w2 = ptr;
    ptr += L * p->hidden_dim * p->dim;
    w->w3 = ptr;
    ptr += L * p->dim * p->hidden_dim;
    w->rms_final_weight = ptr;
    ptr += p->dim;
    ptr += p->seq_len * head_size / 2; /* legacy freq_cis_real */
    ptr += p->seq_len * head_size / 2; /* legacy freq_cis_imag */
    w->wcls = shared ? w->token_embedding_table : ptr;

state:;
    /* run state (malloc_run_state), zeroed like calloc */
    llama2_state *s = &m->state;
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    if (m->format.quantized) {
        size_t gs = m->format.group_size;
        float *qf = carve(&arena, (2 * (size_t)kv_dim + p->dim / gs + p->hidden_dim / gs) * sizeof(float));
        int8_t *qi = carve(&arena, (size_t)p->dim + p->hidden_dim);
        memset(qi, 0, (size_t)p->dim + p->hidden_dim);
        s->k = qf;          qf += kv_dim;
        s->v = qf;          qf += kv_dim;
        s->xq.s = qf;       qf += p->dim / gs;
        s->hq.s = qf;
        s->xq.q = qi;
        s->hq.q = qi + p->dim;
    }
    float *f = carve(&arena, state_floats(p) * sizeof(float));
    memset(f, 0, state_floats(p) * sizeof(float));
    s->x = f;            f += p->dim;
    s->xb = f;           f += p->dim;
    s->xb2 = f;          f += p->dim;
    s->hb = f;           f += p->hidden_dim;
    s->hb2 = f;          f += p->hidden_dim;
    s->q = f;            f += p->dim;
    s->key_cache = f;    f += L * p->seq_len * kv_dim;
    s->value_cache = f;  f += L * p->seq_len * kv_dim;
    s->att = f;          f += (size_t)p->n_heads * p->seq_len;
    s->logits = f;       f += p->vocab_size;
    s->rope = f;

    /* tokenizer (build_tokenizer), strings packed into one block */
    m->vocab = carve(&arena, V * sizeof(char *));
    m->vocab_scores = carve(&arena, V * sizeof(float));
    char *strings = carve(&arena, tokenizer_size);
    m->sorted_vocab = carve(&arena, V * sizeof(llama2_token_index));
    for (int i = 0; i < 256; i++) {
        m->byte_pieces[i * 2] = (unsigned char)i;
        m->byte_pieces[i * 2 + 1] = '\0';
    }
    const unsigned char *t = tokenizer, *end = t + tokenizer_size;
    int32_t max_len;
    memcpy(&max_len, t, 4);
    t += 4;
    if (max_len <= 0) {
        llama2_free(m);
        return LLAMA2_BAD_TOKENIZER;
    }
    m->max_token_length = (unsigned int)max_len;
    for (int i = 0; i < V; i++) {
        int32_t len;
        if (end - t < 8) {
            llama2_free(m);
            return LLAMA2_BAD_TOKENIZER;
        }
        memcpy(&m->vocab_scores[i], t, 4);
        memcpy(&len, t + 4, 4);
        t += 8;
        if (len < 0 || end - t < len) {
            llama2_free(m);
            return LLAMA2_BAD_TOKENIZER;
        }
        memcpy(strings, t, (size_t)len);
        strings[len] = '\0';
        m->vocab[i] = strings;
        strings += len + 1;
        t += len;
    }
    /* encode() sorts lazily; sorting here keeps generation allocation-free */
    for (int i = 0; i < V; i++) {
        m->sorted_vocab[i].str = m->vocab[i];
        m->sorted_vocab[i].id = i;
    }
    qsort(m->sorted_vocab, V, sizeof(llama2_token_index), compare_tokens);

    /* sampler (build_sampler) */
    m->temperature = temperature;
    m->topp = topp;
    m->rng_state = seed;
    m->probindex = carve(&arena, V * sizeof(llama2_prob_index));

    m->str_buffer = carve(&arena, m->max_token_length * 2 + 1 + 2);
    m->prompt_tokens = carve(&arena, (LLAMA2_MAX_PROMPT + 3) * sizeof(int));

    if (m->format.quantized) {
        m->fx_freq_q30 = carve(&arena, (size_t)(p->dim / 2) * sizeof(int32_t));
        fx_prepare(m);
    }
    return LLAMA2_OK;
}

void llama2_free(llama2 *m)
{
    free(m->arena);
    m->arena = NULL;
}

/* ---- neural net blocks -------------------------------------------------- */

static void rmsnorm(float *o, float *x, float *weight, int size)
{
    float ss = 0.0f;
    for (int j = 0; j < size; j++) {
        ss += x[j] * x[j];
    }
    ss /= size;
    ss += 1e-5f;
    ss = 1.0f / sqrtf(ss);
    for (int j = 0; j < size; j++) {
        o[j] = weight[j] * (ss * x[j]);
    }
}

static void softmax(float *x, int size)
{
    float max_val = x[0];
    for (int i = 1; i < size; i++) {
        if (x[i] > max_val) {
            max_val = x[i];
        }
    }
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    for (int i = 0; i < size; i++) {
        x[i] /= sum;
    }
}

static void matmul(float *xout, float *x, float *w, int n, int d)
{
    for (int i = 0; i < d; i++) {
        float val = 0.0f;
        for (int j = 0; j < n; j++) {
            val += w[i * n + j] * x[j];
        }
        xout[i] = val;
    }
}

/*
 * run.c's RoPE factors (cos, sin) for one position. run.c recomputes them in
 * every layer; they only depend on pos, so computing them once per token
 * gives bit-identical values for a fifth of the powf/cosf/sinf calls.
 */
static void rope_table(float *rope, int dim, int head_size, int pos)
{
    for (int i = 0; i < dim; i += 2) {
        int head_dim = i % head_size;
        float freq = 1.0f / powf(10000.0f, head_dim / (float)head_size);
        float val = pos * freq;
        rope[i] = cosf(val);
        rope[i + 1] = sinf(val);
    }
}

float *llama2_forward(llama2 *m, int token, int pos)
{
    if (m->format.quantized)
        return NULL; /* int8 runs the fixed-point engine: fx_forward() */

    llama2_config *p = &m->config;
    llama2_weights *w = &m->weights;
    llama2_state *s = &m->state;
    float *x = s->x;
    int dim = p->dim;
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    int kv_mul = p->n_heads / p->n_kv_heads;
    int hidden_dim = p->hidden_dim;
    int head_size = dim / p->n_heads;
    float sqrt_head_size = sqrtf(head_size);
    rope_table(s->rope, dim, head_size, pos);

    float *content_row = w->token_embedding_table + token * dim;
    memcpy(x, content_row, dim * sizeof(*x));

    for (int l = 0; l < p->n_layers; l++) {
        rmsnorm(s->xb, x, w->rms_att_weight + l * dim, dim);

        int loff = l * p->seq_len * kv_dim;
        s->k = s->key_cache + loff + pos * kv_dim;
        s->v = s->value_cache + loff + pos * kv_dim;

        matmul(s->q, s->xb, w->wq + l * dim * dim, dim, dim);
        matmul(s->k, s->xb, w->wk + l * dim * kv_dim, dim, kv_dim);
        matmul(s->v, s->xb, w->wv + l * dim * kv_dim, dim, kv_dim);

        for (int i = 0; i < dim; i += 2) {
            float fcr = s->rope[i];
            float fci = s->rope[i + 1];
            int rotn = i < kv_dim ? 2 : 1;
            for (int v = 0; v < rotn; v++) {
                float *vec = v == 0 ? s->q : s->k;
                float v0 = vec[i];
                float v1 = vec[i + 1];
                vec[i] = v0 * fcr - v1 * fci;
                vec[i + 1] = v0 * fci + v1 * fcr;
            }
        }

        for (int h = 0; h < p->n_heads; h++) {
            float *q = s->q + h * head_size;
            float *att = s->att + h * p->seq_len;
            for (int t = 0; t <= pos; t++) {
                float *k = s->key_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                float score = 0.0f;
                for (int i = 0; i < head_size; i++) {
                    score += q[i] * k[i];
                }
                score /= sqrt_head_size;
                att[t] = score;
            }

            softmax(att, pos + 1);

            float *xb = s->xb + h * head_size;
            memset(xb, 0, head_size * sizeof(float));
            for (int t = 0; t <= pos; t++) {
                float *v = s->value_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                float a = att[t];
                for (int i = 0; i < head_size; i++) {
                    xb[i] += a * v[i];
                }
            }
        }

        matmul(s->xb2, s->xb, w->wo + l * dim * dim, dim, dim);

        for (int i = 0; i < dim; i++) {
            x[i] += s->xb2[i];
        }

        rmsnorm(s->xb, x, w->rms_ffn_weight + l * dim, dim);

        matmul(s->hb, s->xb, w->w1 + l * dim * hidden_dim, dim, hidden_dim);
        matmul(s->hb2, s->xb, w->w3 + l * dim * hidden_dim, dim, hidden_dim);

        for (int i = 0; i < hidden_dim; i++) {
            float val = s->hb[i];
            val *= (1.0f / (1.0f + expf(-val)));
            val *= s->hb2[i];
            s->hb[i] = val;
        }

        matmul(s->xb, s->hb, w->w2 + l * dim * hidden_dim, hidden_dim, dim);

        for (int i = 0; i < dim; i++) {
            x[i] += s->xb[i];
        }
    }

    rmsnorm(x, x, w->rms_final_weight, dim);

    matmul(s->logits, x, w->wcls, p->dim, p->vocab_size);
    return s->logits;
}

/* ---- tokenizer ---------------------------------------------------------- */

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* Same match as sscanf(piece, "<0x%02hhX>", &byte) == 1. */
static int raw_byte_token(const char *piece, unsigned char *byte)
{
    if (piece[0] != '<' || piece[1] != '0' || piece[2] != 'x')
        return 0;
    int hi = hex_digit(piece[3]);
    if (hi < 0)
        return 0;
    int lo = hex_digit(piece[4]);
    *byte = (unsigned char)(lo < 0 ? hi : hi * 16 + lo);
    return 1;
}

static const char *decode(llama2 *m, int prev_token, int token)
{
    const char *piece = m->vocab[token];
    if (prev_token == LLAMA2_BOS && piece[0] == ' ') {
        piece++;
    }
    unsigned char byte_val;
    if (raw_byte_token(piece, &byte_val)) {
        piece = (const char *)m->byte_pieces + byte_val * 2;
    }
    return piece;
}

/* safe_printf's filter: drop single non-printable, non-space bytes. */
static int printable(const char *piece)
{
    if (piece == NULL || piece[0] == '\0')
        return 0;
    if (piece[1] == '\0') {
        unsigned char b = (unsigned char)piece[0];
        if (!(isprint(b) || isspace(b)))
            return 0;
    }
    return 1;
}

int llama2_next(llama2 *m, int token, int pos)
{
    if (m->format.quantized) {
        fx_begin(m); /* the temperature may have changed since the last call */
        return fx_sample(m, fx_forward(m, token, pos));
    }
    return llama2_sample(m, llama2_forward(m, token, pos));
}

void llama2_feed(llama2 *m, int token, int pos)
{
    if (m->format.quantized)
        fx_forward(m, token, pos);
    else
        llama2_forward(m, token, pos);
}

const char *llama2_token_text(llama2 *m, int prev, int token)
{
    if (token < 0 || token >= m->config.vocab_size)
        return "";
    const char *piece = decode(m, prev, token);
    return printable(piece) ? piece : "";
}

static int str_lookup(char *str, llama2_token_index *sorted_vocab, int vocab_size)
{
    llama2_token_index tok = {.str = str};
    llama2_token_index *res = bsearch(&tok, sorted_vocab, vocab_size,
                                      sizeof(llama2_token_index), compare_tokens);
    return res != NULL ? res->id : -1;
}

static void encode(llama2 *m, const char *text, int *tokens, int *n_tokens)
{
    char *str_buffer = m->str_buffer;
    size_t str_len = 0;
    int V = m->config.vocab_size;

    *n_tokens = 0;
    tokens[(*n_tokens)++] = LLAMA2_BOS;

    if (text[0] != '\0') {
        int dummy_prefix = str_lookup(" ", m->sorted_vocab, V);
        tokens[(*n_tokens)++] = dummy_prefix;
    }

    for (const char *c = text; *c != '\0'; c++) {
        if ((*c & 0xC0) != 0x80) {
            str_len = 0;
        }
        str_buffer[str_len++] = *c;
        str_buffer[str_len] = '\0';
        if ((*(c + 1) & 0xC0) == 0x80 && str_len < 4) {
            continue;
        }
        int id = str_lookup(str_buffer, m->sorted_vocab, V);
        if (id != -1) {
            tokens[(*n_tokens)++] = id;
        } else {
            for (size_t i = 0; i < str_len; i++) {
                tokens[(*n_tokens)++] = (unsigned char)str_buffer[i] + 3;
            }
        }
        str_len = 0;
    }

    while (1) {
        float best_score = -1e10;
        int best_id = -1;
        int best_idx = -1;

        for (int i = 0; i < (*n_tokens - 1); i++) {
            strcpy(str_buffer, m->vocab[tokens[i]]);
            strcat(str_buffer, m->vocab[tokens[i + 1]]);
            int id = str_lookup(str_buffer, m->sorted_vocab, V);
            if (id != -1 && m->vocab_scores[id] > best_score) {
                best_score = m->vocab_scores[id];
                best_id = id;
                best_idx = i;
            }
        }

        if (best_idx == -1) {
            break;
        }

        tokens[best_idx] = best_id;
        for (int i = best_idx + 1; i < (*n_tokens - 1); i++) {
            tokens[i] = tokens[i + 1];
        }
        (*n_tokens)--;
    }
}

/* ---- sampler ------------------------------------------------------------ */

static int sample_argmax(float *probabilities, int n)
{
    int max_i = 0;
    float max_p = probabilities[0];
    for (int i = 1; i < n; i++) {
        if (probabilities[i] > max_p) {
            max_i = i;
            max_p = probabilities[i];
        }
    }
    return max_i;
}

static int sample_mult(float *probabilities, int n, float coin)
{
    float cdf = 0.0f;
    for (int i = 0; i < n; i++) {
        cdf += probabilities[i];
        if (coin < cdf) {
            return i;
        }
    }
    return n - 1;
}

static int compare_prob(const void *a, const void *b)
{
    const llama2_prob_index *a_ = a;
    const llama2_prob_index *b_ = b;
    if (a_->prob > b_->prob)
        return -1;
    if (a_->prob < b_->prob)
        return 1;
    return 0;
}

static int sample_topp(float *probabilities, int n, float topp, llama2_prob_index *probindex, float coin)
{
    int n0 = 0;
    const float cutoff = (1.0f - topp) / (n - 1);
    for (int i = 0; i < n; i++) {
        if (probabilities[i] >= cutoff) {
            probindex[n0].index = i;
            probindex[n0].prob = probabilities[i];
            n0++;
        }
    }
    qsort(probindex, n0, sizeof(llama2_prob_index), compare_prob);

    float cumulative_prob = 0.0f;
    int last_idx = n0 - 1;
    for (int i = 0; i < n0; i++) {
        cumulative_prob += probindex[i].prob;
        if (cumulative_prob > topp) {
            last_idx = i;
            break;
        }
    }

    float r = coin * cumulative_prob;
    float cdf = 0.0f;
    for (int i = 0; i <= last_idx; i++) {
        cdf += probindex[i].prob;
        if (r < cdf) {
            return probindex[i].index;
        }
    }
    return probindex[last_idx].index;
}

/* Shared with the fixed-point sampler (llama2_fixed.arm.c). */
unsigned int llama2_rand_u32(unsigned long long *state)
{
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return (*state * 0x2545F4914F6CDD1Dull) >> 32;
}

static float random_f32(unsigned long long *state)
{
    return (llama2_rand_u32(state) >> 8) / 16777216.0f;
}

int llama2_sample(llama2 *m, float *logits)
{
    int V = m->config.vocab_size;
    int next;
    if (m->temperature == 0.0f) {
        next = sample_argmax(logits, V);
    } else {
        for (int q = 0; q < V; q++) {
            logits[q] /= m->temperature;
        }
        softmax(logits, V);
        float coin = random_f32(&m->rng_state);
        if (m->topp <= 0 || m->topp >= 1) {
            next = sample_mult(logits, V, coin);
        } else {
            next = sample_topp(logits, V, m->topp, m->probindex, coin);
        }
    }
    return next;
}

/* ---- generation loop ---------------------------------------------------- */

int llama2_generate(llama2 *m, const char *prompt, int steps,
                    int *out, int out_cap, int *n_prompt_tokens,
                    llama2_token_fn on_token, void *user)
{
    if (strlen(prompt) > LLAMA2_MAX_PROMPT)
        return -1;
    if (steps <= 0 || steps > m->config.seq_len)
        steps = m->config.seq_len;

    int num_prompt_tokens = 0;
    int *prompt_tokens = m->prompt_tokens;
    encode(m, prompt, prompt_tokens, &num_prompt_tokens);
    if (n_prompt_tokens)
        *n_prompt_tokens = num_prompt_tokens;

    int n_out = 0;
    int token = prompt_tokens[0];
    if (out && n_out < out_cap)
        out[n_out] = token;
    n_out++;

    int pos = 0;
    int quantized = m->format.quantized;
    if (quantized)
        fx_begin(m); /* sampling settings to fixed point, outside the per-token loop */
    while (pos < steps) {
        float *logits = NULL;
        int32_t *ilogits = NULL;
        if (quantized)
            ilogits = fx_forward(m, token, pos);
        else
            logits = llama2_forward(m, token, pos);
        int next;
        if (pos < num_prompt_tokens - 1) {
            next = prompt_tokens[pos + 1];
        } else {
            next = -1;
            if (m->on_step) { /* absent by default: the sampled sequence is unchanged */
                llama2_step st = {logits, ilogits, m->config.vocab_size, pos, token};
                next = m->on_step(&st, m->step_user);
                if (next >= m->config.vocab_size)
                    next = -1;
            }
            if (next < 0)
                next = quantized ? fx_sample(m, ilogits) : llama2_sample(m, logits);
        }
        pos++;

        if (next == LLAMA2_BOS) {
            break;
        }

        if (out && n_out < out_cap)
            out[n_out] = next;
        n_out++;
        const char *piece = decode(m, token, next);
        token = next;
        if (on_token && on_token(next, printable(piece) ? piece : "", user))
            break;
    }
    return n_out;
}
