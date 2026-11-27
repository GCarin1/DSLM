#include "sfx.h"

#include <stddef.h>

/* Notes (Hz): C5 523, E5 659, G5 784, B5 988, C6 1047, E6 1319, G6 1568, B6 1976, C7 2093 */
#define N(f, fr, v, d) {f, fr, v, d}

static const sfx_note key[] = {N(1800, 2, 38, 0)};
static const sfx_note move[] = {N(1250, 2, 30, 1)};
static const sfx_note del[] = {N(620, 3, 38, 1)};
static const sfx_note page[] = {N(1480, 2, 34, 1), N(1760, 2, 34, 1)};
static const sfx_note open[] = {N(1319, 3, 52, 1), N(1976, 6, 52, 1)};
static const sfx_note back[] = {N(988, 3, 48, 1), N(659, 6, 48, 1)};
static const sfx_note start[] = {N(1047, 3, 50, 1), N(1319, 3, 50, 1), N(1568, 7, 50, 1)};
static const sfx_note stop[] = {N(784, 4, 50, 1), N(523, 8, 50, 1)};
static const sfx_note done[] = {N(1047, 4, 52, 1), N(1319, 4, 52, 1), N(1568, 4, 52, 1), N(0, 2, 0, 0),
                                N(2093, 16, 56, 1)};
static const sfx_note error[] = {N(220, 7, 60, 3), N(0, 2, 0, 0), N(196, 14, 60, 3)};
static const sfx_note boot[] = {N(784, 6, 46, 1),  N(1047, 6, 46, 1), N(1319, 6, 46, 1),
                                N(1568, 6, 46, 1), N(0, 3, 0, 0),     N(2093, 24, 52, 1)};
static const sfx_note wake[] = {N(740, 6, 46, 0),  N(0, 2, 0, 0),  N(1001, 5, 46, 0), N(1262, 7, 46, 2),
                                N(0, 4, 0, 0),     N(1497, 3, 46, 0), N(1480, 3, 40, 0), N(0, 6, 0, 0),
                                N(1976, 10, 50, 0), N(988, 30, 44, 0)};

#define SEQ(a) {a, (int)(sizeof a / sizeof a[0])}

static const struct {
    const sfx_note *notes;
    int n;
} table[SFX_COUNT] = {
    [SFX_NONE] = {NULL, 0},     [SFX_KEY] = SEQ(key),     [SFX_MOVE] = SEQ(move),   [SFX_DELETE] = SEQ(del),
    [SFX_PAGE] = SEQ(page),     [SFX_OPEN] = SEQ(open),   [SFX_BACK] = SEQ(back),   [SFX_START] = SEQ(start),
    [SFX_STOP] = SEQ(stop),     [SFX_DONE] = SEQ(done),   [SFX_ERROR] = SEQ(error), [SFX_BOOT] = SEQ(boot),
    [SFX_WAKE] = SEQ(wake),
};

const sfx_note *sfx_notes(sfx_id id, int *n)
{
    if ((int)id < 0 || id >= SFX_COUNT) {
        *n = 0;
        return NULL;
    }
    *n = table[id].n;
    return table[id].notes;
}

void sfx_start(sfx_player *p, sfx_id id)
{
    p->notes = sfx_notes(id, &p->n);
    p->i = 0;
    p->elapsed = 0;
    p->playing = p->n > 0;
}

sfx_step sfx_tick(sfx_player *p)
{
    sfx_step s = {SFX_IDLE, 0, 0, 0};
    if (!p->playing)
        return s;
    if (p->i >= p->n) {
        p->playing = 0;
        s.action = SFX_END;
        return s;
    }
    const sfx_note *note = &p->notes[p->i];
    s.freq = note->freq;
    s.duty = note->duty;
    /* linear decay to half volume over the note */
    s.vol = (uint8_t)(note->vol - note->vol * p->elapsed / (2 * note->frames));
    if (note->freq == 0)
        s.action = p->elapsed == 0 ? SFX_SILENCE : SFX_IDLE;
    else
        s.action = p->elapsed == 0 ? SFX_NOTE : SFX_VOLUME;
    if (++p->elapsed >= note->frames) {
        p->i++;
        p->elapsed = 0;
    }
    return s;
}
