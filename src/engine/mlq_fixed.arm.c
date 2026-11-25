/*
 * Fixed-point LittleKedi forward pass (see mlq.h). Same structure as the fp32
 * reference mla.c: absorbed MLA, interleaved RoPE, sigmoid top-k experts plus
 * a shared expert. Q16 activations; int8 matmuls through fxk_matmul (ITCM).
 */
#include "mlq.h"

#include <stdlib.h>
#include <string.h>

#include "fxmath.h"
#include "fxtables.h"

mla_status mlq_read_config(const void *header, size_t n, mla_config *c, int *group_size)
{
    unsigned char tmp[MLA_HEADER_BYTES];
    if (n < MLA_HEADER_BYTES || memcmp(header, "MLQ1", 4) != 0)
        return MLA_BAD_HEADER;
    memcpy(tmp, header, sizeof tmp);
    memcpy(tmp, "MLA1", 4);
    mla_status st = mla_read_config(tmp, sizeof tmp, c);
    if (st != MLA_OK)
        return st;
    int32_t gs;
    memcpy(&gs, tmp + 76, 4);
    int inner[5] = {c->dim, c->kv, c->n_heads * c->vd, c->dense_hidden, c->moe_hidden};
    if (gs <= 0 || gs > 64)
        return MLA_BAD_HEADER;
    for (int i = 0; i < 5; i++)
        if (inner[i] % gs)
            return MLA_BAD_HEADER;
    if (c->kv > 64 || c->n_active > 16 || c->theta != 10000.0f)
        return MLA_BAD_HEADER; /* sizes of the local arrays; FX_LOG2_10000_Q16 assumes theta 10000 */
    *group_size = gs;
    return MLA_OK;
}

static size_t mat_bytes(size_t rows, size_t cols, int gs)
{
    return rows * cols + 4 * (rows * cols / gs);
}

size_t mlq_file_bytes(const mla_config *c, int gs)
{
    size_t d = c->dim, qd = (size_t)c->n_heads * (c->nope + c->rope), mh = c->moe_hidden;
    size_t n = MLA_HEADER_BYTES;
    for (int l = 0; l < c->n_layers; l++) {
        n += 4 * d + mat_bytes(qd, d, gs) + mat_bytes(c->kv + c->rope, d, gs) + 4 * (size_t)c->kv +
             mat_bytes((size_t)c->n_heads * (c->nope + c->vd), c->kv, gs) +
             mat_bytes(d, (size_t)c->n_heads * c->vd, gs) + 4 * d;
        if (l < c->n_dense)
            n += 2 * mat_bytes(c->dense_hidden, d, gs) + mat_bytes(d, c->dense_hidden, gs);
        else
            n += 4 * (size_t)c->n_experts * d + 4 * (size_t)c->n_experts + /* router, fp32 */
                 (size_t)(c->n_experts + 1) * (2 * mat_bytes(mh, d, gs) + mat_bytes(d, mh, gs));
    }
    return n + 4 * d + mat_bytes(c->vocab, d, gs);
}

/* longest vector that gets quantized: dim, the hidden sizes, or the concatenated heads */
static size_t widest_input(const mla_config *c)
{
    size_t w = (size_t)c->dim;
    size_t cand[3] = {(size_t)c->dense_hidden, (size_t)c->moe_hidden, (size_t)c->n_heads * c->vd};
    for (int i = 0; i < 3; i++)
        if (cand[i] > w)
            w = cand[i];
    return w;
}

static size_t state_words(const mla_config *c, int seq)
{
    size_t qd = (size_t)c->n_heads * (c->nope + c->rope);
    size_t hidden = c->dense_hidden > c->moe_hidden ? c->dense_hidden : c->moe_hidden;
    size_t widest = widest_input(c);
    return 3 * (size_t)c->dim + qd + (c->kv + c->rope) + 2 * (size_t)c->kv + (size_t)c->n_heads * c->vd + seq +
           2 * hidden + c->dim + c->vocab + c->n_experts + c->rope + c->rope / 2 + /* rope table, frequencies */
           3 * widest +                                                       /* activation scales */
           (size_t)c->n_layers * seq * (c->kv + c->rope);                     /* latent + RoPE key cache */
}

