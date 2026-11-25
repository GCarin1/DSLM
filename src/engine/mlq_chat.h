/*
 * Generation loop for the fixed-point LittleKedi engine: prompt (chat markers
 * included) -> tokens until <|end|>, <|endoftext|>, `steps` new tokens or the
 * callback asks to stop. Pure C; the host driver and the DS app share it, so a
 * DS run matches the host golden token for token.
 */
#ifndef DSLM_MLQ_CHAT_H
#define DSLM_MLQ_CHAT_H

#include <stdint.h>

#include "bpe.h"
#include "mlq.h"

#define MLQ_REPEAT_WINDOW 64       /* recent tokens the penalty looks at (llama.cpp's default) */
#define MLQ_REPEAT_PENALTY 1.15f   /* LittleKedi's recommended value (from its GGUF) */

typedef struct {
    int32_t inv_temp; /* Q16; 0 = argmax */
    int32_t topp;     /* Q16 */
    int32_t penalty;  /* Q16; 65536 = off */
    unsigned long long rng;
} mlq_sampling;

/* Load time: float settings to fixed point (temperature 0 = argmax). */
void mlq_sampling_init(mlq_sampling *s, float temperature, float topp, float penalty, unsigned long long seed);

/* Called for each new token with its raw bytes (not NUL-terminated); return nonzero to stop. */
typedef int (*mlq_token_fn)(int token, const uint8_t *piece, int len, void *user);

/*
 * Writes prompt + generated ids to out (cap entries) and the prompt length to
 * *n_prompt. Returns the total count, or -1 if the prompt does not encode into
 * the context. Never allocates.
 */
int mlq_generate(mlq_model *m, const bpe_tokenizer *t, const char *prompt, int steps, mlq_sampling *s, int *out,
                 int cap, int *n_prompt, mlq_token_fn on_token, void *user);

/* "<|user|>text<|end|><|assistant|>" into buf; returns its length, or -1 if cap is too small. */
int mlq_chat_prompt(char *buf, int cap, const char *text);

#endif
