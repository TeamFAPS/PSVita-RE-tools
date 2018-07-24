#include <vitasdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>

#include "utils.h"
const char *DEVICES[DEVICES_AMT]={ "ux0:", "gro0:", "grw0:"};
int getFileSize(const char *file) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoClose(fd);
	return fileSize;
}
int ReadFile(const char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
	return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}
int WriteFileSeek(const char *file, void *buf, size_t seek, size_t size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	if (fd < 0)
	return fd;
	sceIoLseek(fd, seek, SCE_SEEK_SET);
	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}
int WriteFile(const char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd < 0)
	return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}

void generateFolders(const char *path, const char *dest_path) {
	char temp_path[PATH_MAX];
	strcpy(temp_path, path);
	char *folder = (char *)&temp_path;
	folder = strchr(folder, ':') + 1;
	if(folder[0] == '/')
		folder++;
	char current_path[PATH_MAX];
	strcpy(current_path, dest_path);
	char *end_path;
	while((end_path = strchr(folder, '/'))!= NULL) {
		*end_path = 0;
		snprintf(current_path, PATH_MAX, "%s/%s", current_path, folder);
		sceIoMkdir(current_path, 6);
		folder = end_path + 1;
	}
}


int checkExists(const char *path) {
	SceIoStat stat;
	return sceIoGetstat(path, &stat);
}

