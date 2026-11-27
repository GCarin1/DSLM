#include <stdio.h>

#include "mic.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static void test_level(void)
{
    int16_t quiet[400], loud[400], offset[400];
    for (int i = 0; i < 400; i++) {
        quiet[i] = (int16_t)((i % 2) ? 200 : -200);
        loud[i] = (int16_t)((i % 2) ? 30000 : -30000);
        offset[i] = (int16_t)(5000 + ((i % 2) ? 200 : -200)); /* DC offset must not count */
    }
    CHECK(mic_level_s16(quiet, 400) < 5);
    CHECK(mic_level_s16(loud, 400) == 117);
    CHECK(mic_level_s16(offset, 400) == mic_level_s16(quiet, 400));
    CHECK(mic_level_s16(loud, 0) == 0);
    int16_t max[2] = {32767, -32768};
    CHECK(mic_level_s16(max, 2) == 127);
}

static void test_blow_needs_a_sustained_sound(void)
{
    mic_detector d;
    mic_reset(&d);
    for (int f = 0; f < 200; f++) /* silence */
        CHECK(mic_feed(&d, 10) == 0);
    for (int burst = 0; burst < 20; burst++) { /* short clicks never trigger */
        for (int f = 0; f < MIC_FRAMES - 1; f++)
            CHECK(mic_feed(&d, 120) == 0);
        CHECK(mic_feed(&d, 5) == 0);
    }
    int got = 0, at = -1;
    for (int f = 0; f < MIC_FRAMES; f++) {
        int p = mic_feed(&d, f == 3 ? 90 : 60);
        if (p) {
            got = p;
            at = f;
        }
    }
    CHECK(got == 90 && at == MIC_FRAMES - 1); /* the peak of the run, on its last frame */
    for (int f = 0; f < MIC_COOLDOWN; f++)    /* no second trigger during the cooldown */
        CHECK(mic_feed(&d, 127) == 0);
    int again = 0;
    for (int f = 0; f < MIC_FRAMES; f++)
        again |= mic_feed(&d, 70);
    CHECK(again == 70);
}

static void test_temperature_mapping(void)
{
    CHECK(mic_temperature_milli(0) == 200);
    CHECK(mic_temperature_milli(MIC_THRESHOLD) == 200);
    CHECK(mic_temperature_milli(127) == 1200);
    CHECK(mic_temperature_milli(200) == 1200);
    int prev = 0;
    for (int p = MIC_THRESHOLD; p <= 127; p++) { /* louder never means less creative */
        int t = mic_temperature_milli(p);
        CHECK(t >= prev && t >= 200 && t <= 1200);
        prev = t;
    }
}

int main(void)
{
    test_level();
    test_blow_needs_a_sustained_sound();
    test_temperature_mapping();
    puts(failures ? "FAILED" : "all mic tests passed");
    return failures ? 1 : 0;
}
