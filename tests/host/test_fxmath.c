/* Accuracy of the integer math against libm (host only). */
#include <math.h>
#include <stdio.h>

#include "fxmath.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static void test_exp_neg(void)
{
    double worst = 0;
    for (int32_t x = 0; x >= -40 * FX_ONE; x -= 97) {
        double got = fx_exp_neg(x) / 65536.0, want = exp(x / 65536.0);
        double err = fabs(got - want);
        if (err > worst)
            worst = err;
    }
    printf("exp_neg max abs error %.2e\n", worst);
    CHECK(worst < 3e-5); /* about 2 Q16 steps */
    CHECK(fx_exp_neg(0) == FX_ONE);
    CHECK(fx_exp_neg(-100 * FX_ONE) == 0);
}

static void test_exp2_q30(void)
{
    double worst = 0;
    /* RoPE needs 2^y down to 10000^-1 (y = -13.3); below that Q30 runs out of bits. */
    for (int32_t y = 0; y >= -14 * FX_ONE; y -= 131) {
        double want = exp2(y / 65536.0);
        double rel = fabs(fx_exp2_neg_q30(y) / 1073741824.0 - want) / want;
        if (rel > worst)
            worst = rel;
    }
    printf("exp2_neg_q30 max rel error %.2e\n", worst);
    CHECK(worst < 2e-5);
}

static void test_sincos(void)
{
    double worst = 0;
    for (double a = 0; a < 520.0; a += 0.0137) {
        int32_t s, c;
        fx_sincos((int64_t)llround(a * 1073741824.0), &s, &c);
        double es = fabs(s / 65536.0 - sin(a)), ec = fabs(c / 65536.0 - cos(a));
        if (es > worst)
            worst = es;
        if (ec > worst)
            worst = ec;
    }
    printf("sincos max abs error %.2e\n", worst);
    CHECK(worst < 2e-5);
}

static void test_isqrt(void)
{
    CHECK(fx_isqrt64(0) == 0);
    CHECK(fx_isqrt64(1) == 1);
    CHECK(fx_isqrt64(15) == 3);
    CHECK(fx_isqrt64(16) == 4);
    CHECK(fx_isqrt64(4294967296ull) == 65536);
    CHECK(fx_isqrt64(18446744073709551615ull) == 4294967295u);
    for (unsigned long long n = 1; n < 2000000; n = n * 3 + 1) {
        unsigned long long r = fx_isqrt64(n);
        CHECK(r * r <= n && (r + 1) * (r + 1) > n);
    }
}

int main(void)
{
    test_exp_neg();
    test_exp2_q30();
    test_sincos();
    test_isqrt();
    puts(failures ? "FAILED" : "all fxmath tests passed");
    return failures ? 1 : 0;
}
