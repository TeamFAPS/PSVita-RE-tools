#include <stdlib.h>
#include <string.h>
#include "menu.h"

Menu *MenuCreate(MenuPaneSide pane) {
	Menu *menu = malloc(sizeof(Menu));
	memset(menu, 0, sizeof(Menu));
	menu->pane = pane;
	menu->entry_selected = NULL;
	menu->entry_index = 0;
	menu->entry_top = 0;
	menu->visible = 1;
	menu->entry_start = NULL;
	menu->entry_end = NULL;
	
	return menu;
}

void MenuDelete(Menu *menu) {
	MenuEntry *menu_entry = menu->entry_start;
	while(menu_entry->next != NULL) {
		menu_entry = menu_entry->next;
		free(menu_entry->prev);
	}
	free(menu_entry);
	free(menu);
}

MenuEntry *MenuAddEntry(Menu *menu, MenuDrawType draw_type, char *text, void *(*click_cb)(void*,int)) {
	MenuEntry *menu_entry = malloc(sizeof(MenuEntry));
	memset(menu_entry, 0, sizeof(MenuEntry));
	menu_entry->next = NULL;
	menu_entry->active = 0;
	menu_entry->type = MENU_NONE;
	menu_entry->prev = menu->entry_end;
	menu_entry->draw_type = draw_type;
	menu_entry->click_cb = click_cb;
	menu_entry->index = menu->entry_index++;
	
	strncpy(menu_entry->text, text, sizeof(menu_entry->text));
	if(menu->entry_end != NULL) 
		menu->entry_end->next = menu_entry;
	else if(menu->entry_start == NULL) 
		menu->entry_start = menu_entry;
	menu->entry_end = menu_entry;
	if(draw_type != MENU_NORMAL) {
		menu_entry->type = MENU_DISABLE;
	} else if(menu->entry_selected == NULL)
		menu->entry_selected = menu_entry;
	return menu_entry;
}

MenuEntry *MenuFindEntry(Menu *menu, int index) {
	if(index > menu->entry_index)
			return NULL;
	MenuEntry *menu_entry = menu->entry_start;
	while(menu_entry != NULL) {
		if(menu_entry->index == index)
			return menu_entry;
		menu_entry = menu_entry->next;
	}
	return NULL;
}

MenuEntry *MenuFindType(Menu *menu, MenuType type) {
	MenuEntry *menu_entry = menu->entry_end;
	while(menu_entry != NULL) {
		if(menu_entry->type == type)
			return menu_entry;
		menu_entry = menu_entry->prev;
	}
	return NULL;
}

MenuEntry *MenuFindNextEntry(MenuEntry *start_entry, MenuDir dir) {
	MenuEntry *next_entry = start_entry;
	switch(dir) {
		case MENU_DOWN:
			while(next_entry->next != NULL) { 
				next_entry = next_entry->next;
				if(next_entry->type != MENU_DISABLE)
					return next_entry;
			}
			if(next_entry->next == NULL && next_entry->type == MENU_DISABLE)
				return start_entry;
			break;
		case MENU_UP:
				while(next_entry->prev != NULL) {
					next_entry = next_entry->prev;
					if(next_entry->type != MENU_DISABLE)
						return next_entry;
				}
				if(next_entry->prev == NULL && next_entry->type == MENU_DISABLE)
					return start_entry;
			break;
	}
	return start_entry;
}
void MenuDeleteEntry(Menu *menu, int index) {
	if(index > menu->entry_index)
			return;
	MenuEntry *menu_entry = menu->entry_start;
	while(menu_entry != NULL) {
		if(menu_entry->index == index) {
			if(menu->entry_selected 
				&& menu->entry_selected->index == index 
				&& (menu->entry_selected = MenuFindNextEntry(menu_entry, MENU_UP))->index == index) {
				if(menu->entry_selected = MenuFindNextEntry(menu_entry, MENU_DOWN))
					menu->entry_selected = NULL;
			}
			if(menu_entry->prev)
				menu_entry->prev->next = menu_entry->next;
			else
				menu->entry_start = menu_entry->next;
			MenuEntry *next_entry = menu_entry->next;
			if(next_entry) 
				next_entry->prev = menu_entry->prev;
			else
				menu->entry_end = menu_entry->prev;
			free(menu_entry);
			menu->entry_index--;
			menu_entry = next_entry;
			continue;
		}
		if(menu_entry->index > index)
			menu_entry->index-=1;
		menu_entry = menu_entry->next;
	}
			
	
}