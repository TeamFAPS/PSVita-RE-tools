//
// ioPlus 0.1 -- by @dots-tb
// Non-threaded, fast, and simplistic io elevation for user plugins and applications
// WARNING -- USE AT OWN RISK, it obviously elevates IO of all aplications
//

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <vitasdkkern.h>
#include <taihen.h>

#define printf ksceDebugPrintf
#define HOOKS_NUMBER 4
static int hook = -1;
static tai_hook_ref_t ref_hook;

static int sceFiosKernelOverlayResolveSyncForDriver_patched(SceUID pid, int resolveFlag, const char *pInPath, char *pOutPath, size_t maxPath) {
	int ret, state;
	ENTER_SYSCALL(state);
	ret = TAI_CONTINUE(int, ref_hook, pid, resolveFlag, pInPath, pOutPath, maxPath);
	if(memcmp("invalid", pOutPath, sizeof("invalid") - 1) == 0)
		strncpy(pOutPath, pInPath, maxPath);
	EXIT_SYSCALL(state);
	return ret;
}	
void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	hook = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hook, "SceFios2Kernel", TAI_ANY_LIBRARY, 0x0F456345, sceFiosKernelOverlayResolveSyncForDriver_patched);
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	if (hook >= 0) taiHookReleaseForKernel(hook, ref_hook);   
	return SCE_KERNEL_STOP_SUCCESS;
}