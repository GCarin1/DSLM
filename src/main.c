/* DSLM M1: mount the card, read /dslm/dslm.ini, print the settings. */
#include <stdlib.h>

#include "storage/ini.h"
#include "storage/sdcard.h"
#include "ui/ui.h"

#define INI_PATH "/dslm/dslm.ini"
#define INI_MAX_SIZE 4096

int main(void)
{
    dslm_settings s;
    void *text = NULL;
    size_t len = 0;

    ui_init();
    ui_log("DSLM - Llama 2 on the DS");

    if (!sd_mount())
        ui_fatal("FAT mount failed.\nThe binary may need a manual DLDI patch (dlditool).");
    ui_log("FAT mounted");

    dslm_settings_defaults(&s);
    sd_status st = sd_read_file(INI_PATH, INI_MAX_SIZE, &text, &len);
    if (st == SD_OK) {
        dslm_ini_parse(text, len, &s);
        free(text);
        ui_log("read %s (%u bytes)", INI_PATH, (unsigned)len);
    } else {
        ui_log("%s: %s, using defaults", INI_PATH, sd_status_text(st));
    }

    ui_log("checkpoint: %s", s.checkpoint);
    ui_log("tokenizer:  %s", s.tokenizer);
    ui_log("temp x1000: %d  topp x1000: %d", (int)(s.temperature * 1000),
           (int)(s.topp * 1000));
    ui_log("steps: %d  seed: %u", s.steps, (unsigned)s.seed);

    ui_fatal("M1 done: nothing more to run yet.");
}
