#include "mic.h"

int mic_level_s16(const int16_t *s, int n)
{
    if (n <= 0)
        return 0;
    int64_t sum = 0;
    for (int i = 0; i < n; i++)
        sum += s[i];
    int32_t mean = (int32_t)(sum / n);
    int64_t dev = 0;
    for (int i = 0; i < n; i++) {
        int32_t d = s[i] - mean;
        dev += d < 0 ? -d : d;
    }
    int level = (int)(dev / n / 256);
    return level > 127 ? 127 : level;
}

void mic_reset(mic_detector *d)
{
    d->loud = 0;
    d->peak = 0;
    d->cooldown = 0;
}

int mic_feed(mic_detector *d, int level)
{
    if (d->cooldown > 0) {
        d->cooldown--;
        d->loud = 0;
        d->peak = 0;
        return 0;
    }
    if (level < MIC_THRESHOLD) {
        d->loud = 0;
        d->peak = 0;
        return 0;
    }
    if (level > d->peak)
        d->peak = level;
    if (++d->loud < MIC_FRAMES)
        return 0;
    int peak = d->peak;
    d->loud = 0;
    d->peak = 0;
    d->cooldown = MIC_COOLDOWN;
    return peak;
}

int mic_temperature_milli(int peak)
{
    if (peak <= MIC_THRESHOLD)
        return 200;
    if (peak >= 127)
        return 1200;
    return 200 + (peak - MIC_THRESHOLD) * 1000 / (127 - MIC_THRESHOLD);
}
