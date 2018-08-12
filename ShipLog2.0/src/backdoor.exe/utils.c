#include <vitasdkkern.h>
#include "utils.h"


void log_reset()
{
	SceUID fd = ksceIoOpen(LOG_FILE,
		SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
	if (fd < 0)
		return;

	ksceIoClose(fd);
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
int ReadFile(char *file, void *buf, int size) {
	SceUID fd = ksceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
	return fd;

	int read = ksceIoRead(fd, buf, size);

	ksceIoClose(fd);
	return read;
}