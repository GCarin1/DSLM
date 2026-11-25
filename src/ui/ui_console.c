/* libnds text-console backend for ui.h. Top screen = log, bottom = reserved. */
#include "ui.h"

#include <nds.h>
#include <stdarg.h>
#include <stdio.h>

static PrintConsole top;

void ui_init(void)
{
    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    consoleInit(&top, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
}

void ui_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
}

void ui_halt(void)
{
    for (;;)
        swiWaitForVBlank();
}

void ui_fatal(const char *fmt, ...)
{
    va_list ap;
    printf("\nERROR: ");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    ui_halt();
}
