/*
 * GPT-2 byte-level BPE (LittleKedi's tokenizer), on the littlekedi.tok file
 * written by tools/m8/convert_littlekedi.py. Pure C; the file stays in RAM.
 */
#ifndef DSLM_BPE_H
#define DSLM_BPE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int vocab, n_merges, n_special;
    const uint8_t **piece; /* raw bytes of each token */
    int *piece_len;
    int *special_id;       /* special tokens (chat markers) */
    const uint8_t *merges; /* [rank][left, right, result] int32, unaligned in the file */
    uint16_t *by_pair;     /* merge ranks sorted by (left, right) */
    int byte_tok[256];     /* token of each single byte */
    void *arena;
} bpe_tokenizer;

#define BPE_HEADER_BYTES 16

/* Bytes bpe_init allocates, from the first BPE_HEADER_BYTES of the file; 0 if it is not a BPE1 file. */
size_t bpe_runtime_bytes(const void *header);

/* The file buffer must stay alive while the tokenizer is used (pieces point into it). */
int bpe_init(bpe_tokenizer *t, const void *file, size_t size);
void bpe_free(bpe_tokenizer *t);

/* Encode UTF-8 text, special markers included; returns the count or -1 if cap is too small. */
int bpe_encode(const bpe_tokenizer *t, const char *text, int *out, int cap);

/* Token id of a special marker such as "<|end|>", or -1. */
int bpe_special(const bpe_tokenizer *t, const char *text);

/* Raw bytes of a token (not NUL-terminated). Special tokens decode to nothing. */
const uint8_t *bpe_piece(const bpe_tokenizer *t, int id, int *len);

#endif
