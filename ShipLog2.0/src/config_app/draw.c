
#include<psp2/display.h>
#include "draw.h"


void startdraw (){
	vita2d_start_drawing();
	vita2d_clear_screen();
}

void enddraw(){
	vita2d_end_drawing();
	vita2d_common_dialog_update();
	vita2d_swap_buffers();
	sceDisplayWaitVblankStart();
}

void updatedraw(){
	enddraw();
	startdraw();
}
