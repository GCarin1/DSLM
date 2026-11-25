/* Shared integer kernels (see fxkern.h). ARM mode on the DS; the matmul runs from ITCM. */
#include "fxkern.h"

#include <stdlib.h>
#include <string.h>

#include "fxmath.h"

#if defined(__NDS__)
#include <nds/ndstypes.h>
#define FX_ITCM ITCM_CODE
#else
#define FX_ITCM
#endif

/* ---- load time ---------------------------------------------------------- */

/* Round half away from zero; v * 2^bits is exact in double. */
int32_t fxk_to_fixed(float v, int bits)
{
    double d = (double)v * (double)(1LL << bits);
    d = d >= 0 ? d + 0.5 : d - 0.5;
    if (d > 2147483647.0)
        d = 2147483647.0;
    if (d < -2147483648.0)
        d = -2147483648.0;
    return (int32_t)d;
}

void fxk_floats_to_fixed(void *p, size_t n, int bits)
{
    unsigned char *b = p;
    for (size_t i = 0; i < n; i++) {
        float v;
        memcpy(&v, b + 4 * i, 4);
        int32_t r = fxk_to_fixed(v, bits);
        memcpy(b + 4 * i, &r, 4);
    }
}

/* ---- per token ---------------------------------------------------------- */

void fxk_rmsnorm(int32_t *o, const int32_t *x, const int32_t *weight, int size, int64_t eps_q32)
{
    int64_t ss = 0;
    for (int j = 0; j < size; j++)
        ss += (int64_t)x[j] * x[j]; /* Q32 */
    ss /= size;
    ss += eps_q32;
    uint32_t r = fx_isqrt64((uint64_t)ss); /* sqrt, Q16 */
    if (r == 0)
        r = 1;
    int64_t inv = ((int64_t)1 << 32) / r; /* 1 / sqrt, Q16 */
    for (int j = 0; j < size; j++)
        o[j] = (int32_t)(((int64_t)weight[j] * ((inv * x[j]) >> 16)) >> 16);
}

/* runq.c quantize(): q = round(127 * x / max|x|) per group; scale = max / 127 in Q24. */
void fxk_quantize(int8_t *q, int32_t *xs, const int32_t *x, int n, int gs)
{
    for (int g = 0; g < n / gs; g++) {
        const int32_t *xg = x + g * gs;
        uint32_t mx = 0;
        for (int i = 0; i < gs; i++) {
            uint32_t a = xg[i] < 0 ? (uint32_t)(-(int64_t)xg[i]) : (uint32_t)xg[i];
            if (a > mx)
                mx = a;
        }
        xs[g] = (int32_t)(((int64_t)mx << 8) / 127);
        if (mx == 0) {
            memset(q + g * gs, 0, (size_t)gs);
            continue;
        }
        int64_t rcp = ((int64_t)127 << 32) / mx;
        for (int i = 0; i < gs; i++)
            q[g * gs + i] = (int8_t)(((int64_t)xg[i] * rcp + ((int64_t)1 << 31)) >> 32);
    }
}

/* runq.c matmul(): int8 dot products per group, scaled by w (Q30) and x (Q24). */
FX_ITCM void fxk_matmul(int32_t *xout, const int8_t *xq, const int32_t *xs, const int8_t *wq, const int32_t *ws,
                        int n, int d, int gs)
{
    for (int i = 0; i < d; i++) {
        const int8_t *wr = wq + i * n;
        const int32_t *wsr = ws + (i * n) / gs;
        int64_t val = 0;
        for (int j = 0, g = 0; j <= n - gs; j += gs, g++) {
            int32_t acc = 0;
            for (int k = 0; k < gs; k++)
                acc += xq[j + k] * wr[j + k];
            int64_t t = ((int64_t)acc * wsr[g]) >> 14; /* Q16 */
            val += t * xs[g];                          /* Q40 */
        }
        xout[i] = (int32_t)(val >> 24);
    }
}

