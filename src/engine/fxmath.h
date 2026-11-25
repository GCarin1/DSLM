/*
 * Integer-only math for the fixed-point engine. Pure C with no libm and no
 * float, so the DS and the host build give bit-identical results.
 * Q16 = value * 2^16 in an int32; Q30 = value * 2^30.
 */
#ifndef DSLM_FXMATH_H
#define DSLM_FXMATH_H

#include <stdint.h>

#define FX_SHIFT 16
#define FX_ONE (1 << FX_SHIFT)

/* 2^y for y <= 0 (Q16 in), Q30 out. */
int32_t fx_exp2_neg_q30(int32_t y_q16);

/* e^x for x <= 0 (Q16 in), Q16 out. */
int32_t fx_exp_neg(int32_t x_q16);

/* sin and cos of a non-negative angle (Q30 radians), Q16 out. */
void fx_sincos(int64_t angle_q30, int32_t *sin_q16, int32_t *cos_q16);

/* floor(sqrt(n)) */
uint32_t fx_isqrt64(uint64_t n);

#endif
