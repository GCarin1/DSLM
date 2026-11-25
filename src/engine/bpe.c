/* GPT-2 byte-level BPE; see bpe.h. Mirrors tools/m8/littlekedi_ref.py. */
#include "bpe.h"

#include <stdlib.h>
#include <string.h>

static int rd32(const uint8_t *p)
{
    int32_t v;
    memcpy(&v, p, 4); /* the file has no alignment guarantee; ARM9 needs aligned loads */
    return v;
}

/* field k (0 left, 1 right, 2 result) of merge rank r */
static int merge_field(const bpe_tokenizer *t, int r, int k)
{
    return rd32(t->merges + 12 * (size_t)r + 4 * k);
}

static const bpe_tokenizer *sort_ctx;

static int cmp_pair(const void *a, const void *b)
{
    int ra = *(const uint16_t *)a, rb = *(const uint16_t *)b;
    int la = merge_field(sort_ctx, ra, 0), lb = merge_field(sort_ctx, rb, 0);
    if (la != lb)
        return la < lb ? -1 : 1;
    int xa = merge_field(sort_ctx, ra, 1), xb = merge_field(sort_ctx, rb, 1);
    return xa < xb ? -1 : xa > xb;
}

int bpe_init(bpe_tokenizer *t, const void *file, size_t size)
{
    const uint8_t *p = file, *end = p + size;
    memset(t, 0, sizeof *t);
    if (size < 16 || memcmp(p, "BPE1", 4) != 0)
        return -1;
    t->vocab = rd32(p + 4);
    t->n_merges = rd32(p + 8);
    t->n_special = rd32(p + 12);
    if (t->vocab <= 256 || t->n_merges < 0 || t->n_merges > 65535 || t->n_special < 0)
        return -1;
    p += 16;
    size_t bytes = (size_t)t->vocab * (sizeof(uint8_t *) + sizeof(int)) + (size_t)t->n_special * sizeof(int) +
                   (size_t)t->n_merges * sizeof(uint16_t);
    uint8_t *arena = malloc(bytes);
    if (!arena)
        return -1;
    t->arena = arena;
    t->piece = (const uint8_t **)arena;
    t->piece_len = (int *)(arena + (size_t)t->vocab * sizeof(uint8_t *));
    t->special_id = t->piece_len + t->vocab;
    t->by_pair = (uint16_t *)(t->special_id + t->n_special);

    for (int i = 0; i < t->vocab; i++) {
        if (end - p < 4 || rd32(p) < 0 || end - p - 4 < rd32(p))
            goto bad;
        t->piece_len[i] = rd32(p);
        t->piece[i] = p + 4;
        p += 4 + t->piece_len[i];
    }
    for (int i = 0; i < t->n_special; i++) {
        if (end - p < 8)
            goto bad;
        t->special_id[i] = rd32(p);
        p += 8 + rd32(p + 4);
    }
    if (end - p < (ptrdiff_t)(12 * (size_t)t->n_merges))
        goto bad;
    t->merges = p;

    for (int b = 0; b < 256; b++)
        t->byte_tok[b] = -1;
    for (int i = 0; i < t->vocab; i++) {
        int special = 0;
        for (int s = 0; s < t->n_special; s++)
            special |= t->special_id[s] == i;
        if (!special && t->piece_len[i] == 1 && t->byte_tok[t->piece[i][0]] < 0)
            t->byte_tok[t->piece[i][0]] = i;
    }
    for (int i = 0; i < t->n_merges; i++)
        t->by_pair[i] = (uint16_t)i;
    sort_ctx = t;
    qsort(t->by_pair, t->n_merges, sizeof(uint16_t), cmp_pair);
    return 0;
bad:
    bpe_free(t);
    return -1;
}

void bpe_free(bpe_tokenizer *t)
{
    free(t->arena);
    t->arena = NULL;
}

/* rank of merging (left, right), or -1 */
static int merge_rank(const bpe_tokenizer *t, int left, int right, int *result)
{
    int lo = 0, hi = t->n_merges - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int r = t->by_pair[mid];
        int ml = merge_field(t, r, 0), mr = merge_field(t, r, 1);
        if (ml == left && mr == right) {
            *result = merge_field(t, r, 2);
            return r;
        }
        if (ml < left || (ml == left && mr < right))
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}

