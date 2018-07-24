#include <stdlib.h>
#include <vita2d.h>
#include <vitasdk.h>
#include "menu.h"
#include "draw.h"


static vita2d_pgf *pgf;
void drawInit() {
	vita2d_init();
	pgf = vita2d_load_default_pgf();
	drawStart();
}

void drawStart(){
	vita2d_start_drawing();
	vita2d_clear_screen();
}

void drawEnd(){
	vita2d_end_drawing();
	vita2d_common_dialog_update();
	vita2d_swap_buffers();
	sceDisplayWaitVblankStart();
}

void drawUpdate(){
	drawEnd();
	drawStart();
}

void drawMenu(Menu *menu) {
	if(!menu->visible)
		return;
	MenuEntry *menu_entry = menu->entry_start;
	int x = 0,
		y = 0;
	if(menu->pane == PANE_RIGHT) {
		x = 600;
		
	}
	if(menu->entry_selected) {
		if(menu->entry_selected->index < menu->entry_top)
			menu->entry_top--;
	} else
		menu->entry_top = 0;
	if(menu->entry_top < 0)
		menu->entry_top = 0;
	while(menu_entry != NULL) {
		if(menu_entry->index < menu->entry_top) {
			menu_entry = menu_entry->next;
			continue;
		}
		if(y + 45 < 540)
			menu->entry_bottom = menu_entry->index;
				
		if(menu_entry->index > menu->entry_bottom) {
			vita2d_pgf_draw_textf(pgf, x, y + 20, INACTIVE_COL, 1.0f, "... %d MORE", menu->entry_index - menu_entry->index);
			break;
		}
		unsigned int color = INACTIVE_COL;
		float font = 1.0f;
		int old_x = x;
		switch((int)menu_entry->draw_type) {
			case MENU_NORMAL:
				y += 20;
				break;
			case MENU_TITLE:
				font = 1.2f;
				y += 25;
				break;
			case MENU_SUBTITLE:
				font = 0.8f;
				y += 15;
				break;
		}
		switch((int)menu_entry->type) {
			case MENU_BACK:
				vita2d_pgf_draw_text(pgf, x, y, color, 1.0f, "[O]");
				x+=25;
				break;
			case MENU_BAD:
				color = RGBA8(255,0,0,225);
				vita2d_pgf_draw_text(pgf, x, y, color, 1.0f, "[#]");
				x+=25;
				break;
			case MENU_WARNING:
				color = RGBA8(255,162,68,225);
				vita2d_pgf_draw_text(pgf, x, y, color, 1.0f, "[!]");
				x+=25;
				break;
			case MENU_START: 
				vita2d_pgf_draw_text(pgf, x, y, color, 1.0f, "[START]");
				x+=85;
				break;
		}	
		if(menu->entry_selected ) 
			color = menu->entry_selected->index == menu_entry->index ? ACTIVE_COL : color;
		if(menu->pane != PANE_FULLSCREEN) {
			if(menu->active) 
				vita2d_draw_rectangle(x, 535, 480, 5, ACTIVE_COL);
			char text[50];
			strncpy(text, menu_entry->text, sizeof(text));
			text[50]=0;
			vita2d_pgf_draw_text(pgf, x, y, color, font, text);
		} else 
			vita2d_pgf_draw_text(pgf, x, y, color, font, menu_entry->text);
		

		x = old_x;
		menu_entry = menu_entry->next;
	}
	
	if(menu->entry_selected && menu->entry_selected->index > menu->entry_bottom)
				menu->entry_top++;	
			//vita2d_pgf_draw_text(pgf, 0, 20, RGBA8(255,0,0,225), 1.0f, "(EVAL COPY) DO NOT LEAK");
}