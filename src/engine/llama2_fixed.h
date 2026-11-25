/* Fixed-point int8 engine, internal to src/engine (see llama2_fixed.arm.c). */
#ifndef DSLM_LLAMA2_FIXED_H
#define DSLM_LLAMA2_FIXED_H

#include "llama2.h"

/* Load time: convert scales and norms in place, build the Q16 embedding. */
void fx_prepare(llama2 *m);

/* Once per generation: sampling settings to fixed point. */
void fx_begin(llama2 *m);

/* One position; integer only. Returns the Q16 logits. */
int32_t *fx_forward(llama2 *m, int token, int pos);

/* Next token from Q16 logits; integer only. */
int fx_sample(llama2 *m, int32_t *logits);

#endif
