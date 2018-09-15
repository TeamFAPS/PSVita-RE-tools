#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/io/fcntl.h>


#define PHYSDUMP_START			0x402
#define PHYSDUMP_END			0x5FD
#define PHYSDUMP_OUTPUT			"ur0:dump/physmem-dump.bin"
#define DUMP_PATH "ur0:dump/"
#define LOG_FILE DUMP_PATH "physmem-dumper_log.txt"

static void log_write(const char *buffer, size_t length);

#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer)); \
	} while (0)


static void *pa2va(unsigned int pa) {
	unsigned int va = 0;
	unsigned int vaddr;
	unsigned int paddr;
	unsigned int i;

	for (i = 0; i < 0x100000; i++) {
		vaddr = i << 12;
		__asm__("mcr p15,0,%1,c7,c8,0\n\t"
				"mrc p15,0,%0,c7,c4,0\n\t" : "=r" (paddr) : "r" (vaddr));
		if ((pa & 0xFFFFF000) == (paddr & 0xFFFFF000)) {
			va = vaddr + (pa & 0xFFF);
			break;
		}
	}
	return (void *)va;
}

int phy_mem_dump_safe(const char *path, unsigned int start, unsigned int end, unsigned int off, unsigned int len) {
	int fd;
	int ret;
	unsigned int *tbl = NULL;
	void *vaddr;
	unsigned int paddr;
	unsigned int i;
	int write;
	unsigned int ttbr0;

	LOG("dest=%s start=0x%x end=0x%x\n", path, start, end);

	// tbl = (unsigned int *)0x00008000;
	
	__asm__ volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbr0));
	LOG("ttbr0 = 0x%x\n", ttbr0);

	tbl = (unsigned int *) pa2va(ttbr0 & ~0xFF);
	LOG("ttbr0 VA = 0x%x\n", (unsigned int)tbl);

	fd = ksceIoOpen(path, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	LOG("sceIoOpen: 0x%08X\n", fd);
	if (fd < 0)
		goto error;

	for (i = start; i < end; i++) {
		tbl[0x3E0] = (i << 20) | 0x10592;
		vaddr = &tbl[0x3E0];
		__asm__ volatile ("dmb sy");
		//LOG("entry: 0x%08X\n", tbl[0x3E0]);
		__asm__ volatile ("mcr p15,0,%0,c7,c14,1" :: "r" (vaddr) : "memory");
		__asm__ volatile ("dsb sy\n\t"
				"mcr p15,0,r0,c8,c7,0\n\t"
				"dsb sy\n\t"
				"isb sy" ::: "memory");
		LOG("Dumping 0x%08X...\n", i << 20);
		vaddr = (void *) 0x3E000000;
		__asm__ volatile ("mcr p15,0,%1,c7,c8,0\n\t"
				"mrc p15,0,%0,c7,c4,0\n\t" : "=r" (paddr) : "r" (vaddr));

		write = 0;
		while ((write += ksceIoWrite(fd, (char*)vaddr+off+write, len-write)) < len) {
			LOG("partial written: 0x%08X\n", write);
			if (write < 0) {
				LOG("Write error\n");
				goto error;
			}
		}
	}

error:
	ksceIoClose(fd);
	return 0;
}


void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	ksceIoMkdir(DUMP_PATH, 6);
	ksceIoRemove(LOG_FILE); // optional: comment this line if you want to keep logs after reboot
	LOG("phys-dumper started.\n");
	
	char *dest = PHYSDUMP_OUTPUT;
	LOG("Starting physical dump.\n");
	phy_mem_dump_safe(dest, PHYSDUMP_START, PHYSDUMP_END, 0, 0x100000);
	LOG("Finished physical dump.\n");
	
	LOG("phys-dumper module_start sucessfully ended.\n");
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	LOG("Stopping phys-dumper...\n");
	
	LOG("phys-dumper module_stop sucessfully ended.\n");
	return SCE_KERNEL_STOP_SUCCESS;
}

SceUID fd;
void log_write(const char *buffer, size_t length) {
	fd = ksceIoOpen(LOG_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
	if (fd < 0)
		return;
	ksceIoWrite(fd, buffer, length);
	ksceIoClose(fd);
}