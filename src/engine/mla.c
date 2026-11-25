/*
 * fp32 reference forward pass for LittleKedi (see mla.h). Mirrors
 * tools/m8/littlekedi_ref.py, the numpy oracle checked against llama.cpp:
 * absorbed MLA (queries projected into the 32-wide latent), interleaved RoPE,
 * sigmoid-gated top-k experts normalised to sum 1, plus one shared expert.
 */
#include "mla.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

mla_status mla_read_config(const void *header, size_t n, mla_config *c)
{
    const unsigned char *h = header;
    int32_t v[15];
    float f[3];
    if (n < MLA_HEADER_BYTES || memcmp(h, "MLA1", 4) != 0)
        return MLA_BAD_HEADER;
    memcpy(v, h + 4, sizeof v);
    memcpy(f, h + 4 + sizeof v, sizeof f);
    if (v[0] != 1)
        return MLA_BAD_HEADER;
    c->dim = v[1];
    c->n_layers = v[2];
    c->n_dense = v[3];
    c->n_heads = v[4];
    c->nope = v[5];
    c->rope = v[6];
    c->vd = v[7];
    c->kv = v[8];
    c->dense_hidden = v[9];
    c->n_experts = v[10];
    c->n_active = v[11];
    c->moe_hidden = v[12];
    c->vocab = v[13];
    c->max_seq = v[14];
    c->eps = f[0];
    c->theta = f[1];
    c->route_scale = f[2];
    if (c->dim <= 0 || c->n_layers <= 0 || c->n_heads <= 0 || c->rope % 2 || c->n_active > c->n_experts ||
        c->n_dense > c->n_layers || c->vocab <= 0 || c->max_seq <= 0)
        return MLA_BAD_HEADER;
    return MLA_OK;
}

static size_t layer_floats(const mla_config *c, int dense)
{
    size_t d = c->dim, qd = (size_t)c->n_heads * (c->nope + c->rope);
    size_t n = d + qd * d + (size_t)(c->kv + c->rope) * d + c->kv +
               (size_t)c->n_heads * (c->nope + c->vd) * c->kv + d * c->n_heads * c->vd + d;
    if (dense)
        return n + 3 * (size_t)c->dense_hidden * d;
    size_t mh = c->moe_hidden;
    return n + (size_t)c->n_experts * d + c->n_experts + (size_t)c->n_experts * 3 * mh * d + 3 * mh * d;
}

size_t mla_file_bytes(const mla_config *c)
{
    size_t n = 0;
    for (int l = 0; l < c->n_layers; l++)
        n += layer_floats(c, l < c->n_dense);
    n += c->dim + (size_t)c->vocab * c->dim;
    return MLA_HEADER_BYTES + 4 * n;
}

