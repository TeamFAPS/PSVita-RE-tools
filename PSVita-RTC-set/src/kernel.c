#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include <taihen.h>


#define DUMP_PATH "ux0:data/"
#define LOG_FILE DUMP_PATH "rtc_log.txt"

static void log_write(const char *buffer, size_t length);

#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer)); \
	} while (0)



void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	uint32_t ret = -1;
	uintptr_t stub;

	// 0xDCB731F0F1C000 (1970)
	// + 9676800000000 (diff 112 days Idk why)
	// = 0xDCBFFEFF2BC000 = 62135596800000000
	uint32_t startDate1970 = 1527811200; // seconds since 1970 - CHANGE WITH YOUR OWN VALUE
	uint64_t startDate0 = (startDate1970 * 1000 * 1000) + 0xDCBFFEFF2BC000;

	// Setting CP RTC for DevKits
	ret = module_get_export_func(KERNEL_PID, "SceSblPostSsMgr", TAI_ANY_LIBRARY, 0x3F9BDEDF, &stub);
	LOG("ret = %08x\n", ret);
	if (ret == 0){
		void (*sceSblRtcMgrSetCpRtcForDriver) (int) = (void *) stub;
		sceSblRtcMgrSetCpRtcForDriver(startDate1970); // 01-01-2000: 0x386D4381; 
	}
	
	// Setting Network RTC for TestKits
	LOG("%llx\n", startDate0);
	LOG("time0: %8x\n", ((unsigned int *)&startDate0)[0]);
	LOG("time1: %8x\n", ((unsigned int *)&startDate0)[1]);

	unsigned int timestamp[2];
	timestamp[0] = ((unsigned int *)&startDate0)[0];
	timestamp[1] = ((unsigned int *)&startDate0)[1];

	LOG("Updating all Ticks.. ");
	ret = ksceRtcSetCurrentTick(timestamp);
	LOG("ret = %08x\n", ret);
	ret = ksceRtcSetCurrentNetworkTick(timestamp);
	LOG("ret = %08x\n", ret);
	ret = ksceRtcSetCurrentSecureTick(timestamp);
	LOG("ret = %08x\n", ret);
	ret = ksceRtcSetCurrentDebugNetworkTick(timestamp);
	LOG("ret = %08x\n", ret);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	return SCE_KERNEL_STOP_SUCCESS;
}

void log_write(const char *buffer, size_t length)
{
	extern int ksceIoMkdir(const char *, int);
	ksceIoMkdir(DUMP_PATH, 6);

	SceUID fd = ksceIoOpen(LOG_FILE,
		SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
	if (fd < 0)
		return;

	ksceIoWrite(fd, buffer, length);
	ksceIoClose(fd);
}
