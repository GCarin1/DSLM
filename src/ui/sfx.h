/*
 * DSLM's sound effects: original note sequences for the DS's PSG square-wave
 * channels (no samples, no RAM), and the per-frame sequencer that plays
 * them. Pure C, tested on the host; sound.c in ui_gfx.c turns the steps
 * into libnds sound calls from the VBlank interrupt.
 */
#ifndef DSLM_SFX_H
#define DSLM_SFX_H

#include <stdint.h>

typedef enum {
    SFX_NONE,
    SFX_KEY,    /* a key typed */
    SFX_MOVE,   /* focus or selection moved */
    SFX_DELETE, /* a character deleted */
    SFX_PAGE,   /* keyboard page, shift or example prompt changed */
    SFX_OPEN,   /* a card opened, settings saved, new prompt */
    SFX_BACK,   /* back to the menu */
    SFX_START,  /* generation starts */
    SFX_STOP,   /* generation stopped by the user */
    SFX_DONE,   /* the story or answer is finished */
    SFX_ERROR,  /* a load failed or a fatal error */
    SFX_BOOT,   /* the model is loaded at boot */
    SFX_COUNT
} sfx_id;

typedef struct {
    uint16_t freq;  /* Hz; 0 is a rest */
    uint8_t frames; /* 1/60 s */
    uint8_t vol;    /* 0-127, decays to half over the note */
    uint8_t duty;   /* libnds DutyCycle value (0 = 12.5%, 1 = 25%, 3 = 50%) */
} sfx_note;

typedef enum { SFX_IDLE, SFX_NOTE, SFX_VOLUME, SFX_SILENCE, SFX_END } sfx_action;

typedef struct {
    sfx_action action;
    uint16_t freq;
    uint8_t vol;
    uint8_t duty;
} sfx_step;

typedef struct {
    const sfx_note *notes;
    int n, i, elapsed;
    int playing;
} sfx_player;

const sfx_note *sfx_notes(sfx_id id, int *n);
/* Start `id` from its first note, replacing whatever was playing. */
void sfx_start(sfx_player *p, sfx_id id);
/*
 * One frame: SFX_NOTE to start a note, SFX_VOLUME for its decay, SFX_SILENCE
 * for a rest, SFX_END once after the last note, then SFX_IDLE.
 */
sfx_step sfx_tick(sfx_player *p);

#endif
