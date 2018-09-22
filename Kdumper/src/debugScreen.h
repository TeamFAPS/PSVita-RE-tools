#ifndef DEBUG_SCREEN_H
#define DEBUG_SCREEN_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#define COLOR_BLACK      0xFF000000
#define COLOR_RED        0xFF0000FF
#define COLOR_BLUE       0xFFFF0000
#define COLOR_YELLOW     0xFF00FFFF
#define COLOR_GREEN      0xFF00FF00
#define COLOR_MAGENTA    0xFFFF00FF
#define COLOR_CYAN       0xFFFFFF00
#define COLOR_WHITE      0xFFFFFFFF
#define COLOR_GREY       0xFF808080
#define COLOR_DEFAULT_FG COLOR_WHITE
#define COLOR_DEFAULT_BG COLOR_BLACK

int psvDebugScreenInit();

void psvDebugScreenClear(int bg_color);

uint32_t psvDebugScreenSetFgColor(uint32_t color);
uint32_t psvDebugScreenSetBgColor(uint32_t color);

int psvDebugScreenPuts(const char * text);
int psvDebugScreenPrintf(const char *format, ...);

#define printf(...) psvDebugScreenPrintf(__VA_ARGS__)

#endif