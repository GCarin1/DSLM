/*
 * fp32 Llama 2 inference: a port of run.c from llama2.c (MIT, Andrej
 * Karpathy) that works on buffers already in memory. Pure C, no libnds, no
 * file I/O, so the same code runs on the DS and in the host parity test.
 */
#ifndef DSLM_LLAMA2_H
#define DSLM_LLAMA2_H

#include <stddef.h>
#include <stdint.h>

#define LLAMA2_HEADER_BYTES 28    /* fp32 run.c checkpoint header */
#define LLAMA2_Q80_HEADER_BYTES 256 /* int8 runq.c v2 header ("ak42") */
#define LLAMA2_PROBE_BYTES 64     /* enough to recognise either format */
#define LLAMA2_MAX_PROMPT 256 /* bytes; the prompt buffers are sized for this */
#define LLAMA2_BOS 1

/* Which checkpoint layout a file uses (contracts/model-file-format). */
typedef struct {
    int quantized;  /* 0 = fp32 run.c, 1 = int8 runq.c v2 */
    int shared;     /* classifier shares the token embedding */
    int group_size; /* int8 only */
} llama2_format;

typedef struct {
    int8_t *q; /* int8 values */
    float *s;  /* one scale per group_size values */
} llama2_qtensor;

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

/* int8 weights (runq.c); norms stay fp32 and the embedding is dequantized. */
typedef struct {
    llama2_qtensor *q_tokens;
    float *token_embedding_table;
    float *rms_att_weight;
    float *rms_ffn_weight;
    llama2_qtensor *wq, *wk, *wv, *wo; /* one per layer */
    llama2_qtensor *w1, *w2, *w3;
    float *rms_final_weight;
    llama2_qtensor *wcls;
} llama2_qweights;

typedef struct {
    float *x, *xb, *xb2, *hb, *hb2, *q, *k, *v, *att, *logits;
    float *key_cache, *value_cache;
    float *rope;           /* cos/sin pairs for the current position */
    llama2_qtensor xq, hq; /* int8 only: quantized activations */
} llama2_state;

typedef struct {
    char *str;
    int id;
} llama2_token_index;

typedef struct {
    float prob;
    int index;
} llama2_prob_index;

/* One generated position, just before its token is sampled. Exactly one of
 * logits (fp32 engine) and ilogits (int8 engine, Q16) is set; read-only. */
typedef struct {
    const float *logits;
    const int32_t *ilogits;
    int vocab;
    int pos;  /* position being predicted */
    int prev; /* the token before it */
} llama2_step;

/* Return a token id to use instead of sampling, or -1 to sample as usual. */
typedef int (*llama2_step_fn)(const llama2_step *step, void *user);

typedef struct {
    llama2_config config;
    llama2_format format;
    llama2_weights weights;   /* fp32 */
    llama2_qweights qweights; /* int8 */
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

    /* fixed-point int8 engine (llama2_fixed.arm.c) */
    int32_t *fx_freq_q30;   /* RoPE frequency of each (cos, sin) pair */
    int32_t fx_inv_sqrt_hs; /* 1 / sqrt(head_size), Q16 */
    int32_t fx_inv_temp;    /* 1 / temperature, Q16; 0 means argmax */
    int32_t fx_topp;        /* top-p, Q16 */

    /* optional observer of generated positions (NULL after llama2_init) */
    llama2_step_fn on_step;
    void *step_user;
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

/* Parse the 28-byte fp32 header; vocab_size is made positive. */
llama2_status llama2_read_config(const void *header, llama2_config *c, int *shared_weights);

/* Checkpoint size an fp32 header implies, in bytes. */
size_t llama2_checkpoint_bytes(const llama2_config *c, int shared_weights);

/* Recognise either format from the first LLAMA2_PROBE_BYTES (or more) bytes. */
llama2_status llama2_probe(const void *head, size_t n, llama2_config *c, llama2_format *f);

/* File size the header implies for its format, in bytes. */
size_t llama2_file_bytes(const llama2_config *c, const llama2_format *f);

/* Bytes llama2_init allocates (run state, tokenizer, sampler, scratch). */
size_t llama2_runtime_bytes(const llama2_config *c, const llama2_format *f, size_t tokenizer_bytes);

/*
 * Wire up a model whose checkpoint and tokenizer files are already in RAM,
 * in either format. The checkpoint buffer must stay alive and be 4-byte
 * aligned; weights are used in place. An int8 checkpoint is converted in
 * place (its float scales and norms become fixed point), so its buffer must
 * be writable and is no longer a valid file afterwards. The tokenizer buffer
 * may be freed after this returns.
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

/* Text of `token` after `prev` as generation prints it ("" when nothing would print). */
const char *llama2_token_text(llama2 *m, int prev, int token);

float *llama2_forward(llama2 *m, int token, int pos);
int llama2_sample(llama2 *m, float *logits);

#endif
