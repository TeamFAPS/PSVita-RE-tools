#define DUMP_PATH "ux0:dump/"
#define LOG_FILE DUMP_PATH "logger.txt"
void log_write(const char *buffer, size_t length);
int ReadFile(char *file, void *buf, int size);
int bufLength();
void bufClear();
char *bufGet();
#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer)); \
	} while (0)
