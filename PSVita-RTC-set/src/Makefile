TITLE_ID = RTC00001
TARGET = PSVita-RTC-set
PSVITAIP = 192.168.137.84

PLUGIN_OBJS = kernel.o
HEADERS = $(wildcard *.h)

PLUGIN_LIBS = -ltaihenModuleUtils_stub -ltaihen_stub -lSceSysclibForDriver_stub -lSceIofilemgrForDriver_stub -lSceSysmemForDriver_stub -lSceRtcForDriver_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: kplugin.skprx

kplugin.skprx: kplugin.velf
	vita-make-fself -c $< $@

kplugin.velf: kplugin.elf
	vita-elf-create -e exports.yml $< $@

kplugin.elf: $(PLUGIN_OBJS)
	$(CC) $(CFLAGS) $^ $(PLUGIN_LIBS) -o $@ -nostdlib

clean:
	@rm -rf *.velf *.elf *.vpk *.skprx $(MAIN_OBJS) $(PLUGIN_OBJS) param.sfo eboot.bin