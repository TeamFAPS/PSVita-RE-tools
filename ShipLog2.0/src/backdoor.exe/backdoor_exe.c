//ShipLog - Backdoor.exe

//Put together by dots_tb
//Created for Kancolle Kai Vita translation and dev team (expecially you senpai ~<3)
//Special thanks to Team_Molecule for Taihen (special thanks to xyz)
//thanks to xerpi for being underrated (and logging functions/netdebug), frangarcj for oclock
//Freakler for common dialog, TheFlow for VitaShell

//Dialog functions: 
// https://github.com/Freakler/vita-uriCaller

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <taihen.h>

#include "utils.h"
#include "vita_netdbg.h"

#define BUF_SIZE 1024*32

static int __stdout_fd = -1;
static int __stdout_ufd = -1;
static int __stdout_upid = -1;

static char buffer[BUF_SIZE];
static int buf_pos = 0;
static SceUID g_hooks[3];

int (*_ksceDebugSetHandlers)(int (*func)(void *args, char c), void *args);
int (*_ksceDebugRegisterPutcharHandler)(int (*func)(void *args, char c), void *args);

#define ksceDebugRegisterPutcharHandler _ksceDebugRegisterPutcharHandler
#define ksceDebugSetHandlers _ksceDebugSetHandlers

int bufLength() {
	return buf_pos;
}

void bufClear() {	
	memset(buffer,0,BUF_SIZE);
	buf_pos = 0;
}

char *bufGet() {
	return &buffer;
}

static tai_hook_ref_t ref_hook1;

int sceKernelGetStdout_patched() {
	int state, fd;
	ENTER_SYSCALL(state);
	fd = TAI_CONTINUE(int, ref_hook1);
	if(__stdout_upid == ksceKernelGetProcessId()) {
		fd = __stdout_ufd;
	}
	EXIT_SYSCALL(state);
	return fd;
}


void debugHandler(int unk0, const char *format, const uint32_t *args){
	char buf[512];
	
	snprintf(buf, sizeof(buf), format, args[0], args[1], args[2], args[3],args[4],args[5],args[6],args[7],args[8]); 
	stopSend();
	int length = strnlen(buf, sizeof(buf));
	if(buf_pos+length > BUF_SIZE) {
		log_write(buffer, BUF_SIZE);
		bufClear();	
	}
	memcpy(buffer + buf_pos, buf, length);
	buf_pos+=length;
	startSend();
}
int putchar_handler(void *args, char c){
	stopSend();
	if(buf_pos+1 > BUF_SIZE) {
		log_write(buffer, BUF_SIZE);
		bufClear();	
	}
	buffer[buf_pos++] = c;
	startSend();
	return c;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {
	config_file config;
	config.netEnable = 0;
	config.fileLog = 0;
	LOG("ShipLog\n");
	ReadFile(CONFIG_FILE, &config, sizeof(config_file));
	g_hooks[1] = taiHookFunctionExportForKernel(KERNEL_PID,
		&ref_hook1, "SceProcessmgr", 0x2DD91812,
		0xE5AA625C,sceKernelGetStdout_patched);
	LOG("Net logging: %x\n", config.netEnable);
	LOG("File logging: %x\n", config.fileLog);
	init_netdbg(config);
	LOG("hooks:\n");
	LOG("hooks[SceNetPsForDriver_1ABF937D]: %x\n", g_hooks[0]);
	LOG("hooks[sceKernelGetStdout]: %x\n", g_hooks[1]);
	if(module_get_export_func(KERNEL_PID, "SceSysmem", 0x88C17370, 0x10067B7B, (uintptr_t*)&_ksceDebugSetHandlers) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", 0x13D793B7, 0x88AD6D0C, (uintptr_t*)&_ksceDebugSetHandlers);
	if(module_get_export_func(KERNEL_PID, "SceSysmem", 0x88C17370, 0xE6115A72, (uintptr_t*)&_ksceDebugRegisterPutcharHandler) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", 0x13D793B7, 0x22546577, (uintptr_t*)&_ksceDebugRegisterPutcharHandler);	
	LOG("Setting debug handler: %x\n", ksceDebugSetHandlers(&debugHandler, NULL));
	LOG("Setting putchar handler: %x\n",ksceDebugRegisterPutcharHandler(&putchar_handler, NULL));
	
	return SCE_KERNEL_START_SUCCESS;

}

int module_stop(SceSize argc, const void *args)
{
 if (g_hooks[1] >= 0) taiHookReleaseForKernel(g_hooks[1], ref_hook1);
	return SCE_KERNEL_STOP_SUCCESS;
}
int shipLogBufLength() {
	int state;
	int ret;
	ENTER_SYSCALL(state);
	ret = buf_pos;
	EXIT_SYSCALL(state);
	return ret;
}

void shipLogBufClear() {
	int state;
	ENTER_SYSCALL(state);
	bufClear();
	EXIT_SYSCALL(state);
}
void shipLogDumpToDisk() {
	int state;
	ENTER_SYSCALL(state);
	log_write(buffer, buf_pos);
	bufClear();
	EXIT_SYSCALL(state);
}

int shipLogToFile(const char *out) {
	int state, res;
	char fn[256];
	ENTER_SYSCALL(state);
	ksceKernelStrncpyUserToKernel((uintptr_t)&fn, out, sizeof(fn));
	if((__stdout_fd = ksceIoOpen(fn, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6))>-1) {
		 __stdout_ufd = res = ksceKernelCreateUserUid(__stdout_upid = ksceKernelGetProcessId(), __stdout_fd);
	}
	EXIT_SYSCALL(state);
	return res;
}

