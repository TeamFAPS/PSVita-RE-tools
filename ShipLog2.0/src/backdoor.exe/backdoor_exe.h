//shipLogBufLength()
//Returns current position or length of the contents in buffer
int shipLogBufLength();

//shipLogBufClear()
//Clears the buffer and resets position to 0
void shipLogBufClear();

//shipLogToFile(const char *out);
//Sends log output of user process directly to file. ONLY WORKS FROM PROCESS IT WAS LAUNCHD FROM.
// out : location of file you would like to log to.
// ret : returns FD of file on success
int shipLogToFile(const char *out);

//shipLogDumpToDisk();
//Dumps the log buffer to default location
void shipLogDumpToDisk();

typedef struct config_file{
	int netEnable;
	int fileLog;
	SceNetInAddr ip;
	int port;
} config_file;

#define APP_DATA "ux0:data/shipLog/"
#define CONFIG_FILE APP_DATA "config"