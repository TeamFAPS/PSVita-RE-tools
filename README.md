# PSVita Reverse Engineering Tools
by @dots-tb and @CelesteBlue-dev (and other devs if they add their tools there)

Every tool provided here will be licensed under GPLv3.0 unless stated otherwise.

What is that ?

This toolkit provides tools that aid in the development of vita homebrews and plugins. By providing tools that speed up development through automation of processes and gives a more friendly view into complex things of PSVita OS.

These tools aided in the development of plugins such as: NoAVLS, ReStore and ReNpDrm and many RE things that helped in Wiki and vitasdk improvements.

What it won’t do:

These tools are solely used to aid in homebrew / plugins / hacks development. It probably will not produce a major hack. It will not magically make you a vita god. It will not automatically hack the vita. These tools are purely tools, most of them already existed in some form or another.

THIS TOOLSET PURELY SPEEDS UP THE PROCESS OF DEVELOPMENT THROUGH EASE OF ACCESS AND DOES NOT GIVE YOU ANYTHING YOU ALL DID NOT HAVE.

*** Remember: IF YOU DO NOT UNDERSTAND WHAT THESE TOOLS DO, IT MAY NOT BE FOR YOU! ***

## Description of the tools

ShipLog v2.0 - by @dots-tb
---
A complete logging solution for any homebrew, user plugin, kernel plugin. It can use network or file logging.

	+ Added kernel network, removed user plugin dependency
	+ Added ability to select which logging methods.
	- Removed all user plugin dependency
	- USB removed because it causes problem with Shell and CMA (Content Manager Assistant), it is fast but not stable

* Credits: xerpi, psxdev for their works on logging solutions.  Cpasjuste for net.

VitaDecompilerMod – by @dots-tb
---
A FREE alternative to IDA. It has a great pseudo-C decompilation that offers a quick view thanks to text file exporting.

Compared to original @TheFlow's version we added a few features such as:

	+ More strings (EVEN MORE), including data section. (homebrew strings now work)
	+ Generates .c, .h, .txt (NIDs), and db_lookup (<module_name>.yml)
	+ ELF and fSELF support
	+ Compressed fSELF support
	+ Fixed issues with NIDS being improperly found
	+ Includes offsets and vaddr
	+ Automatic entry point (the entry point is retrieved properly from ELF header)
	+ Automatic entry point location (for badly generated ELFs)
	+ Relocation support
	+ There might be more ?

* Credits: TheFloW for original VitaDecompiler and prxtool's original devs

vita-unmake-fself – @by dots-tb
---
A PC tool that decompresses an unencrypted SELF file (skprx, suprx, self, eboot.bin) into an ELF file.

This tool can't decompress NPDRM encrypted SELF nor System encrypted SELF. That means that you will have to use vDump or any dumping tool to first get a unencrypted SELF. Read SELFtoELF documentation for more informations.

That Hooker Got NIDS – by @dots-tb
---
A PC tool that hooks specified NIDS automatically.

* Credits: xerpi for base code used, TheFlow for db.yml parsing

PSVita-ELF-builder – by @CelesteBlue-dev
---
A PC tool that rebuilds ELF from decrypted modules' segments. To be used after using vitaDecrypt or vDump.

* Credits: zecoxao for the tutorial (how to rebuild ELF from decrypted segments and original SELF)

vDump
---
A PSVita homebrew that dumps easily user/kernel modules in a variety of ways (NOTE: NOT GAMES, THIS DOES NOT ENABLE PIRACY).

* Credits: zecoxao for vitadump (new), st4rk for vitadump (old), xerpi for both.

nids.txt / db.yml
---
Some lists of functions names / NIDs / libraries / modules to be used with IDA, VitaDecompiler or other tools that will come later.

ioPlus 0.1, 0.2 – by @dots-tb
---
A PSVita kernel plugin that allows more IO operations in userland. Fast, simpler, and efficient alternative to kuio (by @Rinnegatamante) (3x smaller). It allows elevated IO permissions of user applications and plugins using the original sceIo functions. This includes reading, writing, opening, and folder management within applications such as official games. It may also include getting stats, not sure.

Version 0.2 is much more inefficient, but supports decryption of files within devices that may open such as PFS devices. (WARNING THIS PLUGIN MAY BYPASS SAFE-MODE)

--------------------------------------------------------------------------------

## Using the Tools

ShipLog v2.0 usage
---

### Usage:

Install ShipLog.skprx in taiHEN config.txt under *KERNEL and install ShipLog.vpk. You must reboot the Vita with the kernel plugin installed to open the configuration app. Logs are stored in a buffer and must be obtained using one of the following methods:

- Network:

	1. Open the ShipLog application and configure the network configuration. Make sure you enable net logging. Be sure to save your configuration.
	2. Type the command mentioned on the network configuration page.
	3. Reboot the vita.
	4. The netcat instance should show logs when the vita initializes shell/connects to network.

