#include <stdint.h>
typedef enum MenuDrawType {
	MENU_TITLE,
	MENU_SUBTITLE,
	MENU_NORMAL
} MenuDrawType;

typedef enum MenuType {
	MENU_NONE,
	MENU_WARNING,
	MENU_BAD,
	MENU_ENTER,
	MENU_BACK,
	MENU_CANCEL,
	MENU_START,
	MENU_DISABLE
	
} MenuType;

typedef enum MenuPaneSide {
	PANE_LEFT,
	PANE_RIGHT,
	PANE_FULLSCREEN
} MenuPaneSide;

typedef enum MenuActive{
	MENU_INACTIVE,
	MENU_ACTIVE
} MenuActive;

typedef enum MenuDir {
	MENU_UP,
	MENU_DOWN
} MenuDir;

typedef struct MenuEntry {
	MenuDrawType draw_type;
	MenuType type;
	int active;
	int index;
	char text[128];
	void *(*click_cb)(void*,int);
	struct MenuEntry *next;
	struct MenuEntry *prev;
} MenuEntry;


typedef struct Menu {
	MenuActive active;
	MenuActive visible;
	int entry_index;
	int entry_top;
	int entry_bottom;
	MenuEntry *entry_selected;
	MenuPaneSide pane;
	MenuEntry *entry_start;
	MenuEntry *entry_end;
} Menu;


Menu *MenuCreate(MenuPaneSide pane);
void MenuDelete(Menu *menu);
MenuEntry *MenuAddEntry(Menu *menu, MenuDrawType draw_type, char *text, void *(*click_cb)(void*,int));
MenuEntry *MenuFindEntry(Menu *menu, int index);
MenuEntry *MenuFindNextEntry(MenuEntry *start_entry, MenuDir dir);
MenuEntry *MenuFindType(Menu *menu, MenuType type);
void MenuDeleteEntry(Menu *menu, int index) ;