mla_status mla_init(mla_model *m, const void *file, size_t size, int seq)
{
    memset(m, 0, sizeof *m);
    mla_status st = mla_read_config(file, size, &m->c);
    if (st != MLA_OK)
        return st;
    if (mla_file_bytes(&m->c) != size)
        return MLA_BAD_SIZE;
    mla_config *c = &m->c;
    m->seq = seq > 0 && seq < c->max_seq ? seq : c->max_seq;

    size_t qd = (size_t)c->n_heads * (c->nope + c->rope);
    size_t hidden = c->dense_hidden > c->moe_hidden ? c->dense_hidden : c->moe_hidden;
    size_t floats = 3 * (size_t)c->dim + qd + (c->kv + c->rope) + c->kv + c->kv + (size_t)c->n_heads * c->vd +
                    m->seq + 2 * hidden + c->dim + c->vocab + c->n_experts +
                    (size_t)c->n_layers * m->seq * (c->kv + c->rope) + c->rope;
    unsigned char *arena = malloc(floats * sizeof(float) + (size_t)c->n_layers * sizeof(mla_layer));
    if (!arena)
        return MLA_NO_MEMORY;
    m->arena = arena;
    m->layers = (mla_layer *)arena;
    float *f = (float *)(arena + (size_t)c->n_layers * sizeof(mla_layer));
    memset(f, 0, floats * sizeof(float));
    m->x = f;        f += c->dim;
    m->xb = f;       f += c->dim;
    m->xb2 = f;      f += c->dim;
    m->q = f;        f += qd;
    m->kvbuf = f;    f += c->kv + c->rope;
    m->qlat = f;     f += c->kv;
    m->ctx = f;      f += c->kv;
    m->heads = f;    f += (size_t)c->n_heads * c->vd;
    m->att = f;      f += m->seq;
    m->hb = f;       f += hidden;
    m->hb2 = f;      f += hidden;
    m->ex = f;       f += c->dim;
    m->logits = f;   f += c->vocab;
    m->scores = f;   f += c->n_experts;
    m->lat_cache = f; f += (size_t)c->n_layers * m->seq * c->kv;
    m->kpe_cache = f; f += (size_t)c->n_layers * m->seq * c->rope;
    m->rope_cs = f;

    const float *p = (const float *)((const unsigned char *)file + MLA_HEADER_BYTES);
    size_t d = c->dim, mh = c->moe_hidden;
    for (int l = 0; l < c->n_layers; l++) {
        mla_layer *L = &m->layers[l];
        L->attn_norm = p; p += d;
        L->wq = p;        p += qd * d;
        L->wkv_a = p;     p += (size_t)(c->kv + c->rope) * d;
        L->kv_norm = p;   p += c->kv;
        L->wkv_b = p;     p += (size_t)c->n_heads * (c->nope + c->vd) * c->kv;
        L->wo = p;        p += d * c->n_heads * c->vd;
        L->ffn_norm = p;  p += d;
        if (l < c->n_dense) {
            L->w1 = p; p += (size_t)c->dense_hidden * d;
            L->w2 = p; p += d * c->dense_hidden;
            L->w3 = p; p += (size_t)c->dense_hidden * d;
        } else {
            L->gate_w = p;  p += (size_t)c->n_experts * d;
            L->gate_b = p;  p += c->n_experts;
            L->experts = p; p += (size_t)c->n_experts * 3 * mh * d;
            L->sw1 = p;     p += mh * d;
            L->sw2 = p;     p += d * mh;
            L->sw3 = p;     p += mh * d;
        }
    }
    m->norm = p;
    p += d;
    m->embed = p;
    return MLA_OK;
}

void mla_free(mla_model *m)
{
    free(m->arena);
    m->arena = NULL;
}

/* ---- blocks ------------------------------------------------------------- */

static void rmsnorm(float *o, const float *x, const float *w, int n, float eps)
{
    float ss = 0.0f;
    for (int i = 0; i < n; i++)
        ss += x[i] * x[i];
    float inv = 1.0f / sqrtf(ss / n + eps);
    for (int i = 0; i < n; i++)
        o[i] = w[i] * (x[i] * inv);
}

static void matmul(float *out, const float *x, const float *w, int n, int d)
{
    for (int i = 0; i < d; i++) {
        float v = 0.0f;
        const float *row = w + (size_t)i * n;
        for (int j = 0; j < n; j++)
            v += row[j] * x[j];
        out[i] = v;
    }
}

static float silu(float v)
{
    return v / (1.0f + expf(-v));
}

/* interleaved pairs (2i, 2i+1), angle pos / theta^(2i / rope) */
static void rope_rotate(float *v, const float *cs, int rope)
{
    for (int i = 0; i < rope / 2; i++) {
        float c = cs[2 * i], s = cs[2 * i + 1];
        float a = v[2 * i], b = v[2 * i + 1];
        v[2 * i] = a * c - b * s;
        v[2 * i + 1] = a * s + b * c;
    }
}

/* SwiGLU: out = down(silu(gate x) * up x), all [out][in] */
static void swiglu(float *out, const float *x, const float *gate, const float *up, const float *down,
                   float *h1, float *h2, int dim, int hidden)
{
    matmul(h1, x, gate, dim, hidden);
    matmul(h2, x, up, dim, hidden);
    for (int i = 0; i < hidden; i++)
        h1[i] = silu(h1[i]) * h2[i];
    matmul(out, h1, down, hidden, dim);
}

