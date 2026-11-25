/*
 * DeepSeek-V3-style model (multi-head latent attention + mixture of experts),
 * fp32 reference for LittleKedi. Pure C on buffers in RAM, like llama2.c;
 * the file layout is written by tools/m8/convert_littlekedi.py.
 */
#ifndef DSLM_MLA_H
#define DSLM_MLA_H

#include <stddef.h>
#include <stdint.h>

#define MLA_HEADER_BYTES 128

typedef struct {
    int dim, n_layers, n_dense, n_heads, nope, rope, vd, kv;
    int dense_hidden, n_experts, n_active, moe_hidden, vocab, max_seq;
    float eps, theta, route_scale;
} mla_config;

typedef struct {
    const float *attn_norm, *wq, *wkv_a, *kv_norm, *wkv_b, *wo, *ffn_norm;
    const float *w1, *w2, *w3;                         /* dense layers */
    const float *gate_w, *gate_b, *experts, *sw1, *sw2, *sw3; /* MoE layers */
} mla_layer;

typedef struct {
    mla_config c;
    int seq;              /* positions the cache holds (<= max_seq) */
    mla_layer *layers;
    const float *norm, *embed;
    float *x, *xb, *xb2, *q, *kvbuf, *qlat, *ctx, *heads, *att, *hb, *hb2, *ex, *logits, *scores;
    float *lat_cache, *kpe_cache, *rope_cs;
    void *arena;
} mla_model;

typedef enum { MLA_OK = 0, MLA_BAD_HEADER, MLA_BAD_SIZE, MLA_NO_MEMORY } mla_status;

mla_status mla_read_config(const void *header, size_t n, mla_config *c);
size_t mla_file_bytes(const mla_config *c);

/* seq caps the context (the KV cache is sized for it); 0 means max_seq. */
mla_status mla_init(mla_model *m, const void *file, size_t size, int seq);
void mla_free(mla_model *m);

/* One position; returns vocab logits. */
float *mla_forward(mla_model *m, int token, int pos);

#endif
