/*
 * Golden-output driver. Reuses the pinned llama2.c run.c (fp32) or runq.c
 * (int8, -DLLAMA2C_SOURCE='"runq.c"') unchanged (MIT, Andrej Karpathy):
 * TESTING drops its main(), and this loop mirrors its generate() but prints
 * token ids instead of streaming text. Both files share the API used here.
 *
 * Output, one record per run:
 *   P <prompt token count, BOS included>
 *   T <token ids, BOS first>
 *   S <generated text as hex, filtered like safe_printf>
 */
#define TESTING
#ifndef LLAMA2C_SOURCE
#define LLAMA2C_SOURCE "run.c"
#endif
#include LLAMA2C_SOURCE

static void append_piece(char *buf, size_t *n, const char *piece)
{
    if (piece == NULL || piece[0] == '\0')
        return;
    if (piece[1] == '\0') {
        unsigned char b = (unsigned char)piece[0];
        if (!(isprint(b) || isspace(b)))
            return;
    }
    size_t len = strlen(piece);
    memcpy(buf + *n, piece, len);
    *n += len;
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: golden <checkpoint> <tokenizer> <steps> <prompt>\n");
        return 2;
    }
    char *prompt = argv[4];
    int steps = atoi(argv[3]);

    Transformer transformer;
    build_transformer(&transformer, argv[1]);
    if (steps <= 0 || steps > transformer.config.seq_len)
        steps = transformer.config.seq_len;

    Tokenizer tokenizer;
    build_tokenizer(&tokenizer, argv[2], transformer.config.vocab_size);

    Sampler sampler;
    build_sampler(&sampler, transformer.config.vocab_size, 0.0f, 0.9f, 0);

    int *prompt_tokens = malloc((strlen(prompt) + 3) * sizeof(int));
    int n_prompt = 0;
    encode(&tokenizer, prompt, 1, 0, prompt_tokens, &n_prompt);
    if (n_prompt < 1) {
        fprintf(stderr, "expected at least 1 prompt token\n");
        return 1;
    }

    char *text = malloc((size_t)steps * (tokenizer.max_token_length + 8) + 1);
    size_t text_len = 0;

    printf("P %d\nT %d", n_prompt, prompt_tokens[0]);
    int token = prompt_tokens[0];
    int pos = 0;
    while (pos < steps) {
        float *logits = forward(&transformer, token, pos);
        int next = pos < n_prompt - 1 ? prompt_tokens[pos + 1]
                                      : sample(&sampler, logits);
        pos++;
        if (next == 1)
            break;
        printf(" %d", next);
        append_piece(text, &text_len, decode(&tokenizer, token, next));
        token = next;
    }

    printf("\nS ");
    for (size_t i = 0; i < text_len; i++)
        printf("%02x", (unsigned char)text[i]);
    printf("\n");

    free(text);
    free(prompt_tokens);
    free_sampler(&sampler);
    free_tokenizer(&tokenizer);
    free_transformer(&transformer);
    return 0;
}
