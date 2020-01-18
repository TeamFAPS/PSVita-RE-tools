#ifndef DEBUG_SCREEN_H
#define DEBUG_SCREEN_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

#include "debugScreenFont.c"

#define SCREEN_WIDTH    (960)
#define SCREEN_HEIGHT   (544)
#define SCREEN_FB_WIDTH (960)
#define SCREEN_FB_SIZE  (2 * 1024 * 1024)
#define SCREEN_FB_ALIGN (256 * 1024)
#define SCREEN_GLYPH_W  (8)
#define SCREEN_GLYPH_H  (8)

#define COLOR_BLACK      0xFF000000
#define COLOR_RED        0xFF0000FF
#define COLOR_BLUE       0xFF00FF00
#define COLOR_YELLOW     0xFF00FFFF
#define COLOR_GREEN      0xFFFF0000
#define COLOR_MAGENTA    0xFFFF00FF
#define COLOR_CYAN       0xFFFFFF00
#define COLOR_WHITE      0xFFFFFFFF
#define COLOR_GREY       0xFF808080
#define COLOR_DEFAULT_FG COLOR_WHITE
#define COLOR_DEFAULT_BG COLOR_BLACK

static int psvDebugScreenMutex; /*< avoid race condition when outputing strings */
static uint32_t psvDebugScreenCoordX = 0;
static uint32_t psvDebugScreenCoordY = 0;
static uint32_t psvDebugScreenColorFg = COLOR_DEFAULT_FG;
static uint32_t psvDebugScreenColorBg = COLOR_DEFAULT_BG;
//static SceDisplayFrameBuf psvDebugScreenFrameBuf = {sizeof(SceDisplayFrameBuf), NULL, SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

uint32_t psvDebugScreenSetFgColor(uint32_t color) {
	uint32_t prev_color = psvDebugScreenColorFg;
	psvDebugScreenColorFg = color;
	return prev_color;
}

uint32_t psvDebugScreenSetBgColor(uint32_t color) {
	uint32_t prev_color = psvDebugScreenColorBg;
	psvDebugScreenColorBg = color;
	return prev_color;
}

static size_t psvDebugScreenEscape(const char *str){
	int i,j, p=0, params[8]={};
	for(i=0; i<8 && str[i]!='\0'; i++){
		if(str[i] >= '0' && str[i] <= '9'){
			params[p]=(params[p]*10) + (str[i] - '0');
		}else if(str[i] == ';'){
			p++;
		}else if(str[i] == 'f' || str[i] == 'H'){
			psvDebugScreenCoordX = params[0] * SCREEN_GLYPH_W;
			psvDebugScreenCoordY = params[1] * SCREEN_GLYPH_H;
			break;
		}else if (str[i] == 'm'){
			for(j=0; j<=p; j++){
				switch(params[j]/10){/*bold,dim,underline,blink,invert,hidden => unsupported yet */
				#define BIT2BYTE(bit)    ( ((!!(bit&4))<<23) | ((!!(bit&2))<<15) | ((!!(bit&1))<<7) )
				case  0:psvDebugScreenSetFgColor(COLOR_DEFAULT_FG);psvDebugScreenSetBgColor(COLOR_DEFAULT_BG);break;
				case  3:psvDebugScreenSetFgColor(BIT2BYTE(params[j]%10));break;
				case  9:psvDebugScreenSetFgColor(BIT2BYTE(params[j]%10) | 0x7F7F7F7F);break;
				case  4:psvDebugScreenSetBgColor(BIT2BYTE(params[j]%10));break;
				case 10:psvDebugScreenSetBgColor(BIT2BYTE(params[j]%10) | 0x7F7F7F7F);break;
				#undef BIT2BYTE
				}
			}
			break;
		}
	}
	return i;
}




int ScreenInit = 0;
SceUID displayblock[2];
void *fb_base[2];
int fb_idx = 0;

int psvDebugScreenSet() {
	SceDisplayFrameBuf framebuf = {
		.size        = sizeof(framebuf),
		.base        = fb_base[fb_idx],
		.pitch       = SCREEN_WIDTH,
		.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
		.width       = SCREEN_WIDTH,
		.height      = SCREEN_HEIGHT,
	};

	return sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);
}


