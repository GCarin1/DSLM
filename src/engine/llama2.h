/*
 * fp32 Llama 2 inference: a port of run.c from llama2.c (MIT, Andrej
 * Karpathy) that works on buffers already in memory. Pure C, no libnds, no
 * file I/O, so the same code runs on the DS and in the host parity test.
 */
#ifndef DSLM_LLAMA2_H
#define DSLM_LLAMA2_H

#include <stddef.h>
#include <stdint.h>

#define LLAMA2_HEADER_BYTES 28
#define LLAMA2_MAX_PROMPT 256 /* bytes; the prompt buffers are sized for this */
#define LLAMA2_BOS 1

typedef struct {
    int dim;
    int hidden_dim;
    int n_layers;
    int n_heads;
    int n_kv_heads;
    int vocab_size;
    int seq_len;
} llama2_config;

typedef struct {
    float *token_embedding_table;
    float *rms_att_weight;
    float *rms_ffn_weight;
    float *wq, *wk, *wv, *wo;
    float *w1, *w2, *w3;
    float *rms_final_weight;
    float *wcls;
} llama2_weights;

typedef struct {
    float *x, *xb, *xb2, *hb, *hb2, *q, *k, *v, *att, *logits;
    float *key_cache, *value_cache;
} llama2_state;

typedef struct {
    char *str;
    int id;
} llama2_token_index;

typedef struct {
    float prob;
    int index;
} llama2_prob_index;

typedef struct {
    llama2_config config;
    llama2_weights weights;
    llama2_state state;

    /* tokenizer */
    char **vocab;
    float *vocab_scores;
    llama2_token_index *sorted_vocab;
    unsigned int max_token_length;
    unsigned char byte_pieces[512];

    /* sampler */
    float temperature;
    float topp;
    unsigned long long rng_state;
    llama2_prob_index *probindex;

    /* scratch, allocated once so generation never allocates */
    char *str_buffer;
    int *prompt_tokens;

    void *arena; /* single allocation behind every buffer above */
} llama2;

typedef enum {
    LLAMA2_OK = 0,
    LLAMA2_BAD_HEADER,
    LLAMA2_BAD_SIZE,
    LLAMA2_BAD_TOKENIZER,
    LLAMA2_NO_MEMORY,
    LLAMA2_PROMPT_TOO_LONG
} llama2_status;

/* Parse the 28-byte header; vocab_size is made positive. */
llama2_status llama2_read_config(const void *header, llama2_config *c, int *shared_weights);

/* Checkpoint size the header implies, in bytes. */
size_t llama2_checkpoint_bytes(const llama2_config *c, int shared_weights);

/* Bytes llama2_init allocates (run state, tokenizer, sampler, scratch). */
size_t llama2_runtime_bytes(const llama2_config *c, size_t tokenizer_bytes);

/*
 * Wire up a model whose checkpoint and tokenizer files are already in RAM.
 * The checkpoint buffer must stay alive and be 4-byte aligned; weights are
 * used in place. The tokenizer buffer may be freed after this returns.
 */
llama2_status llama2_init(llama2 *m, const void *checkpoint, size_t checkpoint_size,
                          const void *tokenizer, size_t tokenizer_size,
                          float temperature, float topp, unsigned long long seed);

void llama2_free(llama2 *m);

/*
 * Called once per token after BOS with its decoded text ("" when run.c's
 * safe_printf would print nothing). Return nonzero to stop generating; the
 * tokens produced so far are kept.
 */
typedef int (*llama2_token_fn)(int token, const char *piece, void *user);

/*
 * Generate like run.c's generate(): BOS + prompt, then sample until `steps`
 * positions, BOS, or the callback asks to stop. Writes the full sequence
 * (BOS first) to `out` when it is not NULL. Returns the number of tokens
 * written, or -1 for a prompt that does not fit LLAMA2_MAX_PROMPT. Never
 * allocates.
 */
int llama2_generate(llama2 *m, const char *prompt, int steps,
                    int *out, int out_cap, int *n_prompt_tokens,
                    llama2_token_fn on_token, void *user);

float *llama2_forward(llama2 *m, int token, int pos);
int llama2_sample(llama2 *m, float *logits);

#endif
