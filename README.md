# PSVita Reverse Engineering Tools
by @dots-tb and @CelesteBlue-dev (and other devs if they add their tools there)

Every tool provided here will be licensed under GPLv3.0 unless stated otherwise.

What is that ?

This toolkit provides tools that aid in the development of PSVita homebrews and plugins, as well as PC tools and even emulators, by providing tools that speed up development through automation of processes and gives a more friendly view into complex things of PSVita OS.

These tools aided in the development of plugins such as: ReStore and ReNpDrm, repatch, reF00D, NoAVLS, rebgdl.

These tools also heavily contribute to Wiki and vitasdk improvements.

What it won’t do:

These tools are solely used to aid in homebrew / plugins / hacks development. It probably will not produce a major hack. It will not magically make you a vita god. It will not automatically hack the vita.

*** Remember: IF YOU DO NOT UNDERSTAND WHAT THESE TOOLS DO, IT MAY NOT BE FOR YOU! ***

## Description of the tools

Codename PrincessLog - by @Princess-of-Sleeping 
---
A complete logging solution for any homebrew, user plugin, kernel plugin. It is more efficient and overall nicer than ShipLog. It is Windows only atm.
	
* Credits: Princess-of-Sleeping 

ShipLog v2.0 - by @dots-tb 
---
A complete logging solution for any homebrew, user plugin, kernel plugin. It can use network or file logging.

	+ Added kernel network, removed user plugin dependency
	+ Added ability to select which logging methods.
	- Removed all user plugin dependency
	- USB removed because it causes problem with Shell and CMA (Content Manager Assistant), it is fast but not stable

* Credits: xerpi, psxdev for their works on logging solutions.  Cpasjuste for net.

That Hooker Got NIDS – by @dots-tb
---
A PC tool that hooks specified NIDS automatically.

* Credits: xerpi for base code used, TheFlow for db.yml parsing, yasen for the name ideas

VitaDecompilerMod – by @dots-tb - based on vitadecompiler by TheFloW, itself based on prxtool by TyRaNiD
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

* Credits: TheFloW for original VitaDecompiler and PSP prxtool's contributors

prxtool for PSVita - by TheFloW
---
An alternative to VitaDecompilerMod: prxtool cannot decompile to pseudo-C but it can decompiles ASM very well.

* Credits: TheFloW and xerpi for PSVita port and PSP prxtool's contributors

vita-unmake-fself – by @dots-tb
---
A PC tool that decompresses an unencrypted SELF file (.skprx, .suprx, .self, eboot.bin) into an ELF file (.elf, .velf).

This tool can't decompress NPDRM encrypted SELF nor System encrypted SELF. That means that you will have to use FAGDec or sceutils to first get a unencrypted SELF. Read SELFtoELF documentation for more informations.

vita-elf-inject – by @dots-tb
---
A PC tool that injects an ELF (made by FAGDec or vita-unmake-fself) into a decrypted eboot.bin.

* Credits: Motoharu and CelesteBlue for make-fself rev ur engs.

PSVita-ELF-builder – by @CelesteBlue-dev
---
DEPRECATED - A PC tool that rebuilds ELF from decrypted modules' segments. To be used after using vitaDecrypt (never released).

* Credits: zecoxao for the tutorial (how to rebuild ELF from decrypted segments and original SELF), vitasdk for vita-make-fself

French-American Games Decrypter (FAGDec) - by @CelesteBlue-dev and @dots-tb
---
A PSVita homebrew that decrypts easily PSVita user/kernel and games modules and can generate .ppk (compatibility pack for low FWs).

* Credits: Motoharu, Team Molecule, zecoxao for vitadump(new), xerpi, NPS team esp. @juliosueiras. Check app for further credits.

ioPlus 0.1, 0.2 – by @dots-tb
---
A PSVita kernel plugin that allows more IO operations in userland. Fast, simpler, and efficient alternative to kuio (by @Rinnegatamante) (3x smaller). It allows elevated IO permissions of user applications and plugins using the original sceIo functions. This includes reading, writing, opening, and folder management within applications such as official games. It may also include getting stats, not sure.

Version 0.2 is much more inefficient, but supports decryption of files within devices that may open such as PFS devices. (WARNING THIS PLUGIN MAY BYPASS SAFE-MODE)

physmem_dumper - by @xyzz
---
A PSVita kernel plugin that dumps Non-Secure World (NS kernel + userland) memory using RAM physical range: from 0x40200000 to 0x5FD00000.

