#include <vitasdk.h>
#include <stdio.h>
#include <stdlib.h>


#include "menu.h"
#include "sfo.h"
#include "game.h"
#include "utils.h"
#include "dump.h"
#include "status.h"
#include "credit.h"

static GameEntry *games;
static int games_amt = 0,
			games_index = 0,
			system_entries = 0,
			menu_start_index = 0;

int getPathDevice(char *path, const char *fmt, const char *titleid) {
	for(int i = 0; i < DEVICES_AMT; i++) {
		snprintf(path, PATH_MAX, fmt, DEVICES[i], titleid);
		if(checkExists(path) == 0) 
			return 1;
	}
	return 0;
}

int checkIfPFS(const char *titleid) {
	char current_path[128];
	
	return (getPathDevice(current_path, "%s/app/%s/sce_pfs", titleid) || getPathDevice(current_path, "%s/patch/%s/sce_pfs", titleid));
}


char *getSfoBuffer(const char *titleid) {
	char current_path[PATH_MAX];
	if(getPathDevice(current_path, "%s/app/%s/sce_sys/param.sfo", titleid) || getPathDevice(current_path, "%s/patch/%s/sce_sys/param.sfo", titleid)) {
		
		SceSize sz = getFileSize(current_path);
		if(sz < sizeof(SfoHeader))
			return 0;
		char *sfo_buffer = malloc(sz);
		ReadFile(current_path, sfo_buffer, sz);
		return sfo_buffer;
	}
	return NULL;
}

int gameCount() {
	SceUID dfd = sceIoDopen("ur0:/appmeta");
	SceIoDirent dir;
	int res;
	if (dfd < 0) 
		return 0;
	do {
		if ((res = sceIoDread(dfd, &dir)) > 0 && checkIfPFS(dir.d_name) )
			games_amt++;
	} while (res > 0);
	sceIoDclose(dfd);
	return games_amt;
}

void *gameEntryCB(void *menu, int index) {
	if(index - menu_start_index < 0) 
		return (void*)dumpGenerateModuleMenu(menu,  system_entries + 1 + index - menu_start_index);
	else if(index - menu_start_index <= games_index) 
		return (void*)dumpGenerateModuleMenu(menu, index - menu_start_index + system_entries);
	return menu;
}

GameEntry *gameGetGameFromIndex(int index) {
	return &games[index];
}


void *gameStatusCB(void *menu, int index) {
	return statusGenerateMenu(menu);
}
void *gameCreditCB(void *menu, int index) {
	return creditGenerateMenu(menu);
}

void gameAdd(Menu *menu, char *titleid, GameType type)  {
	games[games_index].menu_index = MenuAddEntry(menu, MENU_NORMAL, games[games_index].title, gameEntryCB)->index;
	strncpy(games[games_index].titleid, titleid, 20); 
	games[games_index].type = type;
	games_index++;
}
Menu *gameGenerateMenu() {
	Menu *menu = MenuCreate(PANE_LEFT);
	size_t sz = sizeof(GameEntry) * (gameCount() + 6);
	games = malloc(sz);
	MenuAddEntry(menu, MENU_TITLE, "FRENCH-AMERICAN GAME DECRYPTER", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "REV ENG BY @CelesteBlue123 (HE IS AMAZING)", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "DEVELOPED BY @dots_tb", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Decrypt modules in list", gameStatusCB)->type = MENU_START;
	MenuAddEntry(menu, MENU_NORMAL, "Further Credits", gameCreditCB)->type = MENU_BACK;
	MenuAddEntry(menu, MENU_TITLE, "SYSTEM:", NULL);
	strncpy(games[games_index].title, "os0:", 128);
	gameAdd(menu, "SYSTEM", SELF_SYSTEM);
	system_entries++;
	strncpy(games[games_index].title, "vs0:", 128);
	gameAdd(menu, "SYSTEM", SELF_SYSTEM);
	system_entries++;
	strncpy(games[games_index].title, "ux0:/os0_em", 128);
	gameAdd(menu, "SYSTEM", SELF_SYSTEM);
	system_entries++;
	strncpy(games[games_index].title, "ux0:/vs0_em", 128);
	gameAdd(menu, "SYSTEM", SELF_SYSTEM);
	system_entries++;
	strncpy(games[games_index].title, "ux0:/app_em", 128);
	gameAdd(menu, "SYSTEM", SELF_SYSTEM);
	system_entries++;
	strncpy(games[games_index].title, "ux0:/patch_em", 128);
	gameAdd(menu, "SYSTEM", SELF_SYSTEM);
	system_entries++;
	MenuAddEntry(menu, MENU_TITLE, "PFS TITLES FOUND:", NULL);
	menu_start_index = menu->entry_index;
	SceUID dfd = sceIoDopen("ur0:/appmeta");
	SceIoDirent dir;
	int res;
	if (dfd < 0) 
		return NULL;
	do {
		memset(&dir, 0, sizeof(SceIoDirent));
		if ((res = sceIoDread(dfd, &dir)) >= 0 && checkIfPFS(dir.d_name)) {
			char *sfo_buffer= getSfoBuffer(dir.d_name);
			if(!sfo_buffer)
				continue;
			int res = getSfoString(sfo_buffer, "STITLE", games[games_index].title, 128);
			free(sfo_buffer);
			if(res == 0) {
				char *newline;
				while((newline = strchr(games[games_index].title,'\n')) != NULL)
					*newline  = ' ';
				gameAdd(menu, dir.d_name, SELF_NP);
			}
			
		}
	} while (res > 0);
	sceIoDclose(dfd);

	return menu;
}