size_t mlq_runtime_bytes(const mla_config *c, int seq)
{
    size_t widest = widest_input(c);
    return 4 * state_words(c, seq) + 3 * widest + (size_t)c->vocab * sizeof(fxk_prob) + (size_t)(c->vocab + 7) / 8 +
           (size_t)c->n_layers * sizeof(mlq_layer) + (size_t)c->n_layers * c->n_experts * 3 * sizeof(mlq_tensor) + 64;
}

int mlq_tokenizer_name(const void *header, size_t n, char *out, size_t cap)
{
    const char *name = (const char *)header + 80;
    size_t len = 0;
    if (n < MLA_HEADER_BYTES)
        return -1;
    while (len < 48 && name[len])
        len++;
    if (len == 0 || len == 48 || len >= cap || memchr(name, '/', len))
        return -1;
    memcpy(out, name, len);
    out[len] = '\0';
    return 0;
}

int mlq_fit_seq(const mla_config *c, size_t budget)
{
    int best = 0;
    for (int seq = 64; seq <= c->max_seq; seq += 64)
        if (mlq_runtime_bytes(c, seq) <= budget)
            best = seq;
    return best;
}

/* map one int8 matrix; its scales become Q30 in place */
static mlq_tensor take_mat(unsigned char **p, size_t rows, size_t cols, int gs)
{
    mlq_tensor t;
    t.q = (const int8_t *)*p;
    *p += rows * cols;
    fxk_floats_to_fixed(*p, rows * cols / gs, 30);
    t.s = (const int32_t *)*p;
    *p += 4 * (rows * cols / gs);
    return t;
}

/* map one fp32 vector, converted to Q16 in place */
static const int32_t *take_vec(unsigned char **p, size_t n)
{
    fxk_floats_to_fixed(*p, n, 16);
    const int32_t *v = (const int32_t *)*p;
    *p += 4 * n;
    return v;
}