See wiki for more information on PSVita's physical memory: https://wiki.henkaku.xyz/vita/Physical_Memory.

The output dump stored in ur0:dump/physmem-dump.bin is aimed to be loaded in IDA PRO using https://github.com/xyzz/vita-ida-physdump.

ErrorResolver - by @SilicaAndPina and @Princess-of-Sleeping
---
A PC Program that can read the PSVita os0:/kd/error_table.bin

kernel_bootimage_extract - by @CelesteBlue-dev and @zecoxao
---
A PC program that extracts embedded kernel modules ELFs from bootimage.elf or embedded ePSP flash files from pcff.elf.

kbl_elf_extract - by @CelesteBlue-dev and @dots-tb
---
A PC program that extracts embedded secure kernel modules ELFs from kernel_boot_loader.elf.seg1.

unarzl - by @TeamMolecule
---
A PC program that extracts ARZL compressed file.

Kdumper - by @TheFloW and @CelesteBlue-dev
---
A PSVita fSELF to run on an activated TestKit/DevKit on FW <=3.67 in order to dump its kernel !

Confirmed working between 3.50 and 3.67. Will need some changes for lower FWs (sceMotionDevGetEvaInfo is only on FW >= 3.50).

Credits: TheFloW for the kernel exploits, CelesteBlue for the many improvements, Mathieulh and LemonHaze for SceNgsUser code.

kdump-extract - by @dots-tb
---
A PC program that finds and extracts segment 0 of a kernel module from a continous kernel memory dump. It ourputs a .elf that can be used for RE (see vitadecompiler), for extracting NIDs (see nids-extract). It is to be used in conjunction with Kdumper on PSVita side.

nids-extract - by @dots-tb
---
A PC program that extracts a list in .yml format of exported NIDs from a PSVita ELF.

psvitalibdoc
---
Some lists of functions names / NIDs / libraries / modules to be used with vitadump IDA plugin, vitaldr IDA plugin, VitaDecompilerMod or prxtool for PSVita.

--------------------------------------------------------------------------------

## Using the Tools

PrincessLog Usage
---

### Usage:

	1. Install NetLoggingMgrSettings.vpk.
	2. Launch the application and configure your settings. Be sure to save.
	3. Add net_logging_mgr.skprx to your config.txt
	4. Run NetDbgLogPc.exe
	5. Reboot.
	Note: If the plugin is already installed and you wish to update the configuration, you may use Update Configuration (along with saving it) without rebooting your system.
	
In the application you wish to log use:

	ksceDebugPrintf, printf (when SceLibc is included such as in games), or sceClibPrintf

QAF Settings:

	There is options to make more verbose logs used in QA. You can enable these in the manager app.
	
Note:
	While being much faster than ShipLog, if there is massive amounts of logs the logger may not be able to process them completely and will freeze (ex: taiHEN hexdump). This is unlikely in normal usage.

	
### Building:

	Each application must built individually with cmake.
	When building the kernel plugin, use "make install" to automatically install the stubs. This must be done before building the manager app.
	The PC app does not have a dependency on order.
	

ShipLog v2.0 usage (Please use PrincessLog unless you are sadly on Linux)
---

### Usage:

Install backdoor_exe.skprx in taiHEN config.txt under *KERNEL and install ShipLog.vpk. You must reboot the Vita with the kernel plugin installed to open the configuration app. Logs are stored in a buffer and must be obtained using one of the following methods:

- Network:

	1. Open the ShipLog application and configure the network configuration. Make sure you enable net logging. Be sure to save your configuration.
	2. Type the command mentioned on the network configuration page.
	3. Reboot the vita.
	4. The netcat instance should show logs when the vita initializes shell/connects to network.

- File:
	
	1. Open the ShipLog application and enable file logging. Make sure you save the configuration.
	2. Reboot the vita.
	3. Check ux0:data/logger.txt for new logs.
	
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

prxtool usage
---

### Usage:

Put the module ELF and db.yml in the same folder as prxtool.exe.

The provided build is compatible with Windows. Simply open command line and:

	Run: prxtool -n db.yml -w <module_name>.elf > <module_name>.S

### Building:

The source code and building instructions are available on TheFloW's github repository:

https://github.com/TheOfficialFloW/prxtool

--------------------------------------------------------------------------------

vita-unmake-fself usage
---

### Usage:

	Run: ./vita-unmake-fself.exe input_fself

	(you most likely will be able to drag-and-drop also)

The output will be produced in the same folder with .elf appended on to the end of the original file name.

