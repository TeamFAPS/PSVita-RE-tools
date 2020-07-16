
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/net/net.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/ime_dialog.h>
#include <psp2/message_dialog.h>
#include <psp2/sysmodule.h>
#include <psp2/power.h>


#include <taihen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugScreen.h"
#include "gxm.h"

#include "NetLoggingMgrInternal.h"
#include "NetLoggingMgr.h"

NetLoggingMgrConfig_t NetLoggingMgrConfig;






static char initparam_buf[0x4000];
static SceNetInitParam initparam;

SceUInt64 tick = 0;
SceUInt64 old_tick = 0;



SceCtrlData ctrl;

uint32_t press_padd = 0;
uint32_t press_key = 0;
uint32_t old_press_key = 0;

void ReadPad(){

	old_press_key = press_key;

	sceCtrlPeekBufferPositive(0, &ctrl, 1);

	press_key = ctrl.buttons;

	press_padd = press_key & (press_key ^ old_press_key);
}

void WaitKeyPress(){

	do{
		ReadPad();
	}while(press_padd == 0);

}




int CallImeDialog(SceImeDialogParam *param){
	int ret;
	
	param->sdkVersion = 0x03150021,
	ret = sceImeDialogInit(param);

	if(ret < 0){
		sceClibPrintf("show dialog failed!: %x\n", ret);
		return ret;
	}

	while(1){
		ret = sceImeDialogGetStatus();
		if(ret < 0){
			break;

		}else if(ret == SCE_COMMON_DIALOG_STATUS_FINISHED){

			SceImeDialogResult result;
			sceClibMemset(&result, 0, sizeof(result));

			sceImeDialogGetResult(&result);

			if(result.button != SCE_IME_DIALOG_BUTTON_CLOSE){
			}
			break;
		}

		gxm_update();
	}

	sceImeDialogTerm();

	return ret;
}

