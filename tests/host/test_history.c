#include <stdio.h>
#include <string.h>

#include "history.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static void test_story_round_trip(void)
{
    history_entry a = {0}, b;
    char buf[1024];
    strcpy(a.date, "2026-11-26 20:15");
    strcpy(a.model, "stories260K_q80");
    strcpy(a.prompt, "Once upon a time");
    a.tokens = 257;
    a.text = "Once upon a time, there was a cat.\n\nThe end.";
    a.text_len = strlen(a.text);
    size_t n = history_format(&a, buf, sizeof buf);
    CHECK(n > a.text_len);
    CHECK(history_parse(buf, n, &b) == 0);
    CHECK(strcmp(b.date, a.date) == 0 && strcmp(b.model, a.model) == 0);
    CHECK(strcmp(b.prompt, a.prompt) == 0);
    CHECK(!b.chat && b.question == 0 && b.tokens == 257 && !b.stopped);
    CHECK(b.text_len == a.text_len && memcmp(b.text, a.text, a.text_len) == 0); /* blank lines in text kept */
}

static void test_chat_round_trip_and_question_clamp(void)
{
    history_entry a = {0}, b;
    char buf[512];
    strcpy(a.model, "littlekedi_q80");
    strcpy(a.prompt, "What causes the seasons?");
    a.chat = 1;
    a.stopped = 1;
    a.text = "What causes the seasons?\n\nThe tilt.";
    a.text_len = strlen(a.text);
    a.question = 24;
    size_t n = history_format(&a, buf, sizeof buf);
    CHECK(history_parse(buf, n, &b) == 0);
    CHECK(b.chat && b.question == 24 && b.stopped && b.date[0] == '\0');
    const char *bad = "dslm-history=1\nkind=chat\nquestion=999\n\nshort";
    CHECK(history_parse(bad, strlen(bad), &b) == 0);
    CHECK(b.question == 5); /* never past the text */
}

static void test_prompt_newlines_cannot_break_the_header(void)
{
    history_entry a = {0}, b;
    char buf[512];
    strcpy(a.prompt, "line one\nmodel=evil");
    a.text = "x";
    a.text_len = 1;
    size_t n = history_format(&a, buf, sizeof buf);
    CHECK(history_parse(buf, n, &b) == 0);
    CHECK(strcmp(b.prompt, "line onemodel=evil") == 0);
    CHECK(b.model[0] == '\0');
}

static void test_rejects_and_tolerates(void)
{
    history_entry b;
    CHECK(history_parse("hello", 5, &b) == -1);
    CHECK(history_parse("", 0, &b) == -1);
    const char *cut = "dslm-history=1\r\ndate=2026-01-02 03:04\r\ntokens=abc\r\nprompt=Hi"; /* CRLF, no text yet */
    CHECK(history_parse(cut, strlen(cut), &b) == 0);
    CHECK(strcmp(b.date, "2026-01-02 03:04") == 0 && b.tokens == 0);
    CHECK(strcmp(b.prompt, "Hi") == 0 && b.text_len == 0);
    char small[16];
    history_entry a = {0};
    a.text = "a long enough text";
    a.text_len = strlen(a.text);
    CHECK(history_format(&a, small, sizeof small) == 0);
}

static void test_names(void)
{
    char name[16];
    history_name(42, name);
    CHECK(strcmp(name, "H0042.TXT") == 0);
    CHECK(history_number(name) == 42);
    CHECK(history_number("h0007.txt") == 7);
    CHECK(history_number("H12.TXT") == -1);
    CHECK(history_number("H0042.BIN") == -1);
    CHECK(history_number("X0042.TXT") == -1);
    CHECK(history_number("H00a2.TXT") == -1);
}

int main(void)
{
    test_story_round_trip();
    test_chat_round_trip_and_question_clamp();
    test_prompt_newlines_cannot_break_the_header();
    test_rejects_and_tolerates();
    test_names();
    puts(failures ? "FAILED" : "all history tests passed");
    return failures ? 1 : 0;
}
