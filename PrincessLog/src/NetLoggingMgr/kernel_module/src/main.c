/*
PSVita RE Tools: NetLoggingMgr aka PrincessLog
Copyright (C) 2020 Princess of Sleeping
Copyright (C) 2020 Asakura Reiko

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/utils.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/net/net.h>
#include <psp2kern/ctrl.h>

#include <psp2/kernel/error.h>

#include <taihen.h>

#include <stdarg.h>

#include "NetLoggingMgrInternal.h"
#include "ringbuf.h"

#define HookImport(module_name, library_nid, func_nid, func_name) taiHookFunctionImportForKernel(KERNEL_PID, &func_name ## _ref, module_name, library_nid, func_nid, func_name ## _patch)

#define HookExport(module_name, library_nid, func_nid, func_name) taiHookFunctionExportForKernel(KERNEL_PID, &func_name ## _ref, module_name, library_nid, func_nid, func_name ## _patch)

#define HookOffset(modid, seg_index, thumb, offset, func_name) taiHookFunctionOffsetForKernel(KERNEL_PID, &func_name ## _ref, modid, seg_index, offset, thumb, func_name ## _patch);

#define HookRelease(hook_uid, func_name) ({ \
	if(hook_uid > 0)taiHookReleaseForKernel(hook_uid, func_name ## _ref); \
})

#define NON_THUMB	0
#define IS_THUMB	1

#define GetExport(modname, lib_nid, func_nid, func) \
	module_get_export_func(KERNEL_PID, modname, lib_nid, func_nid, (uintptr_t *)func)

#define GetOffset(modid, segidx, offset, func_p) \
	module_get_offset(KERNEL_PID, modid, segidx, offset, (uintptr_t *)func_p)

#define NLM_BIT_INIT			(1 << 0)
#define NLM_BIT_DELAY_NET_THREAD	(1 << 1)
#define NLM_BIT_CONFIG_LOADED		(1 << 2)

#define RINGBUF_LEN 0x2000

static uint32_t NetLoggingMgrFlags = 0;

static int net_thread_run = 0;
static SceUID net_thread_uid = 0;

static SceNetSockaddrIn server;

int (* sceKernelGetModuleListForKernel)(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);
int (* sceKernelGetModuleInfoForKernel)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

int (* SceDebugForDriver_391B5B74)(const char *fmt, ...);
int (* sceDebugRegisterPutcharHandlerForKernel)(int (*func)(void *args, char c), void *args);
int (* sceDebugSetHandlersForKernel)(int (*func)(int unk, const char *format, const va_list args), void *args);
void *(* sceDebugGetPutcharHandlerForKernel)(void);
int (* sceDebugDisableInfoDumpForKernel)(int flags);

int (* sceKernelSysrootGetShellPidForDriver)(void);

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int ch, size_t n);
char *strncpy(char *s1, const char *s2, size_t n);
size_t strnlen(const char *s, size_t maxlen);
int vsnprintf(char *s, size_t n, const char *fmt, va_list arg_ptr);

int ksceSblAimgrGetConsoleId(char *cid);
int ksceSblACMgrIsDevelopmentMode(void);

int ksceSblQafMgrIsAllowControlIduAutoUpdate();
int ksceSblQafMgrIsAllowDtcpIpReset();
int ksceSblQafMgrIsAllowKeepCoreFile();
int ksceSblQafMgrIsAllowLoadMagicGate();
int ksceSblQafMgrIsAllowMarlinTest();
int ksceSblQafMgrIsAllowNearTest();
int ksceSblQafMgrIsAllowPSPEmuShowQAInfo();
int ksceSblQafMgrIsAllowRemotePlayDebug();
int ksceSblQafMgrIsAllowSystemAppDebug();

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

unsigned int My_ksceNetInetPton(unsigned char ip1, unsigned char ip2, unsigned char ip3, unsigned char ip4){

	unsigned int ip = 0;

	ip += (unsigned int)(ip1 << 0);
	ip += (unsigned int)(ip2 << 8);
	ip += (unsigned int)(ip3 << 16);
	ip += (unsigned int)(ip4 << 24);

	//*out = ip;

	return ip;
}

// userland printf
int UserDebugPrintfCallback(void *args, char c){
	ringbuf_put_clobber(&c, 1);
	return 0;
}

int KernelDebugPrintfCallback(int unk, const char *fmt, const va_list args){
	char buf[0x400];
	int buf_len = sizeof(buf);
	int len = vsnprintf(buf, buf_len, fmt, args);
	len = len < 0 ? 0 : len;
	len = len >= buf_len ? buf_len - 1 : len;
	ringbuf_put_clobber(buf, len);
	return 0;
}

int PrintModuleListForPid(SceUID pid){

	int ret;

	SceKernelModuleInfo info;
	info.size = sizeof(SceKernelModuleInfo);

	SceUID modlist[128];
	size_t count = 128;

	ret = sceKernelGetModuleListForKernel(pid, 0x7FFFFFFF, 1, modlist, &count);
	if(ret < 0){
		goto end;
	}

	for(int i=count;i>0;i--){

		sceKernelGetModuleInfoForKernel(pid, modlist[i-1], &info);

		ksceDebugPrintf(
			"[%-27s] text:0x%08X(0x%08X) data:0x%08X(0x%08X)\n",
			info.module_name,
			info.segments[0].vaddr, info.segments[0].memsz,
			info.segments[1].vaddr, info.segments[1].memsz
		);
	}
	ksceDebugPrintf("\n");

end:
	return ret;
}

/* flags for sceNetShutdown */
#define SCE_NET_SHUT_RD             0
#define SCE_NET_SHUT_WR             1
#define SCE_NET_SHUT_RDWR           2

