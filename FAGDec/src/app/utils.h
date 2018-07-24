#define DEVICES_AMT 3

const char *DEVICES[DEVICES_AMT];

int getFileSize(const char *file);
int ReadFile(const char *file, void *buf, int size) ;
int WriteFileSeek(const char *file, void *buf, size_t seek, size_t size);
int WriteFile(const char *file, void *buf, int size);
int checkExists(const char *path);
void generateFolders(const char *path, const char *dest_path);