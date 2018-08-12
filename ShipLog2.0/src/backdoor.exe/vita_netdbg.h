#include <vitasdkkern.h>
#include "backdoor_exe.h"
int init_netdbg(config_file config);
void fini_netdbg();
int get_socket();
int is_connected_netdbg();
int send(char *buf, int len);
