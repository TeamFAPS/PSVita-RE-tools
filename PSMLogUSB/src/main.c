/* 
 *
 * This File is part Of:
 *      ___         ___           ___                         ___           ___     
 *     /  /\       /  /\         /__/\                       /  /\         /  /\    
 *    /  /::\     /  /:/_       |  |::\                     /  /::\       /  /:/_   
 *   /  /:/\:\   /  /:/ /\      |  |:|:\    ___     ___    /  /:/\:\     /  /:/ /\  
 *  /  /:/~/:/  /  /:/ /::\   __|__|:|\:\  /__/\   /  /\  /  /:/  \:\   /  /:/_/::\ 
 * /__/:/ /:/  /__/:/ /:/\:\ /__/::::| \:\ \  \:\ /  /:/ /__/:/ \__\:\ /__/:/__\/\:\
 * \  \:\/:/   \  \:\/:/~/:/ \  \:\~~\__\/  \  \:\  /:/  \  \:\ /  /:/ \  \:\ /~~/:/
 *  \  \::/     \  \::/ /:/   \  \:\         \  \:\/:/    \  \:\  /:/   \  \:\  /:/ 
 *   \  \:\      \__\/ /:/     \  \:\         \  \::/      \  \:\/:/     \  \:\/:/  
 *    \  \:\       /__/:/       \  \:\         \__\/        \  \::/       \  \::/   
 *     \__\/       \__\/         \__\/                       \__\/         \__\/    
 *
 * Copyright (c)bps <http://cbps.xyz/> under GPLv3
 *
 */

//Princess Silly Mini Log USB - A logger using the USB serial driver that came with PSM.
// dots_tb and SilicaAndPina (Project Manager)
// Sysie

//Original PrincessLog
// Princess of Sleeping (@PoSsvkey)
// cuevavirus

//Tested and requested by:
// SonicMastr
// teakhanirons

//Requested by
// Yifan Lu

//Brought to you buy Chairman Blue People's Server:
// https://forum.devchroma.nl/
// http://discord.cbps.xyz/
// https://twitter.com/CBPS9


#include <vitasdkkern.h>
#include <taihen.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ringbuf.h"

#define printf ksceDebugPrintf

#define RINGBUF_LEN 0x2000


int (* sceDebugRegisterPutcharHandlerForKernel)(int (*func)(void *args, char c), void *args);
int (* sceDebugSetHandlersForKernel)(int (*func)(int unk, const char *format, const va_list args), void *args);
void *(* sceDebugGetPutcharHandlerForKernel)(void);
int (* sceDebugDisableInfoDumpForKernel)(int flags);

int (* sceUsbSerialStatusForDriver)();
int (* sceUsbSerialStartForDriver)();
int (* sceUsbSerialCloseForDriver)();
int (* sceUsbSerialSetupForDriver)(int);
int (* SceUsbSerialForDriver_6AA4EFA4)();

//int (* sceUsbSerialGetRecvBufferSizeForDriver)();

//int (* sceUsbSerialRecvForDriver)(const void *buffer, unsigned int len, int unk1, int unk2);
int (* sceUsbSerialSendForDriver)(const void *buffer, unsigned int len, int unk1, int unk2);

int g_usb_running = 1;
int g_is_shell = 0;
SceUID g_usb_thread_uid = -1;
SceUID g_usb_flag_uid = -1;

#define SERIAL_BUSY 0x0
#define SERIAL_READY 0x1
#define SERIAL_STOP 0x2

#define HOOKS_NUM 3

SceUID hook[HOOKS_NUM];
tai_hook_ref_t hook_ref[HOOKS_NUM];

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


