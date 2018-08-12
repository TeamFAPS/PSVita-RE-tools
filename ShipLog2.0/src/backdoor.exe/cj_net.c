//https://gist.github.com/xerpi/e426284df19c217a8128
//XERPRI THE HIDDEN GEM
//https://github.com/Cpasjuste/knettest/blob/master/main.c
//Cpasjuste for net idea: because I credit people
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <taihen.h>
#include <sys/time.h>

#include "utils.h"
#include "vita_netdbg.h"


static int _dbgsock = -1;
static int net_thid = -1;
static int is_connected = 0;
static int send_flag = -1;
static config_file config;
SceNetSockaddrIn server;

enum NetTransEvents {
    SEND_IN = 1,
    SEND_OUT = 2,
};

#define MAX_TRIES 1

int (*_ksceNetConnect)(int s, const SceNetSockaddr *name, unsigned int namelen);


int main_netdbg() {
	int tries = 0;
	struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    ksceNetSetsockopt(_dbgsock, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	while(1) {
		if(config.netEnable&&!is_connected) {
			if(_dbgsock>-1)
				ksceNetSocketClose(_dbgsock);
			_dbgsock = ksceNetSocket("vitanetdbg", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
			if(_ksceNetConnect(_dbgsock, (SceNetSockaddr *)&server, sizeof(server))>-1){
				is_connected = 1;
				ksceDebugPrintf("Connected!");
			}
			ksceKernelDelayThread(300000);
		}
		uint32_t result;
		if(bufLength() > 0) {
			int sent = 0, ret = 0;	
		
			if(is_connected&&config.netEnable) {
				ksceKernelWaitEventFlag(send_flag, SEND_OUT,SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR,
                                    &result, NULL);
				while(sent < bufLength()) {
					ret = ksceNetSend(_dbgsock, bufGet() + sent, bufLength(), 0);
					if(config.fileLog)
						log_write(bufGet() + sent, ret);
					 if(ret < 0) {
						is_connected = 0;
						break;
					 }
					 sent += ret;
				}
				if(is_connected)
					bufClear();
			} else if(config.fileLog&&ret<bufLength()) {
				ksceKernelWaitEventFlag(send_flag, SEND_OUT,SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR,
                                    &result, NULL);
				log_write(bufGet()+sent, bufLength()-sent);
				bufClear();
				ksceKernelDelayThread(400000);
			}
						
			ksceKernelDelayThread(100000);
		}
				
		
	}
}

int init_netdbg(config_file conf) {
	send_flag = ksceKernelCreateEventFlag("NetEvent", 0x1000, 0, NULL);
	memcpy(&config, &conf, sizeof(config));
	module_get_export_func(KERNEL_PID, "SceNetPs", 0xB2A5C920, 0x13491DA1, (uintptr_t*)&_ksceNetConnect);
	
	if(config.netEnable) {
		server.sin_len = sizeof(server);
		server.sin_family = SCE_NET_AF_INET;
		memcpy(&server.sin_addr , &config.ip, sizeof(server.sin_addr));
		server.sin_port = ksceNetHtons(config.port);
		memset(server.sin_zero, 0, sizeof(server.sin_zero));
	}
	
	net_thid = ksceKernelCreateThread("vitanetdbg_thd", main_netdbg, 0x10000100, 0x1000, 0, 0, NULL);
    if (net_thid >= 0)
        ksceKernelStartThread(net_thid, 0, NULL);
	else
		return -1;
	return 0;
}

int get_socket() {
	return _dbgsock;
}
void fini_netdbg()
{
	if (_dbgsock) {
		ksceNetSocketClose(_dbgsock);
		_dbgsock = 0;
	}
}
void stopSend() {
	ksceKernelClearEventFlag(send_flag, ~SEND_OUT);
}
void startSend() {
	ksceKernelSetEventFlag(send_flag, SEND_OUT);
}

