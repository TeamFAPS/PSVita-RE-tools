//ShipLog - WarRoom

//Put together by dots_tb
//Created for Kancolle Kai Vita translation and dev team (expecially you senpai ~<3)
//Special thanks to Team_Molecule for Taihen (special thanks to xyz)
//thanks to xerpi for being underrated (and logging functions/netdebug), frangarcj for oclock
//Freakler for common dialog in uriCaller, TheFlow for VitaShell

//Dialog functions: 
// https://github.com/Freakler/vita-uriCaller

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vitasdk.h>
#include <taihen.h>

#include "dialog.h"
#include "menu.h"
#include "../common/settings.h"
#include "../common/parser.h"

char userText[32];
int shown_dial = 0;


void handleDialog(int page, int pos){
	SceCommonDialogStatus status = sceImeDialogGetStatus();
	int tPort = 0;
	if (status == IME_DIALOG_RESULT_FINISHED) {
		SceImeDialogResult result;
		memset(&result, 0, sizeof(SceImeDialogResult));
		sceImeDialogGetResult(&result);

		if (result.button == SCE_IME_DIALOG_BUTTON_CLOSE) {
			status = IME_DIALOG_RESULT_CANCELED;
			sceImeDialogTerm();
			shown_dial = 0;
		} else {
			oslOskGetText(userText);
			switch(page) {
			case 1://Handle network
				switch(pos) {
				case 1://Handle p
					setIP(userText);
					updated(0);
					break;
				case 2://Handle port
					sscanf(userText, "%d", &tPort);
					setPort(tPort);
					updated(0);
					break;
				}
				break;
			}
		}

		sceImeDialogTerm();
		shown_dial = 0;
	}
}
void handleInput(SceCtrlData pad, int *page, int * pos, int max){
	char sPort[5];
	if(pad.buttons & SCE_CTRL_CROSS){
		switch (*page) {
		case 0://Main page
			switch (*pos){
			case 1: 
				shipLogDumpToDisk();
				break;
			case 2:*page = 1;//Network
				*pos = 1;
				break;
			case 3:
				setNet(!getNet());
				updated(0);
				break;
			case 4:
				setFile(!getFile());
				updated(0);
				break;
			case 5: saveConfig(CONFIG_FILE);
					updated(1);
					break;
			}
			break;
		case 1://Network
			switch (*pos) {
			case 1://set ip
				initImeDialog("Please enter an IP:", getIP() , 15, SCE_IME_TYPE_EXTENDED_NUMBER);
				shown_dial=1;				
				break;
			case 2://set port
				sprintf(sPort,"%d",getPort());
				initImeDialog("Please enter an port:", sPort , 5, SCE_IME_TYPE_NUMBER);
				shown_dial=1;
				break;
			case 3: 
				*page = 0;
				*pos = 2;
				break;
			}
			break;
		case 4://First time user
			switch (*pos){
			case 1: 
				*page = 0;
				*pos = 2;
				break;
			}
			break;
		}
		sceKernelDelayThread( 500 * 1000);
	} 
	if(pad.buttons & SCE_CTRL_DOWN){
		if(*pos<max) {
			*pos+=1;
			sceKernelDelayThread( 200 * 1000);
			return;
		}
		
	}
	if(pad.buttons & SCE_CTRL_UP){
		if(*pos>1) {
			sceKernelDelayThread( 200 * 1000);
			*pos-=1;
			return;
		}
	}
	if(pad.buttons & SCE_CTRL_CIRCLE){
		if(*page>0) {
			*pos=*page + 1;
			*page=0;
			sceKernelDelayThread( 500 * 1000);
			return;
		}
	}
	
}

void main() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {
	
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});

	vita2d_init();
	
	SceCtrlData pad;
	memset(&pad, 0, sizeof(pad));

	vita2d_pgf *pgf;
	pgf = vita2d_load_default_pgf();

	int page = 0, pos =1, max = 1;
	if(loadConfig(CONFIG_FILE)<0) {
		//first time configuration
		sceIoMkdir(APP_DATA,0777);
		setIP("192.168.1.1");
		setPort(3333);
		setFile(0);
		setNet(0);
		saveConfig(CONFIG_FILE);
		page = 4;
	}
	//shipLogToFile("ux0:/data/shipLog/test.txt");
	sceClibPrintf("this is a test");
	startdraw();

	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if(shown_dial){
			handleDialog(page,pos);
		} else {
			max = renderMenu(pgf, page, pos);
		}
		updatedraw();
		if(!shown_dial&&pad.buttons){
		handleInput(pad, &page,&pos,max);
		
		}
		
	}
	enddraw();
	sceKernelExitProcess(0);
	return 0;

}