/* BPE of one pre-token: bytes -> ids, merging the lowest-rank pair first */
static int bpe_word(const bpe_tokenizer *t, const uint8_t *s, int n, int *out, int cap)
{
    if (n > cap)
        return -1;
    for (int i = 0; i < n; i++)
        out[i] = t->byte_tok[s[i]];
    for (;;) {
        int best = -1, best_rank = 1 << 30, best_result = 0;
        for (int i = 0; i + 1 < n; i++) {
            int result, r = merge_rank(t, out[i], out[i + 1], &result);
            if (r >= 0 && r < best_rank) {
                best_rank = r;
                best = i;
                best_result = result;
            }
        }
        if (best < 0)
            return n;
        out[best] = best_result;
        memmove(out + best + 1, out + best + 2, (size_t)(n - best - 2) * sizeof(int));
        n--;
    }
}

static int is_space(uint8_t c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static int is_letter(uint8_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0x80; /* non-ASCII: letters */
}

static int is_digit(uint8_t c)
{
    return c >= '0' && c <= '9';
}

static int match_special(const bpe_tokenizer *t, const uint8_t *s, int *len)
{
    for (int i = 0; i < t->n_special; i++) {
        int id = t->special_id[i], n = t->piece_len[id];
        if (n > 0 && memcmp(s, t->piece[id], (size_t)n) == 0) {
            *len = n;
            return id;
        }
    }
    return -1;
}

/*
 * GPT-2 pre-tokenizer: 's|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+
 * (ASCII classes; bytes >= 0x80 count as letters).
 */
static int next_pretoken(const uint8_t *s, int n)
{
    static const char *contractions[] = {"'s", "'t", "'re", "'ve", "'m", "'ll", "'d"};
    if (s[0] == '\'') {
        for (int i = 0; i < 7; i++) {
            int len = (int)strlen(contractions[i]);
            if (len <= n && memcmp(s, contractions[i], (size_t)len) == 0)
                return len;
        }
    }
    int i = s[0] == ' ' && n > 1 && !is_space(s[1]) ? 1 : 0;
    if (i < n && is_letter(s[i])) {
        while (i < n && is_letter(s[i]))
            i++;
        return i;
    }
    if (i < n && is_digit(s[i])) {
        while (i < n && is_digit(s[i]))
            i++;
        return i;
    }
    if (i < n && !is_space(s[i])) {
        while (i < n && !is_space(s[i]) && !is_letter(s[i]) && !is_digit(s[i]))
            i++;
        return i;
    }
    /* whitespace run; leave its last character to prefix the next word */
    int j = 0;
    while (j < n && is_space(s[j]))
        j++;
    if (j == n)
        return j;
    if (j > 1)
        return j - 1;
    return 1;
}

int bpe_encode(const bpe_tokenizer *t, const char *text, int *out, int cap)
{
    const uint8_t *s = (const uint8_t *)text;
    int n = (int)strlen(text), count = 0, i = 0;
    while (i < n) {
        int len, id = match_special(t, s + i, &len);
        if (id >= 0) {
            if (count >= cap)
                return -1;
            out[count++] = id;
            i += len;
            continue;
        }
        /* plain text up to the next special marker */
        int stop = i;
        while (stop < n && match_special(t, s + stop, &len) < 0)
            stop++;
        while (i < stop) {
            int w = next_pretoken(s + i, stop - i);
            int got = bpe_word(t, s + i, w, out + count, cap - count);
            if (got < 0)
                return -1;
            count += got;
            i += w;
        }
    }
    return count;
}

int bpe_special(const bpe_tokenizer *t, const char *text)
{
    int len;
    int id = match_special(t, (const uint8_t *)text, &len);
    return id >= 0 && len == (int)strlen(text) ? id : -1;
}

const uint8_t *bpe_piece(const bpe_tokenizer *t, int id, int *len)
{
    for (int i = 0; i < t->n_special; i++) {
        if (t->special_id[i] == id) {
            *len = 0;
            return t->piece[id];
        }
    }
    *len = t->piece_len[id];
    return t->piece[id];
}
