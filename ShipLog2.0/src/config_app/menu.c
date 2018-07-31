//Menu system form a while back, made by yasen and dots_tb
#include "menu.h"
#include <vitasdk.h>
#include "../common/settings.h"

int renderMenu(vita2d_pgf *pgf, int page, int pos){
	int entries = 1;
	#define TITLE(TEXT)\
	vita2d_pgf_draw_text(pgf, 30, 45,TITLE_COL, 1.2f, (TEXT));
	#define SUB(TEXT)\
	vita2d_pgf_draw_text(pgf, 30, 65,SUB_COL, 0.8f, (TEXT));
	#define MENU_OPTION_F(TEXT,...)\
	vita2d_pgf_draw_textf(pgf, 30, 70+25*entries, pos == entries++ ? ACTIVE_COL : INACTIVE_COL, 1.0f, (TEXT),__VA_ARGS__);
	#define MENU_OPTION(TEXT)\
	vita2d_pgf_draw_text(pgf, 30, 70+25*entries, pos == entries++ ? ACTIVE_COL : INACTIVE_COL, 1.0f, (TEXT));
	switch(page) {
		case 0:
			TITLE("SHIPLOG - WAR ROOM");
			SUB("Put together by dots_tb for kancolle kai translation team");
			MENU_OPTION_F("DUMP LOG BUFFER TO DISK: %d", getLengthOfBuffer());
			MENU_OPTION("NETWORK CONFIGURATION");
			MENU_OPTION_F("Enable Net: \"%d\"",getNet());
			MENU_OPTION_F("Enable File Logging (WARNING, READ MANUAL): \"%d\"",getFile());
			if(!isUpdated()){
				MENU_OPTION("SAVE (!UNSAVED CHANGES!)");
			} else {
				MENU_OPTION("SAVED");
			}
			
			break;
		case 1:
			TITLE("NETWORK CONFIGURATION");
			SUB("Change IP and Port.");
			MENU_OPTION_F("IP: \"%s\"",getIP());
			MENU_OPTION_F("PORT: \"%d\"",getPort());
			MENU_OPTION("<Back");
			MENU_OPTION_F("ON PC(AFTER RESTART): TYPE \"nc -l -p %d\"",getPort());
			break;
		case 4:
			TITLE("WELCOME TO SHIPLOG");
			SUB("Please take your time to setup this client's configuration.");
			MENU_OPTION("OK");
			break;
		case 5:
			TITLE("PLEASE INSTALL CORRECTLY");
			SUB("Something something error");
			break;
	}
	return entries - 1;
}
