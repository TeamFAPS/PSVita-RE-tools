#include <vitasdk.h>
#include "draw.h"
#include "menu.h"
#include "game.h"
#include "dump.h"

#define printf sceClibPrintf



void main() __attribute__ ((weak, alias ("module_start")));

void handleInput(Menu **menu, SceCtrlData pad) {
	if((*menu)->active && (*menu)->entry_selected!=NULL) {
		if(pad.buttons & SCE_CTRL_DOWN){
				(*menu)->entry_selected = MenuFindNextEntry((*menu)->entry_selected, MENU_DOWN);
				if((*menu)->entry_selected->index > (*menu)->entry_bottom)
					(*menu)->entry_top++;
				sceKernelDelayThread(175 * 1000);
			
		}
		if(pad.buttons & SCE_CTRL_UP){
			MenuEntry *menu_entry = MenuFindNextEntry((*menu)->entry_selected, MENU_UP);
			if((*menu)->entry_selected->index != menu_entry->index) {
				(*menu)->entry_selected = menu_entry;
				if(menu_entry->index < (*menu)->entry_top) 
					(*menu)->entry_top--;
			} else if(menu_entry->index > 0) 
				(*menu)->entry_top--;
			sceKernelDelayThread(175 * 1000);
			
		}
		
		if(pad.buttons & SCE_CTRL_CROSS){
				if((*menu)->entry_selected->click_cb != NULL)
					*menu = (Menu*)(*menu)->entry_selected->click_cb((*menu), (*menu)->entry_selected->index);
				(*menu)->active = 1;
				sceKernelDelayThread( 200 * 1000);
		}
		if(pad.buttons & SCE_CTRL_CIRCLE){
			MenuEntry *menu_entry = MenuFindType((*menu), MENU_BACK);
			if(menu_entry && menu_entry->click_cb != NULL)
				*menu = (Menu*)menu_entry->click_cb((*menu), menu_entry->index);
			(*menu)->active = 1;
			sceKernelDelayThread( 200 * 1000);
		}
		if(pad.buttons & SCE_CTRL_START){
			MenuEntry *menu_entry = MenuFindType((*menu), MENU_START);
			if(menu_entry && menu_entry->click_cb != NULL)
				*menu = (Menu*)menu_entry->click_cb((*menu), menu_entry->index);
			(*menu)->active = 1;
			sceKernelDelayThread( 200 * 1000);
		}
	}
}

void handleSpecInput(Menu *menu_l, Menu *menu_r, SceCtrlData pad) {
	menu_l->pane = PANE_LEFT;
	menu_r->pane = PANE_RIGHT;
			menu_r->visible = 1;
			menu_l->visible = 1;
	if(pad.buttons & SCE_CTRL_LEFT){
			if(menu_l->active) {
				menu_l->pane = PANE_FULLSCREEN;
				menu_r->visible = 0;
			} else 
				sceKernelDelayThread( 200 * 1000);
				menu_l->active = MENU_ACTIVE;
				menu_r->active = MENU_INACTIVE;		
		}
		if(pad.buttons & SCE_CTRL_RIGHT){
			if(menu_r->active) {
				menu_r->pane = PANE_FULLSCREEN;
				menu_l->visible = 0;
			} else 
				sceKernelDelayThread( 200 * 1000);
			menu_l->active = MENU_INACTIVE;
			menu_r->active = MENU_ACTIVE;
		}
		if(pad.buttons & SCE_CTRL_CIRCLE) {
			if(menu_r->active && menu_r->entry_selected){
				sceKernelDelayThread( 200 * 1000);
				if(menu_r->entry_selected->type == MENU_WARNING) 
					MenuDeleteEntry(menu_r, menu_r->entry_selected->index);
				else
					menu_r->entry_selected->type = MENU_WARNING;
				
			}
		}
}
int module_start(int argc, const char *argv[]) {
	drawInit();
	drawStart(); 
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});

	SceCtrlData pad;
	memset(&pad, 0, sizeof(pad));
	Menu *game_menu = gameGenerateMenu();
	game_menu->active = 1;
	Menu *dump_menu = dumpGenerateMenu();
	
	while(1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		handleInput(&game_menu, pad);
		handleInput(&dump_menu, pad);
		handleSpecInput(game_menu, dump_menu, pad);
		drawMenu(game_menu);
		drawMenu(dump_menu);
		drawUpdate();
	}
	drawEnd();
	sceKernelExitProcess(0);
	return 0;
}