static int usb_thread(SceSize args, void *argp){
	printf("\n");
	printf("start usb_thread\n");
	printf("\n");
	
	while(g_usb_running){
		char buf[0x400];
		int received_len = ringbuf_get_wait(buf, sizeof(buf), NULL);

		int ret = 0;
	connect:
		ret = sceUsbSerialStatusForDriver();
		if(ret != 0x80244401) {
			if(ret == 1) 
				goto send;
			ksceKernelDelayThread(1000 * 1000);
			if(ret == 0)
				goto connect;
		}
		
		ksceKernelWaitEventFlag(g_usb_flag_uid, SERIAL_STOP, SCE_EVENT_WAITCLEAR, NULL, 0);

		sceUsbSerialStartForDriver();
		sceUsbSerialSetupForDriver(1);
		if (ret == 0x80244403) {
			SceUsbSerialForDriver_6AA4EFA4();
			sceUsbSerialSetupForDriver(1);
		}
		ksceKernelSetEventFlag(g_usb_flag_uid, SERIAL_READY);
		
	send:
		if (received_len == 0) 
			continue;
		
		ksceKernelClearEventFlag(g_usb_flag_uid, SERIAL_READY);
		
		if (sceUsbSerialSendForDriver(buf, received_len, 0, -1) < 0) {
			ksceKernelSetEventFlag(g_usb_flag_uid, SERIAL_STOP);
			ksceKernelDelayThread(1000 * 1000);
			goto connect;
		}

		received_len = ringbuf_get_wait(buf, sizeof(buf), (SceUInt[]){1000 * 1000});
		ksceKernelSetEventFlag(g_usb_flag_uid, SERIAL_READY);
		if (received_len > 0) 
			goto send;
		
	}

	return 0;
}

static void cute_girls_dying() {
	SceUsbSerialForDriver_6AA4EFA4();
	sceUsbSerialCloseForDriver();
}

static int SceUsbMtpForDriver_6FC97594_patched() {
	return 0x0;
}

static int sceUsbstorVStorStart_patched(int r0) {
	ksceKernelWaitEventFlag(g_usb_flag_uid, SERIAL_READY, SCE_EVENT_WAITCLEAR, NULL, 0);
	cute_girls_dying();
	ksceUdcdStopCurrentInternal(2);
	return TAI_CONTINUE(int, hook_ref[1], r0);
}

static int sceUsbstorVStorStop_patched() {
	ksceUdcdStopCurrentInternal(2);
	ksceKernelSetEventFlag(g_usb_flag_uid, SERIAL_STOP);
	return TAI_CONTINUE(int, hook_ref[2]);
}


