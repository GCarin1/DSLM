/*
 * Host driver for the fixed-point LittleKedi engine (src/engine/mlq_fixed.arm.c).
 *   mlq_main <littlekedi_q.bin> <littlekedi.tok> <steps> <prompt> [temp topp penalty seed]
 * Without the optional arguments: greedy, no repeat penalty. Prints:
 *   P <prompt tokens>   T <all token ids>   S <generated text as hex>
 */
#include <stdio.h>
#include <stdlib.h>

#include "bpe.h"
#include "mlq.h"

#define RECENT 64 /* repeat-penalty window, llama.cpp's default */

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
    if (argc != 5 && argc != 9) {
        fprintf(stderr, "usage: mlq_main <model> <tokenizer> <steps> <prompt> [temp topp penalty seed]\n");
        return 2;
    }
    size_t msize, tsize;
    void *mbuf = slurp(argv[1], &msize), *tbuf = slurp(argv[2], &tsize);
    mlq_model m;
    bpe_tokenizer t;
    mla_status st = mlq_init(&m, mbuf, msize, 0);
    if (st != MLA_OK || bpe_init(&t, tbuf, tsize) != 0) {
        fprintf(stderr, "init failed (%d)\n", (int)st);
        return 1;
    }
    int steps = atoi(argv[3]);
    int32_t inv_temp = 0, topp = 0, penalty = 1 << 16;
    unsigned long long rng = 1;
    if (argc == 9) {
        float temp = (float)atof(argv[5]);
        inv_temp = temp > 0.0f ? fxk_to_fixed(1.0f / temp, 16) : 0;
        topp = fxk_to_fixed((float)atof(argv[6]), 16);
        penalty = fxk_to_fixed((float)atof(argv[7]), 16);
        rng = strtoull(argv[8], NULL, 10);
    }
    int *ids = malloc((size_t)m.seq * sizeof(int));
    int n = bpe_encode(&t, argv[4], ids, m.seq);
    if (n <= 0) {
        fprintf(stderr, "encode failed\n");
        return 1;
    }
    int end = bpe_special(&t, "<|end|>"), eot = bpe_special(&t, "<|endoftext|>");
    int total = n;
    for (int pos = 0; pos < total && pos < m.seq - 1; pos++) {
        int32_t *lg = mlq_forward(&m, ids[pos], pos);
        if (pos < n - 1)
            continue;
        if (total - n >= steps)
            break;
        int from = total > RECENT ? total - RECENT : 0;
        int next = mlq_sample(&m, lg, ids + from, total - from, penalty, inv_temp, topp, &rng);
        if (next == end || next == eot)
            break;
        ids[total++] = next;
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
    printf("\nM %zu\n", mlq_runtime_bytes(&m.c, m.seq));
    return 0;
}
