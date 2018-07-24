#include <vitasdk.h>
#include <stdio.h>
#include <stdlib.h>


#include "menu.h"
#include "game.h"
#include "utils.h"
#include "dump.h"

static Menu *parent_menu;
static Menu *status_menu;

void *statusBackCB(void *menu, int index) {
	return (void*)parent_menu;
}

void *statusStartElfCB(void *menu, int index) {
	dumpStart(DUMP_ELF);
	return (void*)menu;
}

void *statusStartSelfCB(void *menu, int index) {
	dumpStart(DUMP_SELF);
	return (void*)menu;
}

#define LOG_PATH "ux0:/FrAmGamesDec/log.txt"
void statusAddLog(MenuType level,const char *text, ...) {
	char log_buffer[128];
	va_list args;
    va_start(args, text);
    vsprintf(log_buffer, text, args);
    va_end(args);
	MenuEntry *menu_entry = MenuAddEntry(status_menu, MENU_NORMAL, log_buffer, NULL);	
	menu_entry->type = level;
	if(level != MENU_NONE) {
		status_menu->entry_selected = menu_entry;
		
	}
	printf("[%i]%s\n", level, log_buffer);
}

Menu *statusGenerateMenu(void *p_menu) {
	parent_menu = (Menu*)p_menu;
	if(!status_menu)
		status_menu = MenuCreate(PANE_LEFT);
	MenuAddEntry(status_menu, MENU_TITLE, "FRENCH-AMERICAN GAME DECRYPTER", NULL);
	MenuAddEntry(status_menu, MENU_SUBTITLE, "REV ENG BY @CelesteBlue123 (LIKE REALLY AMAZING <3)", NULL);
	MenuAddEntry(status_menu, MENU_SUBTITLE, "DEVELOPED BY @dots_tb", NULL);
	MenuAddEntry(status_menu, MENU_NORMAL, "START DECRYPT(ELF)", statusStartElfCB);
	MenuAddEntry(status_menu, MENU_NORMAL, "START DECRYPT(SELF)", statusStartSelfCB)->type = MENU_START;
	MenuAddEntry(status_menu, MENU_NORMAL, "Back to Main", statusBackCB)->type = MENU_BACK;	
	MenuAddEntry(status_menu, MENU_TITLE, "LOG:", NULL);
	return status_menu;
}