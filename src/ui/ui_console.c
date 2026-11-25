/* libnds text-console backend for ui.h. Top screen = text, bottom = status. */
#include "ui.h"

#include <nds.h>
#include <stdarg.h>
#include <stdio.h>

static PrintConsole top;
static PrintConsole bottom;

void ui_init(void)
{
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(&top, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleInit(&bottom, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    consoleSelect(&top);
}

void ui_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
}

void ui_stream(const char *text)
{
    fputs(text, stdout);
    fflush(stdout);
}

void ui_status(const char *fmt, ...)
{
    va_list ap;
    consoleSelect(&bottom);
    consoleClear();
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    consoleSelect(&top);
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
