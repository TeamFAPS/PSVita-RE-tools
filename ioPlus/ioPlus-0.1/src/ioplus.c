//
// ioPlus 0.1 -- by @dots-tb
// Non-threaded, fast, and simplistic io elevation for user plugins and applications
// WARNING -- USE AT OWN RISK, it obviously elevates IO of all aplications
//

#include <stdio.h>
#include <stdarg.h>
#include <vitasdkkern.h>
#include <taihen.h>


#define printf ksceDebugPrintf
#define HOOKS_NUMBER 4
static int hooks_uid[HOOKS_NUMBER];
static tai_hook_ref_t ref_hooks[HOOKS_NUMBER];

typedef struct sceIoOpenOpt
{
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoOpenOpt;

static int _sceIoOpen_patched(const char *filename, int flag, SceIoMode mode, sceIoOpenOpt *opt) {
	int ret, state;
	ENTER_SYSCALL(state);
	if ((ret = TAI_CONTINUE(int, ref_hooks[0], filename, flag, mode, opt)) < 0) { // try original function
		char k_path[128];
		ksceKernelStrncpyUserToKernel(&k_path, (uintptr_t)filename, sizeof(k_path));
		printf("opened thanks to ioPlus: %s\n", k_path);
		if ((ret = ksceIoOpen(k_path, flag, mode)) >= 0)
			ret = ksceKernelCreateUserUid(ksceKernelGetProcessId(), ret);
	}
	EXIT_SYSCALL(state);
	return ret;
}

typedef struct sceIoMkdirOpt
{
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoMkdirOpt;

static int _sceIoMkdir_patched(const char *dirname, SceIoMode mode, sceIoMkdirOpt opt) {
	int ret, state;
	ENTER_SYSCALL(state);
	if ((ret = TAI_CONTINUE(int, ref_hooks[1], dirname, mode, opt)) < 0) { // try original function
		char k_path[128];
		ksceKernelStrncpyUserToKernel(&k_path, (uintptr_t)dirname, sizeof(k_path));
		ret = ksceIoMkdir(k_path, mode);
	}
	EXIT_SYSCALL(state);
	return ret;
}

typedef struct sceIoRmdirOpt
{
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoRmdirOpt;

static int _sceIoRmdir_patched(const char *dirname, sceIoRmdirOpt* opt) {
	int ret, state;
	ENTER_SYSCALL(state);
	if ((ret = TAI_CONTINUE(int, ref_hooks[2], dirname, opt)) < 0) { // try original function
		char k_path[128];
		ksceKernelStrncpyUserToKernel(&k_path, (uintptr_t)dirname, sizeof(k_path));
		ret = ksceIoRmdir(k_path);
	}
	EXIT_SYSCALL(state);
	return ret;
}

typedef struct sceIoRemoveOpt
{
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoRemoveOpt;

static int _sceIoRemove_patched(const char *filename, sceIoRemoveOpt* opt) {
	int ret, state;
	ENTER_SYSCALL(state);
	if ((ret = TAI_CONTINUE(int, ref_hooks[3], filename, opt)) < 0) { // try original function
		char k_path[128];
		ksceKernelStrncpyUserToKernel(&k_path, (uintptr_t)filename, sizeof(k_path));
		ret = ksceIoRemove(k_path);
	}
	EXIT_SYSCALL(state);
	return ret;
}


void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	hooks_uid[0] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[0], "SceIofilemgr", TAI_ANY_LIBRARY, 0xCC67B6FD, _sceIoOpen_patched);
	hooks_uid[1] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[1], "SceIofilemgr", TAI_ANY_LIBRARY, 0x8F1ACC32, _sceIoMkdir_patched);
	hooks_uid[2] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[2], "SceIofilemgr", TAI_ANY_LIBRARY, 0xFFFB4D76, _sceIoRmdir_patched);
	hooks_uid[3] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[3], "SceIofilemgr", TAI_ANY_LIBRARY, 0x78955C65, _sceIoRemove_patched);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	for (int i=0; i < HOOKS_NUMBER; i++)
		if (hooks_uid[i] >= 0) taiHookReleaseForKernel(hooks_uid[i], ref_hooks[i]);   
	return SCE_KERNEL_STOP_SUCCESS;
}