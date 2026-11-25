/*
 * Drift of the fixed-point LittleKedi engine against the fp32 one, teacher-forced:
 * both read the same token sequence; at every position after the first we count
 * whether their argmax agrees, and average the relative L2 error of the logits.
 *   mlq_drift <littlekedi.bin> <littlekedi_q.bin> <token ids...>
 * Prints: <agreements> <positions> <mean relative error>
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "mla.h"
#include "mlq.h"

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
    if (argc < 4) {
        fprintf(stderr, "usage: mlq_drift <fp32 model> <int8 model> <token ids...>\n");
        return 2;
    }
    size_t fsize, qsize;
    void *fbuf = slurp(argv[1], &fsize), *qbuf = slurp(argv[2], &qsize);
    mla_model f;
    mlq_model q;
    if (mla_init(&f, fbuf, fsize, 0) != MLA_OK || mlq_init(&q, qbuf, qsize, 0) != MLA_OK) {
        fprintf(stderr, "init failed\n");
        return 1;
    }
    int n = argc - 3, agree = 0;
    double rel = 0.0;
    for (int pos = 0; pos < n; pos++) {
        int token = atoi(argv[3 + pos]);
        const float *lf = mla_forward(&f, token, pos);
        const int32_t *lq = mlq_forward(&q, token, pos);
        double err = 0.0, norm = 0.0;
        int bf = 0, bq = 0;
        for (int i = 0; i < f.c.vocab; i++) {
            double v = lq[i] / 65536.0;
            err += (v - lf[i]) * (v - lf[i]);
            norm += (double)lf[i] * lf[i];
            if (lf[i] > lf[bf])
                bf = i;
            if (lq[i] > lq[bq])
                bq = i;
        }
        rel += sqrt(err / norm);
        if (pos > 0)
            agree += bf == bq;
    }
    printf("%d %d %.5f\n", agree, n - 1, rel / n);
    return 0;
}
