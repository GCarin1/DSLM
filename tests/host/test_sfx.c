#include <stdio.h>

#include "sfx.h"

static int failures;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
            failures++;                                                \
        }                                                              \
    } while (0)

static int total_frames(sfx_id id)
{
    int n, f = 0;
    const sfx_note *notes = sfx_notes(id, &n);
    for (int i = 0; i < n; i++)
        f += notes[i].frames;
    return f;
}

static void test_tables_are_playable(void)
{
    for (int id = SFX_KEY; id < SFX_COUNT; id++) {
        int n;
        const sfx_note *notes = sfx_notes((sfx_id)id, &n);
        CHECK(n > 0 && notes != NULL);
        for (int i = 0; i < n; i++) {
            CHECK(notes[i].frames > 0);
            CHECK(notes[i].vol <= 127);
            CHECK(notes[i].duty <= 6);
            CHECK(notes[i].freq == 0 || (notes[i].freq >= 100 && notes[i].freq <= 8000));
            CHECK(notes[i].freq != 0 || i > 0); /* no sound starts with a rest */
        }
        /* interface sounds stay short; only the boot chimes may be long */
        CHECK(total_frames((sfx_id)id) <= (id == SFX_BOOT || id == SFX_WAKE ? 90 : 40));
    }
    /* feedback for frequent actions is a blip, never longer than 4 frames */
    CHECK(total_frames(SFX_KEY) <= 4 && total_frames(SFX_MOVE) <= 4 && total_frames(SFX_DELETE) <= 4);
    int n;
    CHECK(sfx_notes(SFX_NONE, &n) == NULL && n == 0);
    CHECK(sfx_notes(SFX_COUNT, &n) == NULL && n == 0);
}

static void test_player_steps_through_every_note(void)
{
    for (int id = SFX_KEY; id < SFX_COUNT; id++) {
        sfx_player p;
        sfx_start(&p, (sfx_id)id);
        int n, notes_started = 0, frames = 0, ends = 0, rests = 0;
        const sfx_note *notes = sfx_notes((sfx_id)id, &n);
        for (int i = 0; i < n; i++)
            rests += notes[i].freq == 0;
        for (int f = 0; f < 200; f++) {
            sfx_step s = sfx_tick(&p);
            if (s.action == SFX_NOTE) {
                notes_started++;
                CHECK(s.freq > 0 && s.vol > 0);
            }
            if (s.action == SFX_VOLUME)
                CHECK(s.vol > 0);
            if (s.action == SFX_END)
                ends++;
            if (s.action != SFX_END && s.action != SFX_IDLE)
                frames = f + 1;
            if (s.action == SFX_SILENCE)
                CHECK(s.vol == 0);
        }
        CHECK(notes_started == n - rests);
        CHECK(ends == 1);
        CHECK(frames <= total_frames((sfx_id)id));
        CHECK(!p.playing);
    }
}

static void test_volume_decays_within_a_note(void)
{
    sfx_player p;
    sfx_start(&p, SFX_DONE);
    int last_note_vol = -1, prev = 0;
    for (int f = 0; f < 60; f++) {
        sfx_step s = sfx_tick(&p);
        if (s.action == SFX_NOTE)
            prev = last_note_vol = s.vol;
        else if (s.action == SFX_VOLUME) {
            CHECK(s.vol <= prev);
            CHECK(s.vol * 2 >= last_note_vol - 1); /* never below half */
            prev = s.vol;
        }
    }
}

static void test_restart_replaces_the_sound(void)
{
    sfx_player p;
    sfx_start(&p, SFX_BOOT);
    for (int f = 0; f < 10; f++)
        sfx_tick(&p);
    sfx_start(&p, SFX_KEY);
    sfx_step s = sfx_tick(&p);
    int n;
    CHECK(s.action == SFX_NOTE && s.freq == sfx_notes(SFX_KEY, &n)[0].freq);
    sfx_start(&p, SFX_NONE);
    CHECK(sfx_tick(&p).action == SFX_IDLE);
}

int main(void)
{
    test_tables_are_playable();
    test_player_steps_through_every_note();
    test_volume_decays_within_a_note();
    test_restart_replaces_the_sound();
    puts(failures ? "FAILED" : "all sfx tests passed");
    return failures ? 1 : 0;
}