int ksceNetShutdown(int s, int how);

static int net_connect(void) {
	int net_sock;

socket:
	net_sock = ksceNetSocket("NetLoggingTCP", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	if (net_sock < 0) {
		goto socket_error;
	}

	int timeout = 5 * 1000 * 1000;
	ksceNetSetsockopt(net_sock, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDTIMEO, &timeout, sizeof(timeout));

	if (ksceNetConnect(net_sock, (SceNetSockaddr*)&server, sizeof(server)) == 0) {
		return net_sock;
	}
	ksceNetShutdown(net_sock, SCE_NET_SHUT_RDWR);

socket_error:
	ksceNetClose(net_sock);
	ksceKernelDelayThread(1000 * 1000);
	goto socket;
}

static void net_close(int net_sock) {
	ksceNetShutdown(net_sock, SCE_NET_SHUT_RDWR);
	ksceNetClose(net_sock);
}

static int net_thread(SceSize args, void *argp){
	if(NetLoggingMgrFlags & NLM_BIT_DELAY_NET_THREAD){
		ksceKernelDelayThread(8 * 1000 * 1000);
	}

	ksceDebugPrintf("\n");
	ksceDebugPrintf("start net_thread\n");
	ksceDebugPrintf("\n");

	while(net_thread_run){
		char buf[0x400];
		int received_len = ringbuf_get_wait(buf, sizeof(buf), NULL);
		if (received_len == 0) {
			continue;
		}

		int net_sock;

	connect:
		net_sock = net_connect();

	send:
		if (ksceNetSend(net_sock, buf, received_len, 0) < 0) {
			net_close(net_sock);
			ksceKernelDelayThread(1000 * 1000);
			goto connect;
		}

		received_len = ringbuf_get_wait(buf, sizeof(buf), (SceUInt[]){1000 * 1000});
		if (received_len > 0) {
			goto send;
		}

		net_close(net_sock);
	}

	return 0;
}




NetLoggingMgrConfig_t NetLoggingMgrConfig;
SceUID hook_uid[0x20];

/*
 * Enable SceKernelModulemgr DebugPrintf ?
 */
static tai_hook_ref_t SceQafMgrForDriver_382C71E8_ref;
static int SceQafMgrForDriver_382C71E8_patch(void){
	return NetLoggingMgrConfig.flags & NLM_CONFIG_FLAGS_BIT_QAF_DEBUG_PRINTF;
	//return 1;
}

int NetLoggingMgrUpdateConfig(NetLoggingMgrConfig_t *new_config){

	int res;
	uint32_t state;

	const char magic[4] = {'N', 'L', 'M', '\0'};
	char chk_magic[4];

	ENTER_SYSCALL(state);

	res = ksceKernelMemcpyUserToKernel(&chk_magic, (uintptr_t)new_config, 4);
	if(res < 0){
		goto end;
	}

	if(memcmp(chk_magic, magic, 4) != 0){
		res = SCE_KERNEL_ERROR_ILLEGAL_TYPE;
		goto end;
	}

	res = ksceKernelMemcpyUserToKernel(&NetLoggingMgrConfig, (uintptr_t)new_config, sizeof(NetLoggingMgrConfig_t));
	if(res < 0){
		goto end;
	}

	server.sin_addr.s_addr = NetLoggingMgrConfig.IPv4;
	server.sin_port = ksceNetHtons(NetLoggingMgrConfig.port ? NetLoggingMgrConfig.port : DEFAULT_PORT);
	
	res = 0;

end:
	EXIT_SYSCALL(state);

	return res;
}


int NetLoggingMgrReadConfig(NetLoggingMgrConfig_t *new_config){

	int res;
	uint32_t state;

	ENTER_SYSCALL(state);

	res = ksceKernelMemcpyKernelToUser((uintptr_t)new_config, &NetLoggingMgrConfig, sizeof(NetLoggingMgrConfig_t));
	if(res < 0){
		goto end;
	}

end:
	EXIT_SYSCALL(state);

	return res;
}



int NetLoggingMgrLoadConfigForKernel(void){

	int res;
	SceUID fd;
	const char magic[4] = {'N', 'L', 'M', '\0'};



	server.sin_len = sizeof(server);
	server.sin_family = SCE_NET_AF_INET;



	memset(server.sin_zero, 0, sizeof(server.sin_zero));



	fd = ksceIoOpen("ur0:data/NetLoggingMgrConfig.bin", SCE_O_RDONLY, 0);
	if(fd < 0){
		res = fd;
		goto end;
	}
	res = ksceIoRead(fd, &NetLoggingMgrConfig, sizeof(NetLoggingMgrConfig_t));
	if(res != sizeof(NetLoggingMgrConfig_t)){
		res = -1;
		goto end;
	}

	if(memcmp(&NetLoggingMgrConfig, &magic, 4) != 0){
		res = SCE_KERNEL_ERROR_ILLEGAL_TYPE;
		res = 0;
		goto end;
	}

	server.sin_addr.s_addr = NetLoggingMgrConfig.IPv4;
	server.sin_port = ksceNetHtons(NetLoggingMgrConfig.port ? NetLoggingMgrConfig.port : DEFAULT_PORT);
	
	res = 0;

	NetLoggingMgrFlags |= NLM_BIT_CONFIG_LOADED;

end:
	if(fd > 0){
		ksceIoClose(fd);
	}

	return res;
}

int NetLoggingMgrFinish(void){

	int ret;

	if(!(NetLoggingMgrFlags & NLM_BIT_INIT)){
		ret = 1;
		goto end;
	}

	if(net_thread_run != 0){
		net_thread_run = 0;
		ksceKernelWaitThreadEnd(net_thread_uid, NULL, NULL);
	}

	if(net_thread_uid > 0){
		ksceKernelDeleteThread(net_thread_uid);
		net_thread_uid = 0;
	}

	HookRelease(hook_uid[0x00], SceQafMgrForDriver_382C71E8);

	sceDebugSetHandlersForKernel(0, 0);

	sceDebugRegisterPutcharHandlerForKernel(0, 0);

	ringbuf_term();

	NetLoggingMgrFlags &= ~NLM_BIT_INIT;

end:
	return ret;
}

int NetLoggingMgrInit(void){

	int ret;

	memset(hook_uid, 0, sizeof(hook_uid));

	if(NetLoggingMgrFlags & NLM_BIT_INIT){
		ret = 1;
		goto end;
	}

	ret = NetLoggingMgrLoadConfigForKernel();
	if(ret < 0){
		goto end;
	}

	ret = ringbuf_init(RINGBUF_LEN);
	if (ret < 0) {
		goto end;
	}

	if(GetExport("SceKernelModulemgr", 0xC445FA63, 0x97CF7B4E, &sceKernelGetModuleListForKernel) < 0)
	if(GetExport("SceKernelModulemgr", 0x92C9FFC2, 0xB72C75A4, &sceKernelGetModuleListForKernel) < 0){
		ret = -1;
		goto end;
	}

	if(GetExport("SceKernelModulemgr", 0xC445FA63, 0xD269F915, &sceKernelGetModuleInfoForKernel) < 0)
	if(GetExport("SceKernelModulemgr", 0x92C9FFC2, 0xDAA90093, &sceKernelGetModuleInfoForKernel) < 0){
		ret = -1;
		goto end;
	}

	if(GetExport("SceSysmem", 0x88C17370, 0xE6115A72, &sceDebugRegisterPutcharHandlerForKernel) < 0)
	if(GetExport("SceSysmem", 0x13D793B7, 0x22546577, &sceDebugRegisterPutcharHandlerForKernel) < 0){
		ret = -1;
		goto end;
	}

	if(GetExport("SceSysmem", 0x88C17370, 0x10067B7B, &sceDebugSetHandlersForKernel) < 0)
	if(GetExport("SceSysmem", 0x13D793B7, 0x88AD6D0C, &sceDebugSetHandlersForKernel) < 0){
		ret = -1;
		goto end;
	}

	if(GetExport("SceSysmem", 0x88C17370, 0xE783518C, &sceDebugGetPutcharHandlerForKernel) < 0)
	if(GetExport("SceSysmem", 0x13D793B7, 0x8D474850, &sceDebugGetPutcharHandlerForKernel) < 0){
		ret = -1;
		goto end;
	}

	if(GetExport("SceSysmem", 0x88C17370, 0xF857CDD6, &sceDebugDisableInfoDumpForKernel) < 0)
	if(GetExport("SceSysmem", 0x13D793B7, 0xA465A31A, &sceDebugDisableInfoDumpForKernel) < 0){
		ret = -1;
		goto end;
	}

	if(GetExport("SceSysmem", 0x2ED7F97A, 0x05093E7B, &sceKernelSysrootGetShellPidForDriver) < 0){
		ret = -1;
		goto end;
	}

	hook_uid[0x00] = HookExport("SceSysmem", 0xFFFFFFFF, 0x382C71E8, SceQafMgrForDriver_382C71E8);

	ret = sceDebugDisableInfoDumpForKernel(0);
	if(ret < 0){
		goto end;
	}

	sceDebugSetHandlersForKernel(KernelDebugPrintfCallback, 0);
	sceDebugRegisterPutcharHandlerForKernel(UserDebugPrintfCallback, 0);

	if(sceKernelSysrootGetShellPidForDriver() < 0){		// Enso user
		NetLoggingMgrFlags |= NLM_BIT_DELAY_NET_THREAD;
	}

	net_thread_uid = ksceKernelCreateThread("net_thread", net_thread, 0x40, 0x1000, 0, 0, 0);
	if(net_thread_uid < 0){
		ret = net_thread_uid;
		goto end;
	}

	net_thread_run = 1;

	ksceKernelStartThread(net_thread_uid, 0, NULL);

	ret = 0;

end:
	NetLoggingMgrFlags |= NLM_BIT_INIT;

	if(ret < 0){
		//NetLoggingMgrFinish();
	}

	return ret;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args){

	int ret = 0;

	SceCtrlData pad;

	ksceCtrlPeekBufferPositive(0, &pad, 1);
	if(pad.buttons & SCE_CTRL_LTRIGGER){
		goto end;
	}

	ret = NetLoggingMgrInit();
	if(ret < 0){
		//return SCE_KERNEL_START_NO_RESIDENT;
		//return SCE_KERNEL_START_FAILED;
	}

end:
	return SCE_KERNEL_START_SUCCESS;
}
/*
//    stop: module_stop
int module_stop(SceSize argc, const void *args){

	//NetLoggingMgrFinish();

	return SCE_KERNEL_STOP_SUCCESS;
}
*/