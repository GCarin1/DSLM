#include <stdio.h>
#include <string.h>

#include "textview.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static int line_is(const char *text, const tv_line *l, const char *want)
{
    return l->len == strlen(want) && memcmp(text + l->start, want, l->len) == 0;
}

static void test_breaks_at_spaces(void)
{
    const char *t = "Once upon a time there was a little girl";
    tv_line l[8];
    int n = tv_wrap(t, strlen(t), 12, l, 8);
    CHECK(n == 4);
    CHECK(line_is(t, &l[0], "Once upon a"));
    CHECK(line_is(t, &l[1], "time there"));
    CHECK(line_is(t, &l[2], "was a little"));
    CHECK(line_is(t, &l[3], "girl"));
}

static void test_never_exceeds_width(void)
{
    const char *t = "aaaa bbbbbbbbbbbbbbbbbbbb cc d eeeeee ffff";
    tv_line l[16];
    int n = tv_wrap(t, strlen(t), 8, l, 16);
    for (int i = 0; i < n; i++)
        CHECK(l[i].len <= 8);
    CHECK(line_is(t, &l[0], "aaaa"));
    CHECK(line_is(t, &l[1], "bbbbbbbb")); /* long word is cut */
}

static void test_newlines_force_breaks(void)
{
    const char *t = "Hi.\nLily said\n\nyes";
    tv_line l[8];
    int n = tv_wrap(t, strlen(t), 32, l, 8);
    CHECK(n == 4);
    CHECK(line_is(t, &l[0], "Hi."));
    CHECK(line_is(t, &l[1], "Lily said"));
    CHECK(line_is(t, &l[2], ""));
    CHECK(line_is(t, &l[3], "yes"));
}

static void test_exact_width_then_space(void)
{
    const char *t = "abcd efgh";
    tv_line l[4];
    int n = tv_wrap(t, strlen(t), 4, l, 4);
    CHECK(n == 2);
    CHECK(line_is(t, &l[0], "abcd"));
    CHECK(line_is(t, &l[1], "efgh"));
}

static void test_null_lines_counts(void)
{
    const char *t = "one two three four five six";
    CHECK(tv_wrap(t, strlen(t), 9, NULL, 0) == tv_wrap(t, strlen(t), 9, (tv_line[8]){{0}}, 8));
    CHECK(tv_wrap("", 0, 32, NULL, 0) == 0);
}

static void test_append_truncates(void)
{
    char buf[8];
    size_t len = 0;
    buf[0] = '\0';
    tv_append(buf, &len, sizeof buf, "abc");
    tv_append(buf, &len, sizeof buf, "defgh");
    CHECK(len == 7);
    CHECK(strcmp(buf, "abcdefg") == 0);
}

int main(void)
{
    test_breaks_at_spaces();
    test_never_exceeds_width();
    test_newlines_force_breaks();
    test_exact_width_then_space();
    test_null_lines_counts();
    test_append_truncates();
    puts(failures ? "FAILED" : "all textview tests passed");
    return failures ? 1 : 0;
}