mla_status mlq_init(mlq_model *m, void *file, size_t size, int seq)
{
    memset(m, 0, sizeof *m);
    mla_status st = mlq_read_config(file, size, &m->c, &m->gs);
    if (st != MLA_OK)
        return st;
    if (mlq_file_bytes(&m->c, m->gs) != size)
        return MLA_BAD_SIZE;
    mla_config *c = &m->c;
    int gs = m->gs;
    m->seq = seq > 0 && seq < c->max_seq ? seq : c->max_seq;
    unsigned char *arena = malloc(mlq_runtime_bytes(c, m->seq));
    if (!arena)
        return MLA_NO_MEMORY;
    m->arena = arena;

    size_t hidden = c->dense_hidden > c->moe_hidden ? c->dense_hidden : c->moe_hidden;
    size_t widest = widest_input(c);
    size_t qd = (size_t)c->n_heads * (c->nope + c->rope);
    m->layers = (mlq_layer *)arena;
    arena += (size_t)c->n_layers * sizeof(mlq_layer);
    mlq_tensor *experts = (mlq_tensor *)arena;
    arena += (size_t)c->n_layers * c->n_experts * 3 * sizeof(mlq_tensor);
    int32_t *w = (int32_t *)arena;
    memset(w, 0, 4 * state_words(c, m->seq));
    m->x = w;        w += c->dim;
    m->xb = w;       w += c->dim;
    m->xb2 = w;      w += c->dim;
    m->q = w;        w += qd;
    m->kvb = w;      w += c->kv + c->rope;
    m->qlat = w;     w += c->kv;
    m->ctx = w;      w += c->kv;
    m->heads = w;    w += (size_t)c->n_heads * c->vd;
    m->att = w;      w += m->seq;
    m->hb = w;       w += hidden;
    m->hb2 = w;      w += hidden;
    m->ex = w;       w += c->dim;
    m->logits = w;   w += c->vocab;
    m->scores = w;   w += c->n_experts;
    m->rope_cs = w;  w += c->rope;
    m->freq_q30 = w; w += c->rope / 2;
    m->xs = w;       w += widest;
    m->hs = w;       w += widest;
    m->cs = w;       w += widest;
    m->lat_cache = w; w += (size_t)c->n_layers * m->seq * c->kv;
    m->kpe_cache = w; w += (size_t)c->n_layers * m->seq * c->rope;
    unsigned char *b = (unsigned char *)w;
    m->xq = (int8_t *)b;  b += widest;
    m->hq = (int8_t *)b;  b += widest;
    m->cq = (int8_t *)b;  b += widest;
    b = (unsigned char *)(((uintptr_t)b + 7) & ~(uintptr_t)7);
    m->cand = (fxk_prob *)b; b += (size_t)c->vocab * sizeof(fxk_prob);
    m->seen = b;

    unsigned char *p = (unsigned char *)file + MLA_HEADER_BYTES;
    size_t d = c->dim, mh = c->moe_hidden;
    for (int l = 0; l < c->n_layers; l++) {
        mlq_layer *L = &m->layers[l];
        L->attn_norm = take_vec(&p, d);
        L->wq = take_mat(&p, qd, d, gs);
        L->wkv_a = take_mat(&p, c->kv + c->rope, d, gs);
        L->kv_norm = take_vec(&p, c->kv);
        L->wkv_b = take_mat(&p, (size_t)c->n_heads * (c->nope + c->vd), c->kv, gs);
        L->wo = take_mat(&p, d, (size_t)c->n_heads * c->vd, gs);
        L->ffn_norm = take_vec(&p, d);
        if (l < c->n_dense) {
            L->w1 = take_mat(&p, c->dense_hidden, d, gs);
            L->w2 = take_mat(&p, d, c->dense_hidden, gs);
            L->w3 = take_mat(&p, c->dense_hidden, d, gs);
            continue;
        }
        L->gate_w = take_vec(&p, (size_t)c->n_experts * d);
        L->gate_b = take_vec(&p, c->n_experts);
        L->experts = experts + (size_t)l * c->n_experts * 3;
        for (int e = 0; e < c->n_experts; e++) {
            L->experts[3 * e] = take_mat(&p, mh, d, gs);
            L->experts[3 * e + 1] = take_mat(&p, mh, d, gs);
            L->experts[3 * e + 2] = take_mat(&p, d, mh, gs);
        }
        L->sw1 = take_mat(&p, mh, d, gs);
        L->sw2 = take_mat(&p, d, mh, gs);
        L->sw3 = take_mat(&p, mh, d, gs);
    }
    m->norm = take_vec(&p, d);
    m->embed = take_mat(&p, c->vocab, d, gs);

    m->route_scale = fxk_to_fixed(c->route_scale, 16);
    int hd = c->nope + c->rope;
    m->inv_sqrt_hd = (int32_t)(((int64_t)1 << 32) / fx_isqrt64((uint64_t)hd << 32));
    /* RoPE: freq_i = theta^(-2i / rope) = 2^(-2i * log2(10000) / rope) */
    for (int i = 0; i < c->rope / 2; i++)
        m->freq_q30[i] = fx_exp2_neg_q30(-(int32_t)(((int64_t)2 * i * FX_LOG2_10000_Q16) / c->rope));
    return MLA_OK;
}

void mlq_free(mlq_model *m)
{
    free(m->arena);
    m->arena = NULL;
}

/* ---- per token: integers only ------------------------------------------- */

static void matmul_q(int32_t *out, const int8_t *xq, const int32_t *xs, const mlq_tensor *w, int n, int d, int gs)
{
    fxk_matmul(out, xq, xs, w->q, w->s, n, d, gs);
}

/* rows [r0, r0 + d) of a matrix with n columns */
static void matmul_rows(int32_t *out, const int8_t *xq, const int32_t *xs, const mlq_tensor *w, int r0, int n,
                        int d, int gs)
{
    fxk_matmul(out, xq, xs, w->q + (size_t)r0 * n, w->s + (size_t)r0 * n / gs, n, d, gs);
}

static void rope_rotate(int32_t *v, const int32_t *cs, int rope)
{
    for (int i = 0; i < rope / 2; i++) {
        int64_t c = cs[2 * i], s = cs[2 * i + 1];
        int64_t a = v[2 * i], b = v[2 * i + 1];
        v[2 * i] = (int32_t)((a * c - b * s) >> 16);
        v[2 * i + 1] = (int32_t)((a * s + b * c) >> 16);
    }
}