float *mla_forward(mla_model *m, int token, int pos)
{
    mla_config *c = &m->c;
    int dim = c->dim, hd = c->nope + c->rope, kv = c->kv, rope = c->rope;
    float scale = 1.0f / sqrtf((float)hd);
    float *x = m->x;

    memcpy(x, m->embed + (size_t)token * dim, dim * sizeof(float));
    for (int i = 0; i < rope / 2; i++) {
        double ang = pos / pow((double)c->theta, (2.0 * i) / rope);
        m->rope_cs[2 * i] = (float)cos(ang);
        m->rope_cs[2 * i + 1] = (float)sin(ang);
    }

    for (int l = 0; l < c->n_layers; l++) {
        mla_layer *L = &m->layers[l];
        float *lat_l = m->lat_cache + (size_t)l * m->seq * kv;
        float *kpe_l = m->kpe_cache + (size_t)l * m->seq * rope;

        rmsnorm(m->xb, x, L->attn_norm, dim, c->eps);
        matmul(m->q, m->xb, L->wq, dim, c->n_heads * hd);
        matmul(m->kvbuf, m->xb, L->wkv_a, dim, kv + rope);
        rmsnorm(lat_l + (size_t)pos * kv, m->kvbuf, L->kv_norm, kv, c->eps);
        memcpy(kpe_l + (size_t)pos * rope, m->kvbuf + kv, rope * sizeof(float));
        rope_rotate(kpe_l + (size_t)pos * rope, m->rope_cs, rope);

        for (int h = 0; h < c->n_heads; h++) {
            float *qh = m->q + (size_t)h * hd; /* [nope | rope] */
            rope_rotate(qh + c->nope, m->rope_cs, rope);
            const float *wk = L->wkv_b + (size_t)h * (c->nope + c->vd) * kv; /* rows 0..nope-1 */
            const float *wv = wk + (size_t)c->nope * kv;                     /* rows nope..nope+vd-1 */
            for (int k = 0; k < kv; k++) {
                float s = 0.0f;
                for (int i = 0; i < c->nope; i++)
                    s += qh[i] * wk[(size_t)i * kv + k];
                m->qlat[k] = s;
            }
            float mx = -INFINITY;
            for (int t = 0; t <= pos; t++) {
                const float *lt = lat_l + (size_t)t * kv, *kt = kpe_l + (size_t)t * rope;
                float s = 0.0f, r = 0.0f;
                for (int k = 0; k < kv; k++)
                    s += m->qlat[k] * lt[k];
                for (int i = 0; i < rope; i++)
                    r += qh[c->nope + i] * kt[i];
                m->att[t] = (s + r) * scale;
                if (m->att[t] > mx)
                    mx = m->att[t];
            }
            float sum = 0.0f;
            for (int t = 0; t <= pos; t++) {
                m->att[t] = expf(m->att[t] - mx);
                sum += m->att[t];
            }
            for (int k = 0; k < kv; k++)
                m->ctx[k] = 0.0f;
            for (int t = 0; t <= pos; t++) {
                float a = m->att[t] / sum;
                const float *lt = lat_l + (size_t)t * kv;
                for (int k = 0; k < kv; k++)
                    m->ctx[k] += a * lt[k];
            }
            matmul(m->heads + (size_t)h * c->vd, m->ctx, wv, kv, c->vd);
        }
        matmul(m->xb2, m->heads, L->wo, c->n_heads * c->vd, dim);
        for (int i = 0; i < dim; i++)
            x[i] += m->xb2[i];

        rmsnorm(m->xb, x, L->ffn_norm, dim, c->eps);
        if (l < c->n_dense) {
            swiglu(m->xb2, m->xb, L->w1, L->w3, L->w2, m->hb, m->hb2, dim, c->dense_hidden);
        } else {
            int mh = c->moe_hidden, ne = c->n_experts;
            matmul(m->scores, m->xb, L->gate_w, dim, ne);
            for (int e = 0; e < ne; e++)
                m->scores[e] = 1.0f / (1.0f + expf(-m->scores[e]));
            /* top-k by score + bias; ties keep the lower index (numpy stable argsort) */
            int pick[16];
            float total = 0.0f;
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
            swiglu(m->xb2, m->xb, L->sw1, L->sw3, L->sw2, m->hb, m->hb2, dim, mh); /* shared */
            for (int k = 0; k < c->n_active; k++) {
                const float *e = L->experts + (size_t)pick[k] * 3 * mh * dim; /* gate, up, down */
                swiglu(m->ex, m->xb, e, e + (size_t)mh * dim, e + 2 * (size_t)mh * dim, m->hb, m->hb2, dim, mh);
                float wgt = m->scores[pick[k]] / total * c->route_scale;
                for (int i = 0; i < dim; i++)
                    m->xb2[i] += wgt * m->ex[i];
            }
        }
        for (int i = 0; i < dim; i++)
            x[i] += m->xb2[i];
    }

    rmsnorm(x, x, m->norm, dim, c->eps);
    matmul(m->logits, x, m->embed, dim, c->vocab);
    return m->logits;
}
