/* Generation loop for LittleKedi; see mlq_chat.h. */
#include "mlq_chat.h"

#include <stdio.h>

void mlq_sampling_init(mlq_sampling *s, float temperature, float topp, float penalty, unsigned long long seed)
{
    s->inv_temp = temperature > 0.0f ? fxk_to_fixed(1.0f / temperature, 16) : 0;
    s->topp = fxk_to_fixed(topp, 16);
    s->penalty = fxk_to_fixed(penalty, 16);
    s->rng = seed ? seed : 1;
}

int mlq_generate(mlq_model *m, const bpe_tokenizer *t, const char *prompt, int steps, mlq_sampling *s, int *out,
                 int cap, int *n_prompt, mlq_token_fn on_token, void *user)
{
    int limit = cap < m->seq ? cap : m->seq;
    int n = bpe_encode(t, prompt, out, limit);
    if (n <= 0 || n >= limit)
        return -1;
    *n_prompt = n;
    int end = bpe_special(t, "<|end|>"), eot = bpe_special(t, "<|endoftext|>");
    int total = n;
    for (int pos = 0; pos < total && pos < limit - 1; pos++) {
        int32_t *logits = mlq_forward(m, out[pos], pos);
        if (pos < n - 1)
            continue;
        if (total - n >= steps)
            break;
        int from = total > MLQ_REPEAT_WINDOW ? total - MLQ_REPEAT_WINDOW : 0;
        int next = mlq_sample(m, logits, out + from, total - from, s->penalty, s->inv_temp, s->topp, &s->rng);
        if (next == end || next == eot)
            break;
        out[total++] = next;
        if (on_token) {
            int len;
            const uint8_t *piece = bpe_piece(t, next, &len);
            if (on_token(next, piece, len, user))
                break;
        }
    }
    return total;
}

int mlq_chat_prompt(char *buf, int cap, const char *text)
{
    int n = snprintf(buf, (size_t)cap, "<|user|>%s<|end|><|assistant|>", text);
    return n >= 0 && n < cap ? n : -1;
}
