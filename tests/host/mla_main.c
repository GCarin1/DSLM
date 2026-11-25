/*
 * Host driver for src/engine/mla.c + bpe.c (LittleKedi). Greedy, no repeat
 * penalty, so it can be compared with tools/m8/littlekedi_ref.py. Prints:
 *   P <prompt tokens>   T <all token ids>   S <generated text as hex>
 */
#include <stdio.h>
#include <stdlib.h>

#include "bpe.h"
#include "mla.h"

static void *slurp(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "cannot read %s\n", path);
        exit(1);
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: mla_main <littlekedi.bin> <littlekedi.tok> <steps> <prompt>\n");
        return 2;
    }
    size_t msize, tsize;
    void *mbuf = slurp(argv[1], &msize), *tbuf = slurp(argv[2], &tsize);
    mla_model m;
    bpe_tokenizer t;
    if (mla_init(&m, mbuf, msize, 0) != MLA_OK || bpe_init(&t, tbuf, tsize) != 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }
    int steps = atoi(argv[3]);
    int *ids = malloc((size_t)m.seq * sizeof(int));
    int n = bpe_encode(&t, argv[4], ids, m.seq);
    if (n <= 0) {
        fprintf(stderr, "encode failed\n");
        return 1;
    }
    int end = bpe_special(&t, "<|end|>"), eot = bpe_special(&t, "<|endoftext|>");
    int total = n;
    for (int pos = 0; pos < total && pos < m.seq - 1; pos++) {
        float *lg = mla_forward(&m, ids[pos], pos);
        if (pos < n - 1)
            continue;
        if (total - n >= steps)
            break;
        int best = 0;
        for (int i = 1; i < m.c.vocab; i++)
            if (lg[i] > lg[best])
                best = i;
        if (best == end || best == eot)
            break;
        ids[total++] = best;
    }
    printf("P %d\nT", n);
    for (int i = 0; i < total; i++)
        printf(" %d", ids[i]);
    printf("\nS ");
    for (int i = n; i < total; i++) {
        int len;
        const uint8_t *p = bpe_piece(&t, ids[i], &len);
        for (int k = 0; k < len; k++)
            printf("%02x", p[k]);
    }
    printf("\n");
    return 0;
}
