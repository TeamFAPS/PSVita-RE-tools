CC = gcc
CFLAGS = -Wall -g
TARGET = psp2-emmc-extract
OBJS = main.o

ifeq ($(DEBUG), 1)
CFLAGS+=-g -O0
else
CFLAGS+=-O2
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -L -lz

clean:
	$(RM) *.o $(TARGET) *.exe *.exe.stackdump
