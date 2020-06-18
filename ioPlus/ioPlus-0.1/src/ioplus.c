//
// ioPlus 0.1 -- by @dots-tb @BigBlackOniiSan
// Non-threaded, fast, and simplistic io elevation for user plugins and applications
// WARNING -- USE AT OWN RISK, it obviously elevates IO of all aplications
//

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dolcesdkkern.h>
#include <taihen.h>

#define printf ksceDebugPrintf
#define HOOKS_NUMBER 4
static int hook = -1;
static tai_hook_ref_t ref_hook;

typedef struct mount_point_overlay{
  uint8_t type;
  uint8_t order;
  uint16_t dst_len;
  uint16_t src_len;
  uint32_t PID;
  uint32_t mountId;
  char dst[292];
  char src[292];
} mount_point_overlay;

static int sceFiosKernelOverlayAddForProcessForDriver_patched(uint32_t pid, mount_point_overlay *overlay, uint32_t *outID) {
	int ret, state;
	ENTER_SYSCALL(state);
	if(memcmp("invalid:", overlay->src, sizeof("invalid:") - 1)==0)
		strncpy(overlay->src, overlay->dst, sizeof(overlay->src));
	ret = TAI_CONTINUE(int, ref_hook, pid, overlay, outID);
	EXIT_SYSCALL(state);
	return ret;
}	
void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	hook = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hook, "SceFios2Kernel", TAI_ANY_LIBRARY, 0x17E65A1C, sceFiosKernelOverlayAddForProcessForDriver_patched);
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	if (hook >= 0) taiHookReleaseForKernel(hook, ref_hook);   
	return SCE_KERNEL_STOP_SUCCESS;
}
