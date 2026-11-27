#include "topk.h"

#include <math.h>

#include "fxmath.h"

/* Keep the k largest values seen so far, sorted descending (k is small). */
static void insert(int *ids, int64_t *vals, int *have, int k, int id, int64_t v)
{
    int at = *have;
    while (at > 0 && vals[at - 1] < v)
        at--;
    if (at >= k)
        return;
    int last = *have < k ? *have : k - 1;
    for (int i = last; i > at; i--) {
        ids[i] = ids[i - 1];
        vals[i] = vals[i - 1];
    }
    ids[at] = id;
    vals[at] = v;
    if (*have < k)
        (*have)++;
}

int topk_q16(const int32_t *logits, int n, int k, int *ids, int *permille)
{
    int64_t vals[TOPK_MAX];
    int have = 0;
    if (k > TOPK_MAX)
        k = TOPK_MAX;
    if (n <= 0 || k <= 0)
        return 0;
    int32_t max = logits[0];
    for (int i = 1; i < n; i++)
        if (logits[i] > max)
            max = logits[i];
    int64_t sum = 0;
    for (int i = 0; i < n; i++) {
        int32_t e = fx_exp_neg(logits[i] - max); /* Q16, 1.0 for the maximum */
        sum += e;
        insert(ids, vals, &have, k, i, e);
    }
    for (int i = 0; i < have; i++)
        permille[i] = sum > 0 ? (int)(vals[i] * 1000 / sum) : 0;
    return have;
}

int topk_f32(const float *logits, int n, int k, int *ids, int *permille)
{
    int64_t vals[TOPK_MAX];
    int have = 0;
    if (k > TOPK_MAX)
        k = TOPK_MAX;
    if (n <= 0 || k <= 0)
        return 0;
    float max = logits[0];
    for (int i = 1; i < n; i++)
        if (logits[i] > max)
            max = logits[i];
    double sum = 0;
    for (int i = 0; i < n; i++) {
        double e = exp((double)(logits[i] - max));
        sum += e;
        insert(ids, vals, &have, k, i, (int64_t)(e * 1e15)); /* e <= 1: fits, 1e-15 resolution */
    }
    for (int i = 0; i < have; i++)
        permille[i] = sum > 0 ? (int)((double)vals[i] / 1e12 / sum) : 0;
    return have;
}
