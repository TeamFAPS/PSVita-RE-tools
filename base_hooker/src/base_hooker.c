#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <taihen.h>
#include <stdlib.h>

#include <vitasdk.h>

#define printf sceClibPrintf
#define HOOKS_NUM 1

tai_hook_ref_t hook_ref[HOOKS_NUM];
int hook_uid[HOOKS_NUM];

void hex_dump(const char *name, char *addr, int length) {
	if(addr < 0x81000000 || addr > 0x83FFFFFF)
		return;
	printf("%s:\n", name);
	for(int i = 0; i < length; i++ )
		printf("%02x ", addr[i]);
	printf("\n\n");
}

int hook_func1(int r1, int r2, int r3, int r4, int r5, int r6, int r7) {
	printf("hook_func1(0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", r1, r2, r3, r4); 
	
	hex_dump("hook_func1 r1", r1, 0x50);
	hex_dump("hook_func1 r2", r2, 0x50);
	hex_dump("hook_func1 r3", r3, 0x50);
	hex_dump("hook_func1 r4", r4, 0x50);
	
	int ret = TAI_CONTINUE(int, hook_ref[0], r1, r2, r3, r4, r5, r6, r7);
	
	hex_dump("hook_func1 r1", r1, 0x50);
	hex_dump("hook_func1 r2", r2, 0x50);
	hex_dump("hook_func1 r3", r3, 0x50);
	hex_dump("hook_func1 r4", r4, 0x50);
	
    printf("hook_func1(0x%08X, 0x%08X, 0x%08X, 0x%08X): 0x%08X\n", r1, r2, r3, r4, ret); 
	return ret;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {
	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_module_info_t);
	taiGetModuleInfo(TAI_MAIN_MODULE, &tai_info);
	
	
	hook_uid[0] = taiHookFunctionOffset(&hook_ref[0], tai_info.modid, 0, 0x0, 1, hook_func1);
	
	for(int i = 0; i < HOOKS_NUM; i++) 
		printf("hook_uid[%i]: %x\n", i, hook_uid[i]); 
 return SCE_KERNEL_START_SUCCESS;
}
int module_stop(SceSize argc, const void *args) {
	
	for(int i = 0; i < HOOKS_NUM; i++) 
		if (hook_uid[i] >= 0) taiHookRelease(hook_uid[i],hook_ref[i]);
	return SCE_KERNEL_STOP_SUCCESS;
}