static void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
  int i;
  for (i = 0; src[i]; i++) {
    if ((src[i] & 0xFF80) == 0) {
      *(dst++) = src[i] & 0xFF;
    } else if((src[i] & 0xF800) == 0) {
      *(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
      *(dst++) = (src[i] & 0x3F) | 0x80;
    } else if((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
      *(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
      *(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
      *(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
      *(dst++) = (src[i + 1] & 0x3F) | 0x80;
      i += 1;
    } else {
      *(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
      *(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
      *(dst++) = (src[i] & 0x3F) | 0x80;
    }
  }

  *dst = '\0';
}

int SetServerPort(void){

	int res;
	char ServerPortStrUtf8[6];
	uint16_t ServerPortStr[6];

	SceImeDialogParam param;
	sceClibMemset(&param, 0, sizeof(param)); 
	sceImeDialogParamInit(&param);
	
	param.title = u"Enter Server Port (ex:8080)";
	param.maxTextLength = (sizeof(ServerPortStr)/2)-1;
	param.initialText = u"";
	param.inputTextBuffer = ServerPortStr;
	param.type = SCE_IME_TYPE_NUMBER;
	res = CallImeDialog(&param);
	utf16_to_utf8((const uint16_t *)&ServerPortStr, (uint8_t *)&ServerPortStrUtf8);

	psvDebugScreenClear(COLOR_DEFAULT_BG);
	psvDebugScreenSet();
	
	if(res < 0){ 
		psvDebugScreenPrintf("Error : CallImeDialog failed: %x\n", res);
		goto end;
	}

	NetLoggingMgrConfig.port = atoi(ServerPortStrUtf8);

	if(NetLoggingMgrConfig.port < 1 || NetLoggingMgrConfig.port > UINT16_MAX){ //lol idk?
		psvDebugScreenPrintf("Error : Invalid Port.\n");
		goto end;
	}

	psvDebugScreenPrintf("Set Server Port : Success.\n");

end:

	psvDebugScreenPrintf("\n");
	psvDebugScreenPrintf("please key press\n");

	ReadPad();
	WaitKeyPress();
	ReadPad();
	swap_fb();

	return res;
}

int SetServerIPv4(void){

	int res;
	char ServerIPv4StrUtf8[16];
	uint16_t ServerIPv4Str[16];

	SceImeDialogParam param;
	sceClibMemset(&param, 0, sizeof(param));
	sceImeDialogParamInit(&param);

	param.title = u"Enter Server IPv4 (ex:192.168.0.6)";
	param.maxTextLength = (sizeof(ServerIPv4Str)/2)-1;
	param.initialText = u"";
	param.inputTextBuffer = ServerIPv4Str;
	param.type = SCE_IME_TYPE_EXTENDED_NUMBER;
	res = CallImeDialog(&param);

	psvDebugScreenClear(COLOR_DEFAULT_BG);
	psvDebugScreenSet();
	
	if(res < 0){ 
		psvDebugScreenPrintf("Error : CallImeDialog failed: %x\n", res);
		goto end;
	}
	
	utf16_to_utf8((const uint16_t *)&ServerIPv4Str, (uint8_t *)&ServerIPv4StrUtf8);

	res = sceNetInetPton(SCE_NET_AF_INET, ServerIPv4StrUtf8, &NetLoggingMgrConfig.IPv4);

	if(res != 1){
		psvDebugScreenPrintf("Error : Invalid IPv4.\n");
		goto end;
	}

	psvDebugScreenPrintf("Set Server IPv4 : Success.\n");

end:

	psvDebugScreenPrintf("\n");
	psvDebugScreenPrintf("please key press\n");

	ReadPad();
	WaitKeyPress();
	ReadPad();
	swap_fb();

	return res;
}


typedef struct MenuItem_t{
	int text_x;
	int text_y;
	char text[0x70];
	char *pText;
	int (* callback)(void);
	struct MenuItem_t *next;
} MenuItem_t;

MenuItem_t *old_MenuItem;
MenuItem_t *MenuItemTreeTop;
int menu_text_y = 20;
int menu_item_num = 0;

MenuItem_t *get_menu_item_tree_top(){
	return MenuItemTreeTop;
}

int menu_item_init(){

	old_MenuItem = NULL;
	MenuItemTreeTop = NULL;

	menu_text_y = 20;

	return 0;
}

int add_menu_item(MenuItem_t *MenuItem, const char *text, ...){

	va_list opt;

	MenuItem->text_x = 20;
	MenuItem->text_y = menu_text_y;

	menu_text_y += 10;

	va_start(opt, text);
	vsnprintf(MenuItem->text, 0x70-1, text, opt);
	va_end(opt);

	MenuItem->pText = MenuItem->text;

	//menu_item_num++;

	return 0;
}

int menu_item_text_change(MenuItem_t *MenuItem, const char *text, ...){

	va_list opt;

	va_start(opt, text);
	vsnprintf(MenuItem->text, 0x70-1, text, opt);
	va_end(opt);

	return 0;
}

int _add_menu_item(const char *text, ...){

	va_list opt;

	MenuItem_t *item = (MenuItem_t *)malloc(sizeof(MenuItem_t));
	if(item){
		return 0;
	}

	item->text_x = 20;
	item->text_y = menu_text_y;

	menu_text_y += 10;

	va_start(opt, text);
	vsnprintf(item->text, 0x70-1, text, opt);
	va_end(opt);

	item->pText = item->text;

	item->next = NULL;

	if(old_MenuItem != NULL){
		old_MenuItem->next = item;
		MenuItemTreeTop = item;
	}

	old_MenuItem = item;

/*
	if(old_MenuItem == NULL){
		old_MenuItem = item;
	}else{
		old_MenuItem->next = item;
		old_MenuItem = item;
	}
*/
	menu_item_num++;

	return 0;
}

int free_menu_item(){

	MenuItem_t *old_MenuItem_next = (MenuItem_t *)old_MenuItem->next;

	do{

		free(old_MenuItem);

		old_MenuItem = old_MenuItem_next;

	}while(old_MenuItem);

	old_MenuItem = NULL;

	return 0;
}

int set_menu_item(MenuItem_t *MenuItem, int text_x, int text_y, const char *text, ...){

	va_list opt;

	MenuItem->text_x = text_x;
	MenuItem->text_y = text_y;

	va_start(opt, text);
	vsnprintf(MenuItem->text, 0x70-1, text, opt);
	va_end(opt);

	MenuItem->pText = MenuItem->text;

	return 0;
}


int set_item_callback(MenuItem_t *MenuItem, int (* func)(void)){

	MenuItem->callback = func;

	return 0;
}

int menu_press_up(int display_max, int sel_max, int *sel, int *sel_extra){

	if(press_padd & SCE_CTRL_UP){
		if(*sel == 0 && *sel_extra == 0){
			*sel_extra = sel_max - display_max;
			*sel = display_max - 1;
		}else{
			if(*sel > 0 && *sel_extra >= 0){
				//*sel--;
				*sel = *sel - 1;
			}else{
				//*sel_extra--;
				*sel_extra = *sel_extra - 1;
			}
		}
	}

	return 0;
}

int menu_press_down(int display_max, int sel_max, int *sel, int *sel_extra){

	if(press_padd & SCE_CTRL_DOWN){

		if(*sel == (display_max-1)){
			if((*sel + *sel_extra) < (sel_max-1)){
				//*sel_extra++;
				*sel_extra = *sel_extra + 1;
			}else{
				*sel_extra = 0;
				*sel = 0;
			}
		}else{
			//*sel++;
			*sel = *sel + 1;
		}
	}

	return 0;
}

int menu_press_right(int display_max, int sel_max, int *sel, int *sel_extra){

	if(press_padd & SCE_CTRL_RIGHT){

		if((*sel + 7) < (display_max-1)){
			*sel += 7;
		}else{
			*sel += ((display_max - 1) - *sel);

			*sel_extra += ((*sel + *sel_extra + 7) < (sel_max-1)) ? 7 : ((sel_max-1) - (*sel + *sel_extra));
		}
	}

	return 0;
}

int menu_press_left(int display_max, int sel_max, int *sel, int *sel_extra){

	if(press_padd & SCE_CTRL_LEFT){
		if((*sel - 7) > 0){

			*sel -= 7;
		}else{
			*sel -= *sel;

			*sel_extra -= ((*sel_extra - 7) > 0) ? 7 : *sel_extra;
		}
	}

	return 0;
}


int menu_press_DPad(int display_max, int sel_max, int *sel, int *sel_extra){

	if(press_padd & SCE_CTRL_UP){
		if(*sel == 0 && *sel_extra == 0){
			*sel_extra = sel_max - display_max;
			*sel = display_max - 1;
		}else{
			if(*sel > 0 && *sel_extra >= 0){
				*sel = *sel - 1;
			}else{
				*sel_extra = *sel_extra - 1;
			}
		}
		goto end;
	}

	if(press_padd & SCE_CTRL_DOWN){

		if(*sel == (display_max-1)){
			if((*sel + *sel_extra) < (sel_max-1)){
				*sel_extra = *sel_extra + 1;
			}else{
				*sel_extra = 0;
				*sel = 0;
			}
		}else{
			*sel = *sel + 1;
		}
		goto end;
	}

	if(press_padd & SCE_CTRL_RIGHT){

		if((*sel + 7) < (display_max-1)){
			*sel += 7;
		}else{
			*sel += ((display_max - 1) - *sel);

			*sel_extra += ((*sel + *sel_extra + 7) < (sel_max-1)) ? 7 : ((sel_max-1) - (*sel + *sel_extra));
		}
		goto end;
	}

	if(press_padd & SCE_CTRL_LEFT){
		if((*sel - 7) > 0){

			*sel -= 7;
		}else{
			*sel -= *sel;

			*sel_extra -= ((*sel_extra - 7) > 0) ? 7 : *sel_extra;
		}
		goto end;
	}

end:
	return 0;
}


int QafSettings(void){

	int sel = 0;
	int sel_max = 2;

	while(1){

		psvDebugScreenPrintf2(0,  20 + (10 * sel),  "*");

		psvDebugScreenPrintf2(0,   0,  "-- Qaf Setting --");

		psvDebugScreenPrintf2(20, 20,  "kernel debug print : %s", (NetLoggingMgrConfig.flags & NLM_CONFIG_FLAGS_BIT_QAF_DEBUG_PRINTF) ? "Enable" : "Disable");
		psvDebugScreenPrintf2(20, 30,  "Back");

		psvDebugScreenSet();
		swap_fb();
		psvDebugScreenClear(COLOR_DEFAULT_BG);

		WaitKeyPress();

		if(press_padd & SCE_CTRL_UP){
			if(sel == 0){
				sel = sel_max - 1;
			}else{
				sel--;
			}
		}

		if(press_padd & SCE_CTRL_DOWN){
			if(sel == (sel_max-1)){
				sel = 0;
			}else{
				sel++;
			}
		}

		if(press_padd & SCE_CTRL_CIRCLE){
			if(sel == 0){

				NetLoggingMgrConfig.flags ^= NLM_CONFIG_FLAGS_BIT_QAF_DEBUG_PRINTF;

			}else if(sel == (sel_max-1)){
				break;
			}
		}

	}

	ReadPad();

	return 0;
}

int UpdateConfig(void){

	int search_unk[2];
	SceUID res;

	psvDebugScreenSet();

	res = _vshKernelSearchModuleByName("NetLoggingMgr", search_unk);
	if(res < 0){

		psvDebugScreenPrintf("Error : NetLoggingMgr not loaded\n");
		goto end;

	}

	res = NetLoggingMgrUpdateConfig(&NetLoggingMgrConfig);
	if(res < 0){

		psvDebugScreenPrintf("Update Config Error : 0x%X\n", res);
		goto end;

	}

	psvDebugScreenPrintf("Update Config Success.\n");

end:

	psvDebugScreenPrintf("\n");
	psvDebugScreenPrintf("please key press\n");

	WaitKeyPress();
	ReadPad();
	swap_fb();

	return 0;
}

int SaveConfig(void){

	SceUID fd;

	psvDebugScreenSet();

	fd = sceIoOpen("ur0:data/NetLoggingMgrConfig.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);

	if(fd < 0){
		psvDebugScreenPrintf("Error : File Open Error, 0x%08X.\n", fd);
		goto end;
	}
	sceIoWrite(fd, &NetLoggingMgrConfig, sizeof(NetLoggingMgrConfig_t));
	sceIoClose(fd);

	psvDebugScreenPrintf("Save Config Success.\n");

end:

	psvDebugScreenPrintf("\n");
	psvDebugScreenPrintf("please key press\n");

	WaitKeyPress();
	ReadPad();
	swap_fb();

	return 0;
}

int MainMenu(){

	int sel = 0;
	int sel_max = 7;
	int sel_idx = 0;
	int set_idx = 0;
	MenuItem_t MenuItem[sel_max];

	int display_max = (sel_max > 52) ? 52 : sel_max;
	int sel_extra = 0;
	int display_x, display_y;

	for(int i=0;i<sel_max;i++){
		sceClibMemset(&MenuItem[i], 0, sizeof(MenuItem_t));
	}

	menu_item_init();

	add_menu_item(&MenuItem[set_idx++], "Set Server IPv4");
	add_menu_item(&MenuItem[set_idx++], "Set Server Port");
	add_menu_item(&MenuItem[set_idx++], "Qaf Settings");
	add_menu_item(&MenuItem[set_idx++], "Update Config");
	add_menu_item(&MenuItem[set_idx++], "Save Config");
	add_menu_item(&MenuItem[set_idx++], "System Reboot");
	add_menu_item(&MenuItem[set_idx++], "Exit");

	set_idx = 0;

	set_item_callback(&MenuItem[set_idx++], SetServerIPv4);
	set_item_callback(&MenuItem[set_idx++], SetServerPort);
	set_item_callback(&MenuItem[set_idx++], QafSettings);
	set_item_callback(&MenuItem[set_idx++], UpdateConfig);
	set_item_callback(&MenuItem[set_idx++], SaveConfig);
	set_item_callback(&MenuItem[set_idx++], scePowerRequestColdReset);
	set_item_callback(&MenuItem[set_idx++], 0);

	while(1){

		sel_idx = sel + sel_extra;

		psvDebugScreenPrintf2(0,  20 + (10 * sel),  "*");

		psvDebugScreenPrintf2(0,   0,  "-- main menu --");

		for(int i=0;i<display_max;i++){

			display_x = MenuItem[i+sel_extra].text_x;
			display_y = MenuItem[i+sel_extra].text_y - (10 * sel_extra);

			psvDebugScreenPrintf2(display_x, display_y, MenuItem[i + sel_extra].pText);

		}

		psvDebugScreenSet();
		swap_fb();
		psvDebugScreenClear(COLOR_DEFAULT_BG);

		WaitKeyPress();

		menu_press_DPad(display_max, sel_max, &sel, &sel_extra);

		if(press_padd & SCE_CTRL_CIRCLE){

			if((void *)(MenuItem[sel_idx].callback) != 0){
				MenuItem[sel_idx].callback();

			}else{
				break;
			}
			continue;
		}

	}



	return 0;
}

int CreateConfigFile(){

	SceIoStat stat;

	int res = sceIoGetstat("ur0:data/NetLoggingMgrConfig.bin", &stat);

	if(res < 0 || (uint32_t)(stat.st_size) != sizeof(NetLoggingMgrConfig_t)){

		const char magic[4] = {'N', 'L', 'M', '\0'};

		memcpy(&NetLoggingMgrConfig.magic, magic, 4);

		SceUID fd = sceIoOpen("ur0:data/NetLoggingMgrConfig.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);

		sceIoWrite(fd, &NetLoggingMgrConfig, sizeof(NetLoggingMgrConfig_t));
		sceIoClose(fd);
	}

	return 0;
}

int ReadConfig(){

	const char magic[4] = {'N', 'L', 'M', '\0'};

	SceUID res;
	SceUID fd;

	int search_unk[2];

	res = _vshKernelSearchModuleByName("NetLoggingMgr", search_unk);
	if(res > 0){

		NetLoggingMgrReadConfig(&NetLoggingMgrConfig);
		goto end;
	}

	fd = sceIoOpen("ur0:data/NetLoggingMgrConfig.bin", SCE_O_RDONLY, 0);

	sceIoRead(fd, &NetLoggingMgrConfig, sizeof(NetLoggingMgrConfig_t));
	sceIoClose(fd);

	if(memcmp(&NetLoggingMgrConfig.magic, magic, 4) != 0){
		CreateConfigFile();
	}

end:
	return 0;
}

int main(int argc, char *argv[]){

	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});

	gxm_init();
	psvDebugScreenInit();

	initparam.memory = &initparam_buf;
	initparam.size = sizeof(initparam_buf);
	initparam.flags = 0;

	sceNetInit(&initparam);

	sceIoMkdir("ur0:data/", 0666);

	CreateConfigFile();

	ReadConfig();

	MainMenu();

	gxm_fini();

	return 0;
}
