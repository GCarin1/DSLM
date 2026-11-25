/*
 * Fixed-point int8 engine (fixed-point-inference, ADR 0003 and 0008).
 *
 * Same structure as llama2.c's runq.c forward pass, with no float after load:
 *   activations, norms, RoPE, attention, logits   Q16 in int32
 *   weight scales                                  Q30 in int32 (in place)
 *   activation scales                              Q24 in int32
 *   dot products                                   int32 per group, int64 per row
 *   exp, sin/cos, 1/sqrt                           fxmath.c (integer, no libm)
 * Every operation is integer, so the DS and the host build agree bit for bit;
 * the drift against runq.c (float) is measured by the host tests.
 */
#include "llama2_fixed.h"

#include <stdlib.h>
#include <string.h>

#include "fxkern.h"
#include "fxmath.h"
#include "fxtables.h"




/* ---- load time (float allowed) ----------------------------------------- */

#define to_fixed fxk_to_fixed
#define floats_to_fixed fxk_floats_to_fixed

static void scales_to_q30(llama2_qtensor *t, int n, size_t size_each, int gs)
{
    for (int i = 0; i < n; i++)
        floats_to_fixed(t[i].s, size_each / gs, 30);
}

void fx_prepare(llama2 *m)
{
    llama2_config *p = &m->config;
    llama2_qweights *w = &m->qweights;
    int gs = m->format.group_size;
    int L = p->n_layers;
    size_t d = p->dim, hd = p->hidden_dim, V = p->vocab_size;
    int hs = p->dim / p->n_heads;
    size_t kv = (size_t)p->n_kv_heads * hs;

    floats_to_fixed(w->rms_att_weight, L * d, 16);
    floats_to_fixed(w->rms_ffn_weight, L * d, 16);
    floats_to_fixed(w->rms_final_weight, d, 16);

    scales_to_q30(w->q_tokens, 1, V * d, gs);
    scales_to_q30(w->wq, L, d * d, gs);
    scales_to_q30(w->wk, L, d * kv, gs);
    scales_to_q30(w->wv, L, d * kv, gs);
    scales_to_q30(w->wo, L, d * d, gs);
    scales_to_q30(w->w1, L, d * hd, gs);
    scales_to_q30(w->w2, L, hd * d, gs);
    scales_to_q30(w->w3, L, d * hd, gs);
    if (!m->format.shared)
        scales_to_q30(w->wcls, 1, d * V, gs);

    /* Q16 embedding: q * s (Q30) rounded to Q16 */
    int32_t *emb = (int32_t *)w->token_embedding_table;
    const int32_t *es = (const int32_t *)w->q_tokens->s;
    for (size_t i = 0; i < V * d; i++) {
        int64_t v = (int64_t)w->q_tokens->q[i] * es[i / gs];
        emb[i] = (int32_t)((v + (1 << 13)) >> 14);
    }

    /* RoPE: freq = 10000^(-head_dim / head_size) = 2^(-head_dim * log2(10000) / head_size) */
    for (int i = 0; i < p->dim; i += 2) {
        int head_dim = i % hs;
        int32_t e = (int32_t)(((int64_t)head_dim * FX_LOG2_10000_Q16) / hs);
        m->fx_freq_q30[i / 2] = fx_exp2_neg_q30(-e);
    }
    m->fx_inv_sqrt_hs = (int32_t)(((int64_t)1 << 32) / fx_isqrt64((uint64_t)hs << 32));
}

void fx_begin(llama2 *m)
{
    m->fx_inv_temp = m->temperature == 0.0f ? 0 : to_fixed(1.0f / m->temperature, 16);
    m->fx_topp = to_fixed(m->topp, 16);
}

/* ---- per token: integers only ------------------------------------------ */

static void rmsnorm_fx(int32_t *o, const int32_t *x, const int32_t *weight, int size)
{
    fxk_rmsnorm(o, x, weight, size, FXK_EPS_1E5_Q32);
}

static void matmul_fx(int32_t *xout, const int8_t *xq, const int32_t *xs, const llama2_qtensor *w, int n, int d,
                      int gs)
{
    fxk_matmul(xout, xq, xs, w->q, (const int32_t *)w->s, n, d, gs);
}

#define quantize_fx fxk_quantize
#define softmax_fx fxk_softmax