int psvDebugScreenSetXY(int new_x, int new_y){

	psvDebugScreenCoordY = new_x;
	psvDebugScreenCoordX = new_y;

	return 0;
}

int psvDebugScreenInit(void){

	psvDebugScreenMutex = sceKernelCreateMutex("log_mutex", 0, 0, NULL);



	displayblock[0] = sceKernelAllocMemBlock("SceDisplay0", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCREEN_FB_SIZE, NULL);
	sceKernelGetMemBlockBase(displayblock[0], (void**)&fb_base[0]);

	displayblock[1] = sceKernelAllocMemBlock("SceDisplay1", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCREEN_FB_SIZE, NULL);
	sceKernelGetMemBlockBase(displayblock[1], (void**)&fb_base[1]);


	return 0;

	//return psvDebugScreenSet();
}

void *GetFbBase(void){
	return fb_base[fb_idx];
}

void swap_fb(){
	fb_idx ^= 1;
}

void psvDebugScreenClear(int bg_color){
	psvDebugScreenCoordX = psvDebugScreenCoordY = 0;
	void *FbBase = GetFbBase();
	int i;
	for(i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		((uint32_t*)FbBase)[i] = bg_color;
	}
}

int psvDebugScreenPuts(const char *text){
	int c, i, j, l;
	uint8_t *font;
	uint32_t *vram_ptr;
	uint32_t *vram;

	for (c = 0; text[c] != '\0' ; c++) {
		if (psvDebugScreenCoordX + 8 > SCREEN_WIDTH) {
			psvDebugScreenCoordY += SCREEN_GLYPH_H;
			psvDebugScreenCoordX = 0;
		}
		if (psvDebugScreenCoordY + 8 > SCREEN_HEIGHT) {
			psvDebugScreenClear(psvDebugScreenColorBg);
		}
		if (text[c] == '\n') {
			psvDebugScreenCoordX = 0;
			psvDebugScreenCoordY += SCREEN_GLYPH_H;
			continue;
		} else if (text[c] == '\r') {
			psvDebugScreenCoordX = 0;
			continue;
		} else if ((text[c] == '\e') && (text[c+1] == '[')) { /* escape code (change color, position ...) */
			c+=psvDebugScreenEscape(text+2)+2;
			continue;
		}

		vram = ((uint32_t*)fb_base[fb_idx]) + psvDebugScreenCoordX + psvDebugScreenCoordY * SCREEN_FB_WIDTH;
		//vram = ((uint32_t*)psvDebugScreenFrameBuf.base) + psvDebugScreenCoordX + psvDebugScreenCoordY * SCREEN_FB_WIDTH;

		font = &psvDebugScreenFont[ (int)text[c] * 8];
		for (i = l = 0; i < SCREEN_GLYPH_W; i++, l += SCREEN_GLYPH_W, font++) {
			vram_ptr  = vram;
			for (j = 0; j < SCREEN_GLYPH_W; j++) {
				if ((*font & (128 >> j))) *vram_ptr = psvDebugScreenColorFg;
				else *vram_ptr = psvDebugScreenColorBg;
				vram_ptr++;
			}
			vram += SCREEN_FB_WIDTH;
		}
		psvDebugScreenCoordX += SCREEN_GLYPH_W;
	}

	return c;
}

int psvDebugScreenPrintf(const char *format, ...) {
	char buf[512];

	va_list opt;

	sceKernelLockMutex(psvDebugScreenMutex, 1, NULL);

	va_start(opt, format);
	vsnprintf(buf, sizeof(buf), format, opt);
	va_end(opt);

	psvDebugScreenPuts(buf);

	sceKernelUnlockMutex(psvDebugScreenMutex, 1);

	return 0;
}

void psvDebugScreenPrintf2(int text_x, int text_y, const char *format, ...) {
	char buf[512];

	va_list opt;

	sceKernelLockMutex(psvDebugScreenMutex, 1, NULL);

	va_start(opt, format);
	vsnprintf(buf, sizeof(buf), format, opt);
	va_end(opt);


	psvDebugScreenCoordX = text_x;
	psvDebugScreenCoordY = text_y;

	psvDebugScreenPuts(buf);

	sceKernelUnlockMutex(psvDebugScreenMutex, 1);

	//return ret;
}


#endif