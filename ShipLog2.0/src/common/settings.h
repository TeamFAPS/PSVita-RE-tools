#include <backdoor_exe.h>

#define APP_DATA "ux0:data/shipLog/"
#define KPLUGIN_NAME "backdoor_exe.skprx"

#define CONFIG_FILE APP_DATA "config"
#define KPLUGIN_FILE PLUGIN_DIR KPLUGIN_NAME

void setIP(const char *ip);
char *getIP();
void setPort(int port);
int getPort();
int isUpdated();
void updated(int bol);
int getLengthOfBuffer();
void setFile(int file);
int getFile();
void setNet(int net);
int getNet();