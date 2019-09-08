//unity_debug - debug logging tool for UNITY on the PS VITA
//Slaves: @dots_tb, @Nkekev, @coburn64, @nyaaasen
// Nkekev - sleep deprivation, token french for this project, testing, and further NID exploration
// Coburn - Unity advisor and C# programmer
// Sys (Yasen) - bringing the team together and """PR"""

//Special thanks to Team_molecule (esp davee for his valiant effort.)
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <taihen.h>
#include <stdlib.h>

#include <vitasdk.h>

int hook[3];
static tai_hook_ref_t start_mod_ref;
static tai_hook_ref_t int_call_ref;
static tai_hook_ref_t il_int_call_ref;

void utf16_to_utf8(uint16_t *src, uint8_t *dst) {
	int i;
	for (i = 0; src[i]; i++) {
		if ((src[i] & 0xFF80) == 0) {
			*(dst++) = src[i] & 0xFF;
		} else if((src[i] & 0xF800) == 0) {
			*(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		} else if((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
			*(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
			*(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
			*(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
			*(dst++) = (src[i + 1] & 0x3F) | 0x80;
			i += 1;
		} else {
			*(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
			*(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		}
	}
	*dst = '\0';
}

typedef struct MonoString {
	char shitwedontneed[0x6];
	int32_t length;
	uint16_t strings[256];
} MonoString;


void Internal_Log(int level, MonoString *msg, void *obj) {
	char utf8msg[512];
	utf16_to_utf8(msg->strings,utf8msg);
	printf("[Unity %d]: %s\n", level, utf8msg);	
}

void Internal_Log2(int level, MonoString *msg, void *obj) {
	char utf8msg[512];
	utf16_to_utf8(msg->strings,utf8msg);
	printf("[Unity %d]: %s\n", level, utf8msg);	
}

int isDebug() {
	printf("Set to debug mode\n\n");
	return 1;
}

int mono_add_internal_call_patched(const char *name, void *ptr) {
	int ret;
	void *ptr_new = ptr;
	//printf("%s\n\n", name);
	if(strcmp("UnityEngine.DebugLogHandler::Internal_Log",name)==0) 
		ptr_new = &Internal_Log;
	else if(strcmp("UnityEngine.Debug::Internal_Log",name)==0) 
		ptr_new = &Internal_Log2;
	else if(strcmp("UnityEngine.Debug::get_isDebugBuild",name)==0) 
		ptr_new = &isDebug;
	
	ret =  TAI_CONTINUE(SceUID, int_call_ref,name, ptr_new);
	
	return ret;

}

/*int func2_patched(int r1, int r2, int r3, int r4) {
	int ret;
	
	ret =  TAI_CONTINUE(SceUID, il_int_call_ref, r1, r2, r3, r3);
	printf("%s %x %x %x\n", r1, r2, r3, r3);
	return ret;

}*/

SceUID sceKernelLoadStartModule_patched(char *path, SceSize args, void *argp, int flags, SceKernelLMOption *option, int *status) {
	int ret = TAI_CONTINUE(int,start_mod_ref, path, args, argp, flags, option, status);
	if(strcmp(path,"app0:/Media/Modules/mono-vita.suprx")==0) {
		printf("MONO was executed\n");
		hook[1] = taiHookFunctionExport(
			&int_call_ref,
			"MONO",
			TAI_ANY_LIBRARY,
			0x231753c3,
			&mono_add_internal_call_patched);
		printf("hook[0]: %x\n",hook[0]);
	} if(strcmp(path,"app0:/Media/Modules/Il2CppAssemblies.suprx")==0) {
		//does not work atm
		
		printf("il2CppAssemblies was executed\n");

		/*hook[2] = taiHookFunctionExport(
			&il_int_call_ref,
			"LIBIL2CPP_PRX",
			TAI_ANY_LIBRARY,
			0xF129763F,
			&func2_patched);
			printf("hook[2]: %x\n",hook[2]);*/
	}
	
	return ret;
}
void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {
	printf("unity_debug by @dots_tb, @Nkekev, @coburn64, @nyaaasen\n");
	
	hook[0] = taiHookFunctionImport(
		&start_mod_ref,
		TAI_MAIN_MODULE,
		TAI_ANY_LIBRARY,
		0x2DCC4AFA,
		sceKernelLoadStartModule_patched);
	return SCE_KERNEL_START_SUCCESS;
}
int module_stop(SceSize argc, const void *args) {
	if (hook[0] >= 0) taiHookRelease(hook[0], start_mod_ref);
	if (hook[1] >= 0) taiHookRelease(hook[1], int_call_ref);
	if (hook[2] >= 0) taiHookRelease(hook[2], il_int_call_ref);

	return SCE_KERNEL_STOP_SUCCESS;
}