int32_t *fx_forward(llama2 *m, int token, int pos)
{
    llama2_config *p = &m->config;
    llama2_qweights *w = &m->qweights;
    llama2_state *s = &m->state;
    int gs = m->format.group_size;
    int dim = p->dim;
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    int kv_mul = p->n_heads / p->n_kv_heads;
    int hidden_dim = p->hidden_dim;
    int head_size = dim / p->n_heads;

    /* The run state buffers are 4-byte cells; the int8 engine stores Q16 in them. */
    int32_t *x = (int32_t *)s->x, *xb = (int32_t *)s->xb, *xb2 = (int32_t *)s->xb2;
    int32_t *hb = (int32_t *)s->hb, *hb2 = (int32_t *)s->hb2;
    int32_t *q = (int32_t *)s->q, *k = (int32_t *)s->k, *v = (int32_t *)s->v;
    int32_t *att = (int32_t *)s->att, *logits = (int32_t *)s->logits, *rope = (int32_t *)s->rope;
    int32_t *key_cache = (int32_t *)s->key_cache, *value_cache = (int32_t *)s->value_cache;
    int32_t *xs = (int32_t *)s->xq.s, *hs = (int32_t *)s->hq.s;
    int8_t *xq = s->xq.q, *hq = s->hq.q;

    memcpy(x, (const int32_t *)w->token_embedding_table + token * dim, dim * sizeof(int32_t));

    for (int i = 0; i < dim; i += 2) {
        int32_t sn, cs;
        fx_sincos((int64_t)pos * m->fx_freq_q30[i / 2], &sn, &cs);
        rope[i] = cs;
        rope[i + 1] = sn;
    }

    for (int l = 0; l < p->n_layers; l++) {
        rmsnorm_fx(xb, x, (const int32_t *)w->rms_att_weight + l * dim, dim);

        quantize_fx(xq, xs, xb, dim, gs);
        matmul_fx(q, xq, xs, w->wq + l, dim, dim, gs);
        matmul_fx(k, xq, xs, w->wk + l, dim, kv_dim, gs);
        matmul_fx(v, xq, xs, w->wv + l, dim, kv_dim, gs);

        for (int i = 0; i < dim; i += 2) {
            int64_t fcr = rope[i], fci = rope[i + 1];
            int rotn = i < kv_dim ? 2 : 1;
            for (int r = 0; r < rotn; r++) {
                int32_t *vec = r == 0 ? q : k;
                int64_t v0 = vec[i], v1 = vec[i + 1];
                vec[i] = (int32_t)((v0 * fcr - v1 * fci) >> 16);
                vec[i + 1] = (int32_t)((v0 * fci + v1 * fcr) >> 16);
            }
        }

        int loff = l * p->seq_len * kv_dim;
        memcpy(key_cache + loff + pos * kv_dim, k, kv_dim * sizeof(int32_t));
        memcpy(value_cache + loff + pos * kv_dim, v, kv_dim * sizeof(int32_t));

        for (int h = 0; h < p->n_heads; h++) {
            const int32_t *qh = q + h * head_size;
            int32_t *atth = att + h * p->seq_len;
            for (int t = 0; t <= pos; t++) {
                const int32_t *kh = key_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                int64_t score = 0;
                for (int i = 0; i < head_size; i++)
                    score += (int64_t)qh[i] * kh[i]; /* Q32 */
                atth[t] = (int32_t)(((score >> 16) * m->fx_inv_sqrt_hs) >> 16);
            }

            softmax_fx(atth, pos + 1);

            int32_t *xbh = xb + h * head_size;
            memset(xbh, 0, head_size * sizeof(int32_t));
            for (int t = 0; t <= pos; t++) {
                const int32_t *vh = value_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                int64_t a = atth[t];
                for (int i = 0; i < head_size; i++)
                    xbh[i] += (int32_t)((a * vh[i]) >> 16);
            }
        }

        quantize_fx(xq, xs, xb, dim, gs);
        matmul_fx(xb2, xq, xs, w->wo + l, dim, dim, gs);
        for (int i = 0; i < dim; i++)
            x[i] += xb2[i];

        rmsnorm_fx(xb, x, (const int32_t *)w->rms_ffn_weight + l * dim, dim);

        quantize_fx(xq, xs, xb, dim, gs);
        matmul_fx(hb, xq, xs, w->w1 + l, dim, hidden_dim, gs);
        matmul_fx(hb2, xq, xs, w->w3 + l, dim, hidden_dim, gs);

        fxk_silu_mul(hb, hb2, hidden_dim); /* SwiGLU */

        quantize_fx(hq, hs, hb, hidden_dim, gs);
        matmul_fx(xb, hq, hs, w->w2 + l, hidden_dim, dim, gs);
        for (int i = 0; i < dim; i++)
            x[i] += xb[i];
    }

    rmsnorm_fx(x, x, (const int32_t *)w->rms_final_weight, dim);

    quantize_fx(xq, xs, x, dim, gs);
    matmul_fx(logits, xq, xs, w->wcls, dim, p->vocab_size, gs);
    return logits;
}

/* ---- sampler: integers only -------------------------------------------- */

int fx_sample(llama2 *m, int32_t *logits)
{
    return fxk_sample(logits, m->config.vocab_size, m->fx_inv_temp, m->fx_topp, &m->rng_state,
                      (fxk_prob *)m->probindex);
}
