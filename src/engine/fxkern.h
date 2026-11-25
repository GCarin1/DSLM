/*
 * Integer kernels shared by the fixed-point engines (llama2_fixed, mla_fixed).
 * Q16 activations, int8 groups with Q24 activation scales and Q30 weight
 * scales; integers only after load, so DS and host builds agree bit for bit.
 */
#ifndef DSLM_FXKERN_H
#define DSLM_FXKERN_H

#include <stddef.h>
#include <stdint.h>

#define FXK_EPS_1E5_Q32 42950 /* 1e-5 in Q32 */
#define FXK_EPS_1E6_Q32 4295  /* 1e-6 in Q32 */

/* ---- load time (float allowed) ---- */
int32_t fxk_to_fixed(float v, int bits);
void fxk_floats_to_fixed(void *p, size_t n, int bits); /* in place */

/* ---- per token: integers only ---- */
void fxk_rmsnorm(int32_t *o, const int32_t *x, const int32_t *weight, int size, int64_t eps_q32);
void fxk_quantize(int8_t *q, int32_t *xs, const int32_t *x, int n, int gs);
void fxk_matmul(int32_t *xout, const int8_t *xq, const int32_t *xs, const int8_t *wq, const int32_t *ws,
                int n, int d, int gs);
void fxk_softmax(int32_t *x, int size);
int32_t fxk_sigmoid(int32_t x);                      /* Q16 in, Q16 out */
void fxk_silu_mul(int32_t *h, const int32_t *h2, int n); /* h = silu(h) * h2 */

/* Next token from Q16 logits; inv_temp 0 means argmax. cand holds n (prob, index) pairs. */
typedef struct {
    int32_t prob;
    int index;
} fxk_prob;
int fxk_sample(int32_t *logits, int n, int32_t inv_temp, int32_t topp, unsigned long long *rng, fxk_prob *cand);

/* xorshift generator shared with run.c's sampler (defined in llama2.c) */
unsigned int llama2_rand_u32(unsigned long long *state);

#endif
