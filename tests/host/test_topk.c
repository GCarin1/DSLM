#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "topk.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

#define N 512

/* Reference: double softmax and a full sort of the indices. */
static void reference(const float *l, int n, int *order, double *p)
{
    double max = l[0], sum = 0;
    for (int i = 1; i < n; i++)
        max = l[i] > max ? l[i] : max;
    for (int i = 0; i < n; i++) {
        p[i] = exp(l[i] - max);
        sum += p[i];
        order[i] = i;
    }
    for (int i = 0; i < n; i++)
        p[i] /= sum;
    for (int i = 0; i < n; i++) /* selection sort, descending */
        for (int j = i + 1; j < n; j++)
            if (p[order[j]] > p[order[i]]) {
                int t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
}

static unsigned rng = 12345;
static float frand(void)
{
    rng = rng * 1103515245u + 12345u;
    return (float)((rng >> 8) & 0xFFFF) / 65536.0f;
}

static void test_matches_reference_on_random_logits(void)
{
    static float lf[N];
    static int32_t lq[N];
    static int order[N];
    static double p[N];
    for (int trial = 0; trial < 50; trial++) {
        float spread = 2.0f + (float)(trial % 5) * 3.0f;
        for (int i = 0; i < N; i++) {
            lf[i] = (frand() - 0.5f) * spread;
            lq[i] = (int32_t)lrintf(lf[i] * 65536.0f);
        }
        reference(lf, N, order, p);
        int ids_f[5], pm_f[5], ids_q[5], pm_q[5];
        CHECK(topk_f32(lf, N, 5, ids_f, pm_f) == 5);
        CHECK(topk_q16(lq, N, 5, ids_q, pm_q) == 5);
        for (int i = 0; i < 5; i++) {
            CHECK(ids_f[i] == order[i] || p[ids_f[i]] == p[order[i]]); /* equal logits may swap */
            CHECK(abs(pm_f[i] - (int)(p[order[i]] * 1000)) <= 1);
            /* fixed point: the same ranking unless two values are within rounding */
            CHECK(ids_q[i] == order[i] || fabs(p[ids_q[i]] - p[order[i]]) < 1e-4);
            CHECK(abs(pm_q[i] - (int)(p[order[i]] * 1000)) <= 3);
            if (i)
                CHECK(pm_q[i] <= pm_q[i - 1] && pm_f[i] <= pm_f[i - 1]);
        }
    }
}

static void test_peaked_and_small_inputs(void)
{
    float lf[4] = {0.0f, 10.0f, 0.0f, -5.0f};
    int32_t lq[4] = {0, 10 * 65536, 0, -5 * 65536};
    int ids[8], pm[8];
    CHECK(topk_q16(lq, 4, 8, ids, pm) == 4); /* k > n */
    CHECK(ids[0] == 1 && pm[0] >= 999);
    CHECK(topk_f32(lf, 4, 2, ids, pm) == 2);
    CHECK(ids[0] == 1 && pm[0] >= 999 && pm[1] == 0);
    CHECK(topk_q16(lq, 0, 5, ids, pm) == 0);
    CHECK(topk_q16(lq, 4, 0, ids, pm) == 0);
    CHECK(topk_q16(lq, 4, 20, ids, pm) == 4); /* clamped to TOPK_MAX, then n */
    int32_t flat[3] = {7, 7, 7};
    CHECK(topk_q16(flat, 3, 3, ids, pm) == 3);
    CHECK(pm[0] == 333 && pm[1] == 333 && pm[2] == 333);
}

int main(void)
{
    test_matches_reference_on_random_logits();
    test_peaked_and_small_inputs();
    puts(failures ? "FAILED" : "all topk tests passed");
    return failures ? 1 : 0;
}