/* SwiGLU on an already quantized input: out = down(silu(gate x) * up x) */
static void swiglu_q(mlq_model *m, int32_t *out, const mlq_tensor *gate, const mlq_tensor *up,
                     const mlq_tensor *down, int hidden)
{
    int dim = m->c.dim, gs = m->gs;
    matmul_q(m->hb, m->xq, m->xs, gate, dim, hidden, gs);
    matmul_q(m->hb2, m->xq, m->xs, up, dim, hidden, gs);
    fxk_silu_mul(m->hb, m->hb2, hidden);
    fxk_quantize(m->hq, m->hs, m->hb, hidden, gs);
    matmul_q(out, m->hq, m->hs, down, hidden, dim, gs);
}

int32_t *mlq_forward(mlq_model *m, int token, int pos)
{
    mla_config *c = &m->c;
    int dim = c->dim, gs = m->gs, kv = c->kv, rope = c->rope, nope = c->nope, vd = c->vd;
    int hd = nope + rope;
    int32_t *x = m->x;

    /* embedding row, dequantized: q * s (Q30) -> Q16 */
    const int8_t *eq = m->embed.q + (size_t)token * dim;
    const int32_t *es = m->embed.s + (size_t)token * dim / gs;
    for (int i = 0; i < dim; i++)
        x[i] = (int32_t)(((int64_t)eq[i] * es[i / gs] + (1 << 13)) >> 14);

    for (int i = 0; i < rope / 2; i++) {
        int32_t sn, cs;
        fx_sincos((int64_t)pos * m->freq_q30[i], &sn, &cs);
        m->rope_cs[2 * i] = cs;
        m->rope_cs[2 * i + 1] = sn;
    }

    for (int l = 0; l < c->n_layers; l++) {
        mlq_layer *L = &m->layers[l];
        int32_t *lat_l = m->lat_cache + (size_t)l * m->seq * kv;
        int32_t *kpe_l = m->kpe_cache + (size_t)l * m->seq * rope;

        fxk_rmsnorm(m->xb, x, L->attn_norm, dim, FXK_EPS_1E6_Q32);
        fxk_quantize(m->xq, m->xs, m->xb, dim, gs);
        matmul_q(m->q, m->xq, m->xs, &L->wq, dim, c->n_heads * hd, gs);
        matmul_q(m->kvb, m->xq, m->xs, &L->wkv_a, dim, kv + rope, gs);
        fxk_rmsnorm(lat_l + (size_t)pos * kv, m->kvb, L->kv_norm, kv, FXK_EPS_1E6_Q32);
        memcpy(kpe_l + (size_t)pos * rope, m->kvb + kv, rope * sizeof(int32_t));
        rope_rotate(kpe_l + (size_t)pos * rope, m->rope_cs, rope);

        for (int h = 0; h < c->n_heads; h++) {
            int32_t *qh = m->q + (size_t)h * hd; /* [nope | rope] */
            rope_rotate(qh + nope, m->rope_cs, rope);
            /* absorbed query: q_lat[k] = sum_i q_nope[i] * Wk[i][k]; Wk rows of wkv_b, kv columns */
            int row0 = h * (nope + vd);
            int64_t acc[64] = {0};
            for (int i = 0; i < nope; i++) {
                const int8_t *wr = L->wkv_b.q + (size_t)(row0 + i) * kv;
                const int32_t *sr = L->wkv_b.s + (size_t)(row0 + i) * kv / gs;
                for (int g = 0; g < kv / gs; g++) {
                    int64_t a = ((int64_t)qh[i] * sr[g]) >> 16; /* Q30 */
                    for (int k = g * gs; k < (g + 1) * gs; k++)
                        acc[k] += a * wr[k];
                }
            }
            for (int k = 0; k < kv; k++)
                m->qlat[k] = (int32_t)(acc[k] >> 14); /* Q16 */

            for (int t = 0; t <= pos; t++) {
                const int32_t *lt = lat_l + (size_t)t * kv, *kt = kpe_l + (size_t)t * rope;
                int64_t s = 0;
                for (int k = 0; k < kv; k++)
                    s += (int64_t)m->qlat[k] * lt[k];
                for (int i = 0; i < rope; i++)
                    s += (int64_t)qh[nope + i] * kt[i];
                m->att[t] = (int32_t)(((s >> 16) * m->inv_sqrt_hd) >> 16);
            }
            fxk_softmax(m->att, pos + 1);

            int64_t cacc[64] = {0};
            for (int t = 0; t <= pos; t++) {
                const int32_t *lt = lat_l + (size_t)t * kv;
                int64_t a = m->att[t];
                for (int k = 0; k < kv; k++)
                    cacc[k] += a * lt[k];
            }
            for (int k = 0; k < kv; k++)
                m->ctx[k] = (int32_t)(cacc[k] >> 16);
            fxk_quantize(m->cq, m->cs, m->ctx, kv, gs);
            matmul_rows(m->heads + (size_t)h * vd, m->cq, m->cs, &L->wkv_b, row0 + nope, kv, vd, gs);
        }
        fxk_quantize(m->hq, m->hs, m->heads, c->n_heads * vd, gs);
        matmul_q(m->xb2, m->hq, m->hs, &L->wo, c->n_heads * vd, dim, gs);
        for (int i = 0; i < dim; i++)
            x[i] += m->xb2[i];

        fxk_rmsnorm(m->xb, x, L->ffn_norm, dim, FXK_EPS_1E6_Q32);
        fxk_quantize(m->xq, m->xs, m->xb, dim, gs);
        if (l < c->n_dense) {
            swiglu_q(m, m->xb2, &L->w1, &L->w3, &L->w2, c->dense_hidden);
        } else {
            int ne = c->n_experts;
            for (int e = 0; e < ne; e++) { /* router in full Q16: no activation quantization */
                const int32_t *wr = L->gate_w + (size_t)e * dim;
                int64_t acc = 0;
                for (int i = 0; i < dim; i++)
                    acc += (int64_t)wr[i] * m->xb[i];
                m->scores[e] = fxk_sigmoid((int32_t)(acc >> 16));
            }
            int pick[16];
            int64_t total = 0;
            for (int k = 0; k < c->n_active; k++) {
                int best = -1;
                for (int e = 0; e < ne; e++) {
                    int used = 0;
                    for (int j = 0; j < k; j++)
                        used |= pick[j] == e;
                    if (!used && (best < 0 || m->scores[e] + L->gate_b[e] > m->scores[best] + L->gate_b[best]))
                        best = e;
                }
                pick[k] = best;
                total += m->scores[best];
            }
            swiglu_q(m, m->xb2, &L->sw1, &L->sw3, &L->sw2, c->moe_hidden); /* shared expert */
            for (int k = 0; k < c->n_active; k++) {
                const mlq_tensor *e = L->experts + 3 * pick[k];
                swiglu_q(m, m->ex, &e[0], &e[1], &e[2], c->moe_hidden);
                int64_t wgt = total ? ((int64_t)m->scores[pick[k]] << 16) / total : 0; /* Q16 */
                wgt = (wgt * m->route_scale) >> 16;
                for (int i = 0; i < dim; i++)
                    m->xb2[i] += (int32_t)((wgt * m->ex[i]) >> 16);
            }
        }
        for (int i = 0; i < dim; i++)
            x[i] += m->xb2[i];
    }

    fxk_rmsnorm(x, x, m->norm, dim, FXK_EPS_1E6_Q32);
    fxk_quantize(m->xq, m->xs, x, dim, gs);
    matmul_q(m->logits, m->xq, m->xs, &m->embed, dim, c->vocab, gs);
    return m->logits;
}

int mlq_sample(mlq_model *m, int32_t *logits, const int *recent, int n_recent, int32_t penalty_q16,
               int32_t inv_temp_q16, int32_t topp_q16, unsigned long long *rng)
{
    int n = m->c.vocab;
    if (penalty_q16 > 0 && penalty_q16 != FX_ONE) {
        /* llama.cpp-style: each distinct recent token once; positive logits divided, negative multiplied */
        memset(m->seen, 0, (size_t)(n + 7) / 8);
        for (int i = 0; i < n_recent; i++) {
            int t = recent[i];
            if (t < 0 || t >= n || (m->seen[t >> 3] & (1 << (t & 7))))
                continue;
            m->seen[t >> 3] |= (uint8_t)(1 << (t & 7));
            int64_t v = logits[t];
            logits[t] = (int32_t)(v > 0 ? (v << 16) / penalty_q16 : (v * penalty_q16) >> 16);
        }
    }
    return fxk_sample(logits, n, inv_temp_q16, topp_q16, rng, m->cand);
}
