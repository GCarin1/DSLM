/*
 * Host driver for the fixed-point LittleKedi engine (src/engine/mlq_fixed.arm.c).
 *   mlq_main <littlekedi_q80.bin> <littlekedi.tok> <steps> <prompt> [temp topp penalty seed]
 * Without the optional arguments: greedy, no repeat penalty. Prints:
 *   P <prompt tokens>   T <all token ids>   S <generated text as hex>
 */
#include <stdio.h>
#include <stdlib.h>

#include "bpe.h"
#include "mlq_chat.h" /* the same generation loop as the DS app */

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
    mlq_sampling s;
    if (argc == 9)
        mlq_sampling_init(&s, (float)atof(argv[5]), (float)atof(argv[6]), (float)atof(argv[7]),
                          strtoull(argv[8], NULL, 10));
    else
        mlq_sampling_init(&s, 0.0f, 0.0f, 1.0f, 1);
    int *ids = malloc((size_t)m.seq * sizeof(int));
    int n = 0, total = mlq_generate(&m, &t, argv[4], steps, &s, ids, m.seq, &n, NULL, NULL);
    if (total < 0) {
        fprintf(stderr, "encode failed\n");
        return 1;
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
