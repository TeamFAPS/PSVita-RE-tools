CC=gcc
CFLAGS=-static -Os -s
LDFLAGS=
SOURCES=main.c module_info_parser.c unarzl.c
EXECUTABLE=psp2-kbl-elf-extract

all:
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(EXECUTABLE)
	
clean:
	rm -rf $(EXECUTABLE)