void fxk_softmax(int32_t *x, int size)
{
    int32_t max_val = x[0];
    for (int i = 1; i < size; i++)
        if (x[i] > max_val)
            max_val = x[i];
    int64_t sum = 0;
    for (int i = 0; i < size; i++) {
        x[i] = fx_exp_neg(x[i] - max_val);
        sum += x[i];
    }
    int64_t rcp = ((int64_t)1 << 32) / sum; /* sum >= 1.0 because the max contributes 1 */
    for (int i = 0; i < size; i++)
        x[i] = (int32_t)((x[i] * rcp) >> 16);
}

/* sigmoid with exp only of non-positive values */
int32_t fxk_sigmoid(int32_t x)
{
    int32_t ax = x < 0 ? -x : x;
    int64_t e = fx_exp_neg(-ax);
    int64_t den = FX_ONE + e;
    return (int32_t)(x >= 0 ? ((int64_t)FX_ONE << 16) / den : (e << 16) / den);
}

/* SwiGLU: silu(x) = x * sigmoid(x), then times the up projection */
void fxk_silu_mul(int32_t *h, const int32_t *h2, int n)
{
    for (int i = 0; i < n; i++) {
        int32_t val = h[i];
        int64_t out = ((int64_t)val * fxk_sigmoid(val)) >> 16;
        h[i] = (int32_t)((out * h2[i]) >> 16);
    }
}

/* Descending probability; ties by index so every qsort gives the same order. */
static int fxk_compare_prob(const void *a, const void *b)
{
    const fxk_prob *x = a, *y = b;
    if (x->prob != y->prob)
        return x->prob > y->prob ? -1 : 1;
    return x->index - y->index;
}

int fxk_sample(int32_t *logits, int n, int32_t inv_temp, int32_t topp, unsigned long long *rng, fxk_prob *cand)
{
    int max_i = 0;
    for (int i = 1; i < n; i++)
        if (logits[i] > logits[max_i])
            max_i = i;
    if (inv_temp == 0)
        return max_i;

    /* softmax(logits / temperature), unnormalised: p_i = exp((l_i - max) / T) */
    int32_t max_val = logits[max_i];
    int64_t sum = 0;
    for (int i = 0; i < n; i++) {
        int64_t z = ((int64_t)(logits[i] - max_val) * inv_temp) >> 16;
        logits[i] = fx_exp_neg(z < -(1 << 30) ? -(1 << 30) : (int32_t)z);
        sum += logits[i];
    }
    int64_t coin = llama2_rand_u32(rng) >> 8; /* [0, 2^24), like random_f32() */

    if (topp <= 0 || topp >= FX_ONE) {
        int64_t target = (coin * sum) >> 24, cdf = 0;
        for (int i = 0; i < n; i++) {
            cdf += logits[i];
            if (target < cdf)
                return i;
        }
        return n - 1;
    }

    /* top-p: keep p_i / sum >= (1 - topp) / (n - 1), sort, cut where the sum passes topp */
    int n0 = 0;
    for (int i = 0; i < n; i++) {
        if ((int64_t)logits[i] * (n - 1) * FX_ONE >= (int64_t)(FX_ONE - topp) * sum) {
            cand[n0].prob = logits[i];
            cand[n0].index = i;
            n0++;
        }
    }
    qsort(cand, n0, sizeof(fxk_prob), fxk_compare_prob);
    int64_t cum = 0;
    int last = n0 - 1;
    for (int i = 0; i < n0; i++) {
        cum += cand[i].prob;
        if (cum * FX_ONE > (int64_t)topp * sum) {
            last = i;
            break;
        }
    }
    int64_t target = (coin * cum) >> 24, cdf = 0;
    for (int i = 0; i <= last; i++) {
        cdf += cand[i].prob;
        if (target < cdf)
            return cand[i].index;
    }
    return cand[last].index;
}
