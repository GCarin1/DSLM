/*
 * Voice mode: microphone loudness, blow detection and the loudness ->
 * temperature mapping. Pure C, tested on the host; ui_gfx.c records the
 * microphone (16-bit PCM) and feeds a level per frame.
 */
#ifndef DSLM_MIC_H
#define DSLM_MIC_H

#include <stdint.h>

#define MIC_THRESHOLD 40 /* level (0-127) that counts as blowing or a loud voice */
#define MIC_FRAMES 15    /* that long in a row (1/60 s each) starts a generation */
#define MIC_COOLDOWN 90  /* frames after a trigger before the next one can start */

typedef struct {
    int loud;     /* frames in a row at or above the threshold */
    int peak;     /* loudest level of the current loud run */
    int cooldown;
} mic_detector;

/* Loudness 0-127 of signed 16-bit samples: mean distance from their average, /256. */
int mic_level_s16(const int16_t *s, int n);

void mic_reset(mic_detector *d);
/* One frame's level; returns the peak (> 0) when a blow is detected, else 0. */
int mic_feed(mic_detector *d, int level);

/* Peak level -> temperature in thousandths: 200 for a soft blow up to 1200 for the loudest. */
int mic_temperature_milli(int peak);

#endif
