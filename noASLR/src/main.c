/*
 * PSVita ASLR disabler
 * Copyright (C) 2020, Princess of Sleeping
 */

#include <string.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>


#define HookImport(module_name, library_nid, func_nid, func_name) taiHookFunctionImportForKernel(0x10005, &func_name ## _ref, module_name, library_nid, func_nid, func_name ## _patched)
#define HookRelease(hook_uid, func_name) ({ (hook_uid > 0) ? taiHookReleaseForKernel(hook_uid, func_name ## _ref) : -1; })

#define DEBUG		0
#define USE_INJECT	1

#if DEBUG == 1
#define printf ksceDebugPrintf
#else
#define printf(...)
#endif

SceUID hook_uid[4];

const char movs_r0_0[] = {0x00, 0x20};
const char movs_r3_0[] = {0x00, 0x23};
const char nop_x4[]    = {0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF};

#if !USE_INJECT
tai_hook_ref_t SceProcessmgrForDriver_2CEB1C7A_ref;
int SceProcessmgrForDriver_2CEB1C7A_patched(SceUID pid, void *dst, SceSize maxlen){
	int res;

	res = TAI_CONTINUE(int, SceProcessmgrForDriver_2CEB1C7A_ref, pid, dst, maxlen);
	if (res == 0 && maxlen == 4) {
		printf("Disable user low level ASLR : 0x%03X -> 0\n", (*(int *)dst) & 0xFF0);
		*(int *)dst = 0;
	}

	return res;
}
#endif


void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	SceUID SceKernelModulemgr_modid = ksceKernelSearchModuleByName("SceKernelModulemgr");

	/*
	 * Always set extra memory size reserved for ASLR to zero
	 * this is target a module text for all process
	 */
	hook_uid[0] = taiInjectDataForKernel(0x10005, SceKernelModulemgr_modid, 0, 0xCC, movs_r0_0, 2);

	/*
	 * Patch to disable SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_EXTRA_LOW
	 */
	hook_uid[1] = taiInjectDataForKernel(0x10005, SceKernelModulemgr_modid, 0, 0xC2, nop_x4, sizeof(nop_x4));

	/*
	 * Patch to disable SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_EXTRA_HIGH
	 */
	hook_uid[2] = taiInjectDataForKernel(0x10005, SceKernelModulemgr_modid, 0, 0xB8, nop_x4, sizeof(nop_x4));

	#if USE_INJECT
		hook_uid[3] = taiInjectDataForKernel(0x10005, SceKernelModulemgr_modid, 0, 0xA54, movs_r3_0, 2);
	#else
		hook_uid[3] = HookImport("SceKernelModulemgr", 0x746EC971, 0x2CEB1C7A, SceProcessmgrForDriver_2CEB1C7A);
	#endif

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	#if USE_INJECT
		taiInjectReleaseForKernel(hook_uid[3]);
	#else
		HookRelease(hook_uid[3], SceProcessmgrForDriver_2CEB1C7A);
	#endif
	taiInjectReleaseForKernel(hook_uid[2]);
	taiInjectReleaseForKernel(hook_uid[1]);
	taiInjectReleaseForKernel(hook_uid[0]);

	return SCE_KERNEL_STOP_SUCCESS;
}