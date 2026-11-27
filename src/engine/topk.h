/*
 * The k most likely next tokens and their probabilities, for showing what
 * the model "thinks" (it never feeds back into sampling). Softmax at
 * temperature 1 over the raw logits. Pure C, tested on the host.
 */
#ifndef DSLM_TOPK_H
#define DSLM_TOPK_H

#include <stdint.h>

#define TOPK_MAX 8

/*
 * Q16 logits (the int8 engine): writes up to k (<= TOPK_MAX) token ids, most
 * likely first, with their probabilities in permille (0-1000). Returns the
 * count written (min(k, n)).
 */
int topk_q16(const int32_t *logits, int n, int k, int *ids, int *permille);

/* The same for float logits (the fp32 reference engine). */
int topk_f32(const float *logits, int n, int k, int *ids, int *permille);

#endif
