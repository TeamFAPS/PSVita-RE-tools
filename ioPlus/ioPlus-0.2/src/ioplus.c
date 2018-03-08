//
// ioPlus 0.2 -- by @dots-tb
// Non-threaded, fast, and simplistic io elevation (with PFS decryption) for user plugins and applications
// WARNING -- USE AT OWN RISK, it obviously elevates IO of all aplications
//
// Issues:
// Potential race condition when adding virtual devices
// File opens are slowed down due GC

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <vitasdkkern.h>
#include <taihen.h>


#define printf ksceDebugPrintf
#define HOOKS_NUMBER 6

#define MAX_DEVICES 10
#define DECRYPT_KEYWORD "iop-decrypt:"

typedef struct PdDevice{
	char pd_name[18];
	char v_name[18];
	int name_len;
	uint32_t pid;
} PdDevice;

static PdDevice devices[MAX_DEVICES] = {[0 ... MAX_DEVICES - 1] = {{'\0', '\0', 0, 0}}};
static int hooks_uid[HOOKS_NUMBER];
static tai_hook_ref_t ref_hooks[HOOKS_NUMBER];


static int ksceIoOpenForPid_patched(SceUID pid, const char *filename, int flag, SceIoMode mode) {
	int ret, state, fd = 0;
	ENTER_SYSCALL(state);
	//garbage collector
	SceIoStat stat;
	for (int i=0; i < MAX_DEVICES; i++) {
		if ((fd = ksceIoGetstat(devices[i].pd_name, &stat)) < 0) {
			devices[i].name_len = 0;
			continue;
		}
	}
	if ((ret = TAI_CONTINUE(int, ref_hooks[0], pid, filename, flag, mode)) < 0
	&& (ret = ksceIoOpen(filename, flag, mode)) < 0
	&& !memcmp(DECRYPT_KEYWORD, filename, sizeof(DECRYPT_KEYWORD) - 1)) { // try original function
		char new_path[128];
		for (int i=0; i < MAX_DEVICES; i++) {
			if (devices[i].name_len == 0)
				continue;
			memset(new_path, 0, sizeof(new_path));
			memcpy(new_path, devices[i].v_name, devices[i].name_len);
			strncat(new_path, strchr(filename, ':') + 1, sizeof(new_path) - 1);
			if ((ret = TAI_CONTINUE(int, ref_hooks[0], devices[i].pid, new_path, flag, mode)) >= 0)
				break;
		}
	}
	EXIT_SYSCALL(state);
	return ret;
}

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

static int SceFios2KernelForDriver_0F456345(SceUID pid, int r2, char *old_path, char *new_path, int r5) {
	int ret, state, currentSlot = -1;
	ENTER_SYSCALL(state);
	ret = TAI_CONTINUE(int, ref_hooks[4], pid, r2, old_path, new_path, r5);
	if (!memcmp("PD", new_path, 2)) {
		uint32_t device_len = strchr(old_path, ':') - old_path + 1, pd_len = strchr(new_path, ':') - new_path + 1;
		for (int i=0; i < MAX_DEVICES; i++) {
			if (devices[i].name_len != 0) {
				if (!memcmp(devices[i].pd_name, new_path, pd_len - 1))
					currentSlot = -2;
			} if (currentSlot == -1)
				currentSlot = i;
		}
		if (currentSlot >= 0) {
			memcpy(devices[currentSlot].v_name, old_path, device_len);
			memcpy(devices[currentSlot].pd_name, new_path, pd_len);
			devices[currentSlot].v_name[device_len] = devices[currentSlot].pd_name[pd_len] = '\0';
			devices[currentSlot].name_len = device_len;
			devices[currentSlot].pid = pid;				
		}
	}
	EXIT_SYSCALL(state);
	return ret;	
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	//hooks_uid[0] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[0], "SceIofilemgr", TAI_ANY_LIBRARY, 0xCC67B6FD, _sceIoOpen_patched);
	hooks_uid[1] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[1], "SceIofilemgr", TAI_ANY_LIBRARY, 0x8F1ACC32, _sceIoMkdir_patched);
	hooks_uid[2] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[2], "SceIofilemgr", TAI_ANY_LIBRARY, 0xFFFB4D76, _sceIoRmdir_patched);
	hooks_uid[3] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[3], "SceIofilemgr", TAI_ANY_LIBRARY, 0x78955C65, _sceIoRemove_patched);
	
	hooks_uid[4] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[0], "SceIofilemgr", TAI_ANY_LIBRARY, 0xC3D34965, ksceIoOpenForPid_patched);
	
	hooks_uid[5] = taiHookFunctionExportForKernel(KERNEL_PID, &ref_hooks[4], "SceFios2Kernel", TAI_ANY_LIBRARY, 0x0F456345,  SceFios2KernelForDriver_0F456345);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	for (int i=0; i < HOOKS_NUMBER; i++)
		if (hooks_uid[i] >= 0) taiHookReleaseForKernel(hooks_uid[i], ref_hooks[i]);   
	return SCE_KERNEL_STOP_SUCCESS;
}