/*
 * Fixed-point LittleKedi engine (MLQ1 files: int8 matrices with group-wise
 * scales, fp32 vectors). Integers only after load, through fxkern; DS and
 * host builds agree bit for bit. The fp32 reference is mla.c.
 */
#ifndef DSLM_MLQ_H
#define DSLM_MLQ_H

#include <stddef.h>
#include <stdint.h>

#include "fxkern.h"
#include "mla.h"

typedef struct {
    const int8_t *q;
    const int32_t *s; /* Q30 after load (fp32 in the file) */
} mlq_tensor;

typedef struct {
    const int32_t *attn_norm, *kv_norm, *ffn_norm, *gate_b; /* Q16 after load */
    const int32_t *gate_w;          /* MoE router [n_experts][dim], Q16 (fp32 in the file) */
    mlq_tensor wq, wkv_a, wkv_b, wo;
    mlq_tensor w1, w2, w3;          /* dense */
    mlq_tensor sw1, sw2, sw3;       /* MoE shared expert */
    mlq_tensor *experts;            /* [n_experts][gate, up, down] */
} mlq_layer;

typedef struct {
    mla_config c;
    int gs, seq;
    mlq_layer *layers;
    const int32_t *norm;
    mlq_tensor embed;
    int32_t route_scale;   /* Q16 */
    int32_t inv_sqrt_hd;   /* Q16 */
    int32_t *freq_q30;
    /* state, Q16 */
    int32_t *x, *xb, *xb2, *q, *kvb, *qlat, *ctx, *heads, *att, *hb, *hb2, *ex, *logits, *scores, *rope_cs;
    int32_t *xs, *hs, *cs; /* activation scales (Q24) */
    int8_t *xq, *hq, *cq;  /* quantized activations */
    int32_t *lat_cache, *kpe_cache;
    fxk_prob *cand;
    uint8_t *seen;         /* repeat-penalty bitset over the vocabulary */
    void *arena;
} mlq_model;

mla_status mlq_read_config(const void *header, size_t n, mla_config *c, int *group_size);
size_t mlq_file_bytes(const mla_config *c, int group_size);
size_t mlq_runtime_bytes(const mla_config *c, int seq);

/* The file buffer is converted in place (scales and vectors become fixed point). */
mla_status mlq_init(mlq_model *m, void *file, size_t size, int seq);
void mlq_free(mlq_model *m);

/* One position; integers only. Returns Q16 logits. */
int32_t *mlq_forward(mlq_model *m, int token, int pos);

/*
 * Next token: repeat penalty over recent[0..n_recent) (penalty Q16, 65536 = off),
 * then temperature / top-p like fxk_sample (inv_temp 0 = argmax).
 */
int mlq_sample(mlq_model *m, int32_t *logits, const int *recent, int n_recent, int32_t penalty_q16,
               int32_t inv_temp_q16, int32_t topp_q16, unsigned long long *rng);

#endif
