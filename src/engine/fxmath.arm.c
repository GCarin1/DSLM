/* Integer-only math for the fixed-point engine (see fxmath.h). ARM mode on the DS. */
#include "fxmath.h"

#include "fxtables.h"

/* 2^(-u) for u >= 0 (Q16), Q30 out: 2^(-n-1) * 2^(g/65536) with g = 65536 - frac. */
static uint32_t exp2_neg_u(uint32_t u_q16)
{
    uint32_t n = u_q16 >> 16;
    if (n >= 31)
        return 0;
    uint32_t g = 65536u - (u_q16 & 0xFFFFu); /* 1..65536 */
    uint32_t idx = g >> 8, frac = g & 0xFFu;
    uint32_t t = FX_EXP2_Q30[idx];
    if (idx < 256)
        t += (uint32_t)(((uint64_t)(FX_EXP2_Q30[idx + 1] - t) * frac) >> 8);
    return t >> (n + 1);
}

int32_t fx_exp2_neg_q30(int32_t y_q16)
{
    if (y_q16 >= 0)
        return 1 << 30;
    return (int32_t)exp2_neg_u((uint32_t)(-(int64_t)y_q16));
}

int32_t fx_exp_neg(int32_t x_q16)
{
    if (x_q16 >= 0)
        return FX_ONE;
    uint64_t u = ((uint64_t)(-(int64_t)x_q16) * (uint64_t)FX_LOG2E_Q30) >> 30; /* Q16 */
    if (u >= (31u << 16))
        return 0;
    uint32_t r30 = exp2_neg_u((uint32_t)u);
    return (int32_t)((r30 + (1u << 13)) >> 14);
}

/* Taylor series on [0, pi/2) in Q30; error below 1e-7. */
static void sincos_quadrant(int64_t r, int64_t *s, int64_t *c)
{
    const int64_t one = (int64_t)1 << 30;
    int64_t x2 = (r * r) >> 30;
    int64_t t = one;
    t = one - ((x2 * t) >> 30) / 110;
    t = one - ((x2 * t) >> 30) / 72;
    t = one - ((x2 * t) >> 30) / 42;
    t = one - ((x2 * t) >> 30) / 20;
    t = one - ((x2 * t) >> 30) / 6;
    *s = (r * t) >> 30;

    t = one;
    t = one - ((x2 * t) >> 30) / 132;
    t = one - ((x2 * t) >> 30) / 90;
    t = one - ((x2 * t) >> 30) / 56;
    t = one - ((x2 * t) >> 30) / 30;
    t = one - ((x2 * t) >> 30) / 12;
    t = one - ((x2 * t) >> 30) / 2;
    *c = t;
}

static int32_t q30_to_q16(int64_t v)
{
    return (int32_t)((v + (1 << 13)) >> 14);
}

void fx_sincos(int64_t angle_q30, int32_t *sin_q16, int32_t *cos_q16)
{
    int64_t a = angle_q30 % FX_TWO_PI_Q30;
    if (a < 0)
        a += FX_TWO_PI_Q30;
    int quadrant = (int)(a / FX_HALF_PI_Q30);
    if (quadrant > 3)
        quadrant = 3;
    int64_t s, c;
    sincos_quadrant(a - quadrant * FX_HALF_PI_Q30, &s, &c);
    switch (quadrant) {
    case 0: *sin_q16 = q30_to_q16(s);  *cos_q16 = q30_to_q16(c);  break;
    case 1: *sin_q16 = q30_to_q16(c);  *cos_q16 = q30_to_q16(-s); break;
    case 2: *sin_q16 = q30_to_q16(-s); *cos_q16 = q30_to_q16(-c); break;
    default: *sin_q16 = q30_to_q16(-c); *cos_q16 = q30_to_q16(s); break;
    }
}

uint32_t fx_isqrt64(uint64_t n)
{
    uint64_t res = 0, bit = (uint64_t)1 << 62;
    while (bit > n)
        bit >>= 2;
    while (bit) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)res;
}
