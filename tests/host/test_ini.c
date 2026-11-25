#include <stdio.h>
#include <string.h>

#include "ini.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static void parse(const char *text, dslm_settings *s)
{
    dslm_settings_defaults(s);
    dslm_ini_parse(text, strlen(text), s);
}

static void test_defaults_on_empty(void)
{
    dslm_settings s;
    parse("", &s);
    CHECK(strcmp(s.checkpoint, "/dslm/models/stories260K.bin") == 0);
    CHECK(strcmp(s.tokenizer, "/dslm/models/tok512.bin") == 0);
    CHECK(s.temperature == 0.0f);
    CHECK(s.topp == 0.9f);
    CHECK(s.steps == 256);
    CHECK(s.seed == 0);
}

static void test_full_file(void)
{
    dslm_settings s;
    parse("[model]\ncheckpoint = /a/m.bin\ntokenizer=/a/t.bin\n"
          "[sampling]\ntemperature = 0.5\ntopp = 0.8\nsteps = 64\nseed = 42\n",
          &s);
    CHECK(strcmp(s.checkpoint, "/a/m.bin") == 0);
    CHECK(strcmp(s.tokenizer, "/a/t.bin") == 0);
    CHECK(s.temperature == 0.5f);
    CHECK(s.topp == 0.8f);
    CHECK(s.steps == 64);
    CHECK(s.seed == 42);
}

static void test_unknown_keys_and_sections_ignored(void)
{
    dslm_settings s;
    parse("[extra]\nsteps = 1\n[sampling]\nbogus = 7\nsteps = 10\n", &s);
    CHECK(s.steps == 10);
    CHECK(strcmp(s.checkpoint, "/dslm/models/stories260K.bin") == 0);
}

static void test_crlf_comments_and_spaces(void)
{
    dslm_settings s;
    parse("; note\r\n# note\r\n  [ sampling ]  \r\n  steps   =   32  \r\n", &s);
    CHECK(s.steps == 32);
}

static void test_invalid_values_keep_defaults(void)
{
    dslm_settings s;
    parse("[sampling]\ntemperature = abc\ntopp = 1.5\nsteps = 0\nseed = -3\n",
          &s);
    CHECK(s.temperature == 0.0f);
    CHECK(s.topp == 0.9f);
    CHECK(s.steps == 256);
    CHECK(s.seed == 0);
}

static void test_key_outside_section_ignored(void)
{
    dslm_settings s;
    parse("steps = 5\n", &s);
    CHECK(s.steps == 256);
}

static void test_path_too_long_keeps_default(void)
{
    dslm_settings s;
    char text[400] = "[model]\ncheckpoint = ";
    memset(text + strlen(text), 'x', 200);
    strcat(text, "\n");
    parse(text, &s);
    CHECK(strcmp(s.checkpoint, "/dslm/models/stories260K.bin") == 0);
}

static void test_not_nul_terminated(void)
{
    dslm_settings s;
    const char raw[] = {'[', 's', 'a', 'm', 'p', 'l', 'i', 'n', 'g', ']', '\n',
                        's', 't', 'e', 'p', 's', '=', '9', '9', 'X'};
    dslm_settings_defaults(&s);
    dslm_ini_parse(raw, sizeof raw - 1, &s); /* drops the trailing 'X' */
    CHECK(s.steps == 99);
}

int main(void)
{
    test_defaults_on_empty();
    test_full_file();
    test_unknown_keys_and_sections_ignored();
    test_crlf_comments_and_spaces();
    test_invalid_values_keep_defaults();
    test_key_outside_section_ignored();
    test_path_too_long_keeps_default();
    test_not_nul_terminated();
    puts(failures ? "FAILED" : "all ini tests passed");
    return failures ? 1 : 0;
}
