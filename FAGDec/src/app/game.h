#include <sys/syslimits.h>
typedef enum GameType {
	SELF_NP,
	SELF_SYSTEM
} GameType;
typedef struct GameEntry {
	char title[128];
	char titleid[20];
	char path[PATH_MAX];
	GameType type;
	int menu_index;
} GameEntry;

GameEntry *gameGetGameFromIndex(int index);