CC = gcc
CFLAGS = -g -s -static -Wall -Wextra
TARGET = PSVita-ELF-builder
OBJS = main.o file.o

ifeq ($(DEBUG), 1)
CFLAGS+=-g -O0
else
CFLAGS+=-O2
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lz

clean:
	$(RM) *.o $(TARGET) *.exe *.exe.stackdump
