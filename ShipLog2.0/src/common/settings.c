#include <vitasdk.h>
#include "settings.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static char _ip[15];
static int _port;
static int _updated = 1;
static int _isNet = 0;
static int _isFile = 0;
void setPort(int port){
	_port = port;
}
int getPort(){
	return _port;
}
void setFile(int file){
	_isFile = file;
}
int getFile(){
	return _isFile;
}
void setNet(int net){
	_isNet = net;
}
int getNet(){
	return _isNet;
}
void setIP(const char *ip){
	strncpy(_ip,ip,15);
}
char *getIP(){
	char * ret = malloc(15);
	strcpy(ret,_ip);
	return ret;
}
int isUpdated(){
	return _updated;
}
int getLengthOfBuffer(){
	int ret =  shipLogBufLength();
	return ret;
}
void updated(int bol) {
	_updated = bol;
}