### Building:

Dependencies: zlib

	make

--------------------------------------------------------------------------------

vita-elf-inject usage
---

### Usage:

	Run: ./vita-elf-inject.exe input_fself input_elf

	(you most likely will be able to drag-and-drop also)
	
### Recommended usage for modding:	
	
	1. Run FAGDec to obtain a SELF.
	2. Use vita-unmake-fself to extract the elf from the self.
	3. Make required modifications to the elf.
	4. Use vita-elf-inject to inject this modified elf back into the self.
	
The original eboot will be replaced with the product of injection. Please make a backup to plan accordingly.

NOTE: The product will run, however it will have slight changes that differentiates it from a eboot made with the official make-fself.

### Building:

Dependencies: zlib

	make

--------------------------------------------------------------------------------
ErrorResolver usage
---
### Usage: 
* Requires os0:/kd/error_table.bin  to be present in the Working Directory.

```
Arguments: <mode> <input>
        Modes:
        -d Decode hex to shortcode
        -b Bruteforce hex from shortcode
No arguments = interactive
```
In decode it simply looks up the shortcode for the provided hex string    
However, in bruteforce mode, it will try every hex code in the table until it finds a match which takes about 2 seconds.      

Example: ErrorResolver -b C2-9779-2
would return 80102601 as this is the Hex code for C2-9779-2

### Building

Dependencies: none

	make
--------------------------------------------------------------------------------

FAGDec usage
---
Install the .vpk on a PSVita.

### Controls:
	Left/Right switches panels. Holding down each key will expand that pane to fullscreen.
	On the Modules list panel, you may hit "Circle" twice to delete a module. "Cross" cancels this action. You can hold down circle.
	On a menu with special option that can be jumped to with a button, the button that executes this option is indicated to the right of that specifc option.
		
### Menus:
	Decrypt to...:
		SELF - These are verified against the original ELF and can be ran directly on the PSVita. Big ELFs (50MB+) may have trouble verifying.
		ELF - These are not verified to increase speed. The product must be make_fself'd on the computer or by another method. However, the sha256 is saved to be verified later if you wish.

#### NOTE: Modders need a ELF to modify, then they have to make a SELF from the modded ELF. See just below:

### To manually convert ELF to SELF (on PC using make_fself):
		1) Obtain the leaked SDK make_fself.exe (YOU CANNOT USE THE VITASDK VERSION).
		2) Run: make_fself.exe -c -e <modulename>.elf <modulename>
		3) Open self_auth.bin/<modulename>.auth and copy the first 8 bytes to offset 0x80 of the output SELF of make_fself.exe. These 8 bytes data is the program-authority-id.
		
		NOTE: If you do not wish to use illegally obtained material or want a cross-platform solution, use vita-elf-inject

### Decrypting games (when installed on PSVita):
	Just select the title from the screen, and select the modules you wish to be decrypted.

### Decrypting games (when NOT installed on PSVita) -> Using the PATH_ID spoofing system:
	vs0/os0 - Drop the module into the vs0:/vs0_em or os0:/os0_em and it will decrypt as if it was the respective device.
	app/patch (YOU DO NOT NEED ASSETS TO USE THIS MODE) - Drop the game module into ux0:/app_em/<titleid> or ux0:/patch_em/<TITLEID>. They must be in their respective folder. They also must be PFS decrypted.
	NOTE: IN ORDER FOR A NPDRM GAME TO BE DECRYPTED, appropriate work.bin must be located at ux0:/app_em/<titleid>/sce_sys/package/work.bin. This applies to both patches and base games.

### WHEN MODDING YOU NEED THE self_auth.bin FROM THE BASE GAME! PLEASE DO NOT FORGET!

--------------------------------------------------------------------------------

PSVita-ELF-builder usage
---
To reverse PSVita, you need some dumps of the PSVita modules. These dumps are either memory dumps (St4rk's vitadump) or decrypted SELF (vitaDecrypt).

Now you also have a ALL IN ONE solution: vDump. But in case you want to decrypt quickly a lot of SELFs, you will keep using vitaDecrypt.

vitaDecrypt outputs only compressed decrypted segments. But you have to decompress these segments, or better, convert to ELF file format. This is the aim of this tool.

The output .elf are valid for RE and they can also be rebuilded into SELF using vita-make-fself.

### Usage :

1) In os0-, ud0- and vs0-, place the REAL files of your PSVita filesystem.
2) In ux0-/dump/, place the out folder that you got using vitadecrypt.
3) To rebuild ELF, on Windows run _RUNME.BAT.
4) After having rebuilded ELF, to rebuild SELF, on Windows run BATCH_MAKE_FSELF.BAT.
5) You can now use the ELFs in vitadecompiler, IDA, or radare2 or simply use an hexadecimal editor to look into them.

