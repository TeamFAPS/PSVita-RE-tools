#include <string.h>
#include <vitasdk.h>
#include "settings.h"
#include "parser.h"
//from vitashell I think?
int ReadFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
	return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}

int WriteFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
	return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}
int checkFileExist(const char *file) {
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));

	return sceIoGetstat(file, &stat) >= 0;
}
int loadConfig(char *file) {
	config_file config;
	memset(&config, 0, sizeof(config_file));
	if(ReadFile(file, (void*)&config, sizeof(config_file))<0)
		return -1;
	char ip_text[15];
	sceNetInetNtop(SCE_NET_AF_INET,&config.ip,ip_text,sizeof(ip_text));
	setIP(ip_text);
	setFile(config.fileLog);
	setNet(config.netEnable);
	setPort(config.port);
	return 0;
}
int saveConfig(char *file) {
	config_file config;
	memset(&config, 0, sizeof(config_file));
	sceNetInetPton(SCE_NET_AF_INET, getIP(), &config.ip);
	config.port=getPort();
	config.netEnable = getNet();
	config.fileLog = getFile();
	WriteFile(file, (void*)&config, sizeof(config_file));
	return 0;
}
/*int checkInstall() {
	checkFileExist(N_PLUGIN);
}*/