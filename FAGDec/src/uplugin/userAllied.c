#include <vitasdk.h>

#include <kEntente.h>
#include "userAllied.h"


int userAlliedDecryptSelf(struct ententeParams *u_param) {
	return kuEntenteDecryptSelf(u_param);
}

int	userAlliedStatus() {
	return kuEntenteStatus();
}

void userAlliedGetLogs(char *dest) {
	kuEntenteGetLogs(dest);
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  return SCE_KERNEL_STOP_SUCCESS;
}