void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args){
	
	g_usb_flag_uid = ksceKernelCreateEventFlag("Silly USB Flag", 0, SERIAL_STOP, NULL);
	
	hook[0] = taiHookFunctionExportForKernel(KERNEL_PID, &hook_ref[0], "SceUsbMtp", TAI_ANY_LIBRARY, 0x6FC97594, SceUsbMtpForDriver_6FC97594_patched);
	hook[1] = taiHookFunctionExportForKernel(KERNEL_PID, &hook_ref[1], "SceUsbstorVStorDriver", TAI_ANY_LIBRARY, 0xB606F1AF, sceUsbstorVStorStart_patched);
	hook[2] = taiHookFunctionExportForKernel(KERNEL_PID, &hook_ref[2], "SceUsbstorVStorDriver", TAI_ANY_LIBRARY, 0x0FD67059 , sceUsbstorVStorStop_patched);
	
	if(module_get_export_func(KERNEL_PID, "SceSysmem", 0x88C17370, 0xE6115A72, &sceDebugRegisterPutcharHandlerForKernel) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", 0x13D793B7, 0x22546577, &sceDebugRegisterPutcharHandlerForKernel);

	if(module_get_export_func(KERNEL_PID, "SceSysmem", 0x88C17370, 0x10067B7B, &sceDebugSetHandlersForKernel) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", 0x13D793B7, 0x88AD6D0C, &sceDebugSetHandlersForKernel);

	if(module_get_export_func(KERNEL_PID, "SceSysmem", 0x88C17370, 0xE783518C, &sceDebugGetPutcharHandlerForKernel) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", 0x13D793B7, 0x8D474850, &sceDebugGetPutcharHandlerForKernel);

	if(module_get_export_func(KERNEL_PID, "SceSysmem", 0x88C17370, 0xF857CDD6, &sceDebugDisableInfoDumpForKernel) < 0)
		module_get_export_func(KERNEL_PID, "SceSysmem", 0x13D793B7, 0xA465A31A, &sceDebugDisableInfoDumpForKernel);
	
	ringbuf_init(RINGBUF_LEN);
	
	sceDebugDisableInfoDumpForKernel(0);

	sceDebugSetHandlersForKernel(KernelDebugPrintfCallback, 0);
	sceDebugRegisterPutcharHandlerForKernel(UserDebugPrintfCallback, 0);
	printf(
			" These logs are part of CBPS's\n"
			"      ___         ___           ___                         ___           ___     \n"
			"     /  /\\       /  /\\         /__/\\                       /  /\\         /  /\\    \n"
			"    /  /::\\     /  /:/_       |  |::\\                     /  /::\\       /  /:/_   \n"
			"   /  /:/\\:\\   /  /:/ /\\      |  |:|:\\    ___     ___    /  /:/\\:\\     /  /:/ /\\  \n"
			"  /  /:/~/:/  /  /:/ /::\\   __|__|:|\\:\\  /__/\\   /  /\\  /  /:/  \\:\\   /  /:/_/::\\ \n"
			" /__/:/ /:/  /__/:/ /:/\\:\\ /__/::::| \\:\\ \\  \\:\\ /  /:/ /__/:/ \\__\\:\\ /__/:/__\\/\\:\\\n"
			" \\  \\:\\/:/   \\  \\:\\/:/~/:/ \\  \\:\\~~\\__\\/  \\  \\:\\  /:/  \\  \\:\\ /  /:/ \\  \\:\\ /~~/:/\n"
			"  \\  \\::/     \\  \\::/ /:/   \\  \\:\\         \\  \\:\\/:/    \\  \\:\\  /:/   \\  \\:\\  /:/ \n"
			"   \\  \\:\\      \\__\\/ /:/     \\  \\:\\         \\  \\::/      \\  \\:\\/:/     \\  \\:\\/:/  \n"
			"    \\  \\:\\       /__/:/       \\  \\:\\         \\__\\/        \\  \\::/       \\  \\::/   \n"
			"     \\__\\/       \\__\\/         \\__\\/                       \\__\\/         \\__\\/    \n" 
																												);
	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_module_info_t);
	printf("taiGetModuleInfoForKernel %x\n", taiGetModuleInfoForKernel(KERNEL_PID, "SceUsbSerial", &tai_info));
	printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1E78 | 1, (uintptr_t *)&sceUsbSerialStatusForDriver));
	printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x168C | 1, (uintptr_t *)&sceUsbSerialStartForDriver));
	printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x17A0 | 1, (uintptr_t *)&sceUsbSerialSetupForDriver));
	printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x18F4 | 1, (uintptr_t *)&SceUsbSerialForDriver_6AA4EFA4));
	printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1A68 | 1, (uintptr_t *)&sceUsbSerialSendForDriver));
	printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1738 | 1, (uintptr_t *)&sceUsbSerialCloseForDriver));
	//printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1A28 | 1, (uintptr_t *)&sceUsbSerialRecvForDriver));
	//printf("module_get_offset %x\n", module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1D30 | 1, (uintptr_t *)&sceUsbSerialGetRecvBufferSizeForDriver));
	
	g_usb_thread_uid = ksceKernelCreateThread("usb_thread", usb_thread, 0x40, 0x1000, 0, 0, 0);
	ksceKernelStartThread(g_usb_thread_uid, 0, NULL);
	
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args){
	sceDebugSetHandlersForKernel(0, 0);
	sceDebugRegisterPutcharHandlerForKernel(0, 0);
	
	if(g_usb_running != 0){
		g_usb_running = 0;
		ksceKernelWaitThreadEnd(g_usb_thread_uid, NULL, NULL);
	}

	if(g_usb_thread_uid > 0){
		ksceKernelDeleteThread(g_usb_thread_uid);
		g_usb_thread_uid = 0;
	}
	
	ringbuf_term();
	cute_girls_dying();
	
	for(int i = 0; i < HOOKS_NUM; i++)
		taiHookReleaseForKernel(hook[i], hook_ref[i]);	
	return SCE_KERNEL_STOP_SUCCESS;
}