- File:
	
	1. Open the ShipLog application and enable file logging. Make sure you save the configuration.
	2. Reboot the vita.
	3. Check ux0:data/logs.txt for new logs.
	
	NOTE: This method may ware down your memory card and is slow. This method is only recommended when tested material may crash the system, preventing the dumping of logs.
	
- Dumping the buffer

	1. Open the ShipLog application and select dump logs to disk.
	2. Check ux0:data/logs.txt for new logs.
	
In the application you wish to log use:

	ksceDebugPrintf, printf (when SceLibc is included such as in games), or sceClibPrintf

### Building:

	./build.sh

It builds ShipLog.skprx and ShipLog.vpk.

--------------------------------------------------------------------------------

VitaDecompilerMod usage
---
Dependencies:

VitaDecompiler requires capstone (a disassembler) libraries and libyaml. (On Windows, capstone must be compiled from sources). Make sure you install the *.a and header files to the right directories.

	Run: ./vitadecompiler binary db.yml
	
It will create 4 files:
	
	<binary>.c (The decompiled code)
	<binary>.nids.txt (A detailed list of imports and exports)
	<binary>.yml (a db_lookup or exports in yml format)
	<binary>.h (Prototypes/list of all functions in the source code.
	
Interpreting the output:

The top section is a printed NIDS table which gives the exports and imports of a module. This information will provide the offsets, virtual address, NID, library name, library NID, and the NID name (or generated name).

Each function has a virtual address and offset displayed next to it. This offset given (if not for a function that has a NID) maybe hooked with Taihen.

Most strings or values are accompanied by a s_text/s_data which gives you the original address. The address is then checked repeatedly until a non-address is found. 

--------------------------------------------------------------------------------

vita-unmake-fself usage
---

### Usage:

	Run: ./vita-unmake-fself.exe input_fself

	(you most likely will be able to drag-and-drop also)

The output will be produced in the same folder with .elf appended on to the end of the original file name.

### Building:

Dependencies: zlib

	make install

--------------------------------------------------------------------------------

That Hooker Got NIDs usage
---

### Usage:

Dependencies: zlib, libyaml

	Run: ./THGN binary <all/library_name/exports/imports> <kernel/user> db.yml <sys:1/0>

Options:

	All: Every NID will be hooked. This will try to hook as an export at first, then attempt hook it as an import.

	Library_name: Every NID of a library (such as “SceCtrl”) within the module specified will be hooked. This will try to hook as an export at first, then attempt hook it as an import.

	Exports: Every export NID will be hooked.

	Imports: Every import NID will be hooked.

	Kernel: The generated code will work in kernel space.

	User: The generated code will work in user space.

	Sys: You may choose 1 or 0 to enable or disable syscall mode. You may omit this argument. Some functions will not log unless it enters syscall mode. If you do not see anything within your logs, you may try this option. Try not to use it.

--------------------------------------------------------------------------------

vDump usage
---
Everything is contained within the vpk. Follow the onscreen instructions provided in the application.

Methods:

	VitaDump (old) – Only works with user mode modules, this method dumps the sections out of memory.

	VitaDump (new) – MAKE SURE YOU RUN THE KPLUGIN (scroll down to kplugin and hit X to enable it). This method uses system functions to decrypt segments of SYSTEM applications.

--------------------------------------------------------------------------------

PSVita-ELF-builder usage
---
To reverse PSVita, you need some dumps of the PSVita modules. These dumps are either memory dumps (St4rk's vitadump) or decrypted SELF (vitaDecrypt).

Now you also have a ALL IN ONE solution: vDump. But in case you want to decrypt quickly a lot of SELFs, you will keep using vitaDecrypt.

vitaDecrypt outputs only compressed decrypted segments. But you have to decompress these segments, or better, convert to ELF file format. This is the aim of this tool.

The output .elf are valid for RE and they can also be rebuilded into SELF using vita-make-fself.

Please read the provided README.md for Usage informations.


ioPlus 0.1/0.2 usage
---
### Installation:

This is a kernel plugin and so it must be added to the taihen config.txt under the *KERNEL section. Once installed, you may use the standard sceIo functions such as sceIoOpen in user plugins and applications as normal.

#### ioPlus 0.2 only:

Using PFS decryption on ioPlus 0.2: to use decryption, use the “iop-decrypt:” device.

	Ex: to open app0:/Media/level0 ----> iop-decrypt:/Media/level0

NOTE: an opened device with the file decrypted must be currently opened in order for this to work.

### Building:

	mkdir build
	cd build
	cmake ..
	make all

--------------------------------------------------------------------------------

## Further thanks
zecoxao, xerpi, Team_molecule, mr.gas, MajorTom, TheFlow, Rinnegatamante, cpasjuste, Freakler, sys(yasen), Nkekev, SilicaAndPina, motoharu, mathieulh
