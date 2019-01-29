CC=gcc
CFLAGS=-Wall
LDFLAGS=-lz
SOURCES= mingw_mmap.c aes.c tools.c sha1.c sha2.c main.c
EXECUTABLE=index-dat-tool
all:
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(EXECUTABLE)
clean:
	rm -rf $(EXECUTABLE)