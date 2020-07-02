TARGET := NetDbgLogPc
SOURCES := source
BUILD	:= build

LIBS := -lstdc++ -lpsapi -lws2_32

CFLAGS := -fpermissive -DNO_STDIO_REDIRECT -Dmain=SDL_main
LFLAGS :=
LDARGS := -DNO_STDIO_REDIRECT -Dmain=SDL_main

C_FILES	:= $(shell find $(SOURCES) -type f -name '*.c')
CPP_FILES	:= $(shell find $(SOURCES) -type f -name '*.cpp')
OBJ_FILES := $(patsubst %.c,%.o,$(C_FILES)) $(patsubst %.cpp,%.o,$(CPP_FILES))
CC = $(GCC_PREFIX)gcc
CXX = $(GCC_PREFIX)g++
LD = $(GCC_PREFIX)gcc
WINDRES = $(GCC_PREFIX)windres

all: $(TARGET).exe

$(TARGET).exe: $(OBJ_FILES)
	@echo Creating icon resource file...
	@$(WINDRES) source/icon.rc -O coff -o icon.res
	@echo Linking object files...
	$(LD) -o $@ $(LDARGS) $^ icon.res $(LIBS)

%.o: %.c
	@echo Creating object file $@...
	$(CXX) $(CFLAGS) -c $< -o $@
	
clean:
	@echo Cleaning...
	@rm -f $(OBJ_FILES)
	@rm -f icon.res
	@rm -f *.exe