You can also hexedit as you want the ELFs then transform them into SELFs.

### WARNING : NEVER WRITE to your PSVita os0: nor vs0:.

--------------------------------------------------------------------------------

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

physmem_dumper usage
---
### How to get a dump:
Put physmem_dumper.skprx in ur0:tai/. You have 3 ways to start this kernel module:
1. EARLY KERNEL INIT. If you have enso, add this line at the start of ur0:tai/boot_config.txt:
-load ur0:tai/physmem_dumper.skprx
2. PRE SHELL INIT. Add this line in ur0:tai/config.txt under *KERNEL line: 
ur0:tai/physmem_dumper.skprx
3. POST SHELL INIT. Rename physmem_dumper.skprx to kplugin.txt and move this file to ux0: root. Launch kplugin loader app by xerpi. Wait until the app tells you to press START to exit.

### How to use the dump:
The output dump stored in ur0:dump/physmem-dump.bin is aimed to be loaded in IDA PRO using https://github.com/xyzz/vita-ida-physdump.

kernel_bootimage_extract usage
---

#### PSVita kernel bootimage mode
Obtain os0:/bootimage.elf using sceutils or FAGDec in ELF mode or by other means.

Run:

	./kernel_bootimage_extract bootimage.elf outdir

Then you can look in the new folder named "outdir" that now embeds many .elf files.

#### ePSP flash image mode
Obtain vs0:/app/NPXS10028/pcff.elf using sceutils or FAGDec in ELF mode or by other means.

Run:

	./kernel_bootimage_extract pcff.elf outdir -p

Then you can look in the new folder named "outdir" that now embeds ePSP flash files.

--------------------------------------------------------------------------------

kbl_elf_extract usage
---

Obtain kernel_boot_loader.elf.seg1 using sceutils.

Run:

	./kbl_elf_extract kernel_boot_loader.elf.seg1

Then you can look in the current folder that now embeds the extracted secure kernel modules ELF files.

--------------------------------------------------------------------------------

unarzl usage
---

Obtain a ARZL compressed file.

Run:

	./unarzl arzl_compressed_file.bin output_file.bin

or simply:

	./unarzl arzl_compressed_file.bin

Then you can look in the current folder that now embeds the extracted file.

--------------------------------------------------------------------------------

Kdumper usage
---
Before compiling, you have to change IP address to the one of your PC in main.c. After compiling, install the app on activated testkit/devkit <3.68. On PC listen TCP on port 9023 
. Run the PSVita app. Follow the instructions on screen. The kernel dump is sent to PC through socket.

kdump-extract usage
---
Obtain a kernel dump from Kdumper. Be sure that the vaddr of SceSysmem seg0 is at offset 0x0 of the kdump. Kdumper will write it to the file, but you must remove preceeding information.

Run:

	kdump_extract kdump.bin
	
--------------------------------------------------------------------------------

nids-extract usage
---
A db yaml will be generated to stdout using the exports of a specified ELF. You will need to specify a version to be inserted to yaml such as "3.60", which is shown in the following example.

#### For a single file:

	nids-extract <FW version> <binary name>.elf > <output filename>.yml

Example:

	nids-extract.exe 3.65 kd/acmgr.elf > acmgr.yml
	
#### For multiple files in one command in terminal:

	./nids-extract 3.60 $(find decrypted -name '*.elf' -not -path "./app/*") > db_lookup.yml`

or better:

	./nids-extract 3.60 $(find 360_fw/fs_dec -type f -name '*.elf' ! -name eboot.elf ! -path '*/sm/*.elf') > db_lookup.yml

--------------------------------------------------------------------------------

## Further thanks

zecoxao, xerpi, Team_molecule (yifanlu, Davee, Proxima, xyz), Hykem, St4rk, mr.gas, MajorTom, TheFloW, Rinnegatamante, cpasjuste, Freakler, sys (yasen), Nkekev, SilicaAndPina, motoharu, mathieulh, aerosoul, SKGleba, frangarcj, velocity, der0ad (wargio), SKFU, Vita3K, devnoname120, LemonHaze, SocraticBliss, PrincessOfSleeping, Sorvigolova, 173210, qwikrazor87, ColdBird, Princess of Sleeping
