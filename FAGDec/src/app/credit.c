#include <vitasdk.h>
#include <stdio.h>
#include <stdlib.h>


#include "menu.h"
static Menu *parent_menu;
void *creditsBackCB(void *menu, int index) {
	MenuDelete(menu);
	return (void*)parent_menu;
}

Menu *creditGenerateMenu(void *p_menu) {
	parent_menu = (Menu*)p_menu;
	Menu *menu  = MenuCreate(PANE_LEFT);
	MenuAddEntry(menu, MENU_TITLE, "FRENCH-AMERICAN GAME DECRYPTER", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "THE FREEDOM TO DECRYPT thanks to:", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@Celesteblue123", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Rev Ur Engs and Kplugin work", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@dots_tb", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Application you(')r(e) using, minimal RE, and integration", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "(thinking of new ways to shill repatch)", NULL);
	MenuAddEntry(menu, MENU_TITLE, "NoPayStation Team", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@juliosueiras", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Head tester, NPS megacontributor, His wads of cash and time", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Radziu @AluProductions", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Public Relations (aka Top Shill) and garbage 9gag memes", NULL);
	MenuAddEntry(menu, MENU_TITLE, "Special thanks:", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Motoharu", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Good Rev ur Engs, Good Dev, and Good Tastes", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "TheFloW", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "NoNpDRM, Code to steal, more fragmentation than android", NULL);
	MenuAddEntry(menu, MENU_TITLE, "Anti-Mai Propaganda Team", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "xness151x", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "jeff7360", NULL);
	MenuAddEntry(menu, MENU_TITLE, "Testing Team", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Hosted on the NEW SilicaServer 2.0 Tm", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@Nkekev", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@froid_san", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Fuzzy-Predator", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "That One Otaku Gamer!", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "amadeus (He gives the best footjobs)", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "<your name here>", NULL);
	MenuAddEntry(menu, MENU_TITLE, "Obligatory:", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@zecoxao(zex), TeamMolecule, vitasdk/wiki, @nyaasen (sys), VitaPiracy, Modders, These USA", NULL);
	MenuAddEntry(menu, MENU_TITLE, "No thanks to:", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "@skgleba @coderx31", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Ben \"Chairman\" Mai [socialist/communist (not the good one like celeste though)]", NULL);
	MenuAddEntry(menu, MENU_TITLE, "Obituary:", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Pray for those we have lost:", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "MaiDumper 2016-2018", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "/r/VitaMai 2018-2018", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Fuzzy's Time 2018-2018", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Silica's butthole 2001-2018", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "People who actually updated.", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Switch Users", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Amen.", NULL);
	MenuAddEntry(menu, MENU_TITLE, "And always, #LetMaiDie and we await your MODS", NULL);
	MenuAddEntry(menu, MENU_SUBTITLE, "Vita scene has a great ATMOSPHERE!", NULL);
	MenuAddEntry(menu, MENU_NORMAL, "Back to Main", creditsBackCB)->type = MENU_BACK;	
	MenuAddEntry(menu, MENU_TITLE, "LOG:", NULL);
	return menu;
}