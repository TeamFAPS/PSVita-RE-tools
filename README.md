# PSVita ReverseEngineering Tools
by @dots_tb and @CelesteBlue123 and other devs if they add their tools there


Every tool provided here will be licensed under GPLv2 unless stated otherwise.

What is that ?
This toolkit provides tools that aid in the development of vita homebrew and plugins. By providing tools that speed up development through automation of processes.

What it won’t do :
These tools are solely used to aid in homebrew development. It probably will NOT produce a major hack. It will not magically make you a vita god. It will not automatically hack the vita. These tools are purely tools, most of them already existed in some form or another.

THIS TOOLSET PURELY SPEEDS UP THE PROCESS OF DEVELOPMENT THROUGH EASE OF ACCESS AND DOES NOT GIVE YOU ANYTHING YOU ALL DID NOT HAVE.

I REPEAT, THESE TOOLS DO NOT PROVIDE ANYTHING NEW!

*** Remember: IF YOU DO NOT UNDERSTAND WHAT THESE TOOLS DO, IT MAY NOT BE FOR YOU! ***


--------------------------------------------------------------------------------

- USB logging solution - by dots_tb

A nerfed usbhostfs with the pspsh commands removed, this is really just used for “dependable logging” which is fast and runs in the background.

This mod just adds stdout which allows for easy debugging whether it a kernel plugin, user plugin, or homebrew application.

	+Adds USB stdout

	+Works on Windows now/Compiles on windows

	+Upgraded usb lib to libusb1.0

	+Stops MTP driver from running

	-Everything psp2shell related?

*Credits: Cpasjuste whom ported usbhostfs to vita

--------------------------------------------------------------------------------

- VitaDecompilerMod – by dots_tb

Free alternative to IDA, we added a few features such as:

	+More strings, including data section.

	+ELF and SELF support

	+Decompression

	+Fixed issues with NIDS being improperly found

	+Includes offsets and vaddr

	+Automatic entry point

	+There might be more?
	
*Credits: TheFlow for original VitaDecompiler and prxtool's original devs

--------------------------------------------------------------------------------

- vita-unmake-fself – by dots_tb

Decompresses a unencrypted SELF file (skprx, suprx, self, eboot.bin) into an ELF file.

--------------------------------------------------------------------------------

- That Hooker Got NIDS – by dots_tb

A tool that hooks specified NIDS automatically.

*Credits: xerpi for base code used, TheFlow for db.yml parsing

--------------------------------------------------------------------------------

- RebuildElf – by CelesteBlue

A PC tool that rebuilds ELF from segments

*Credits: zecoxao for the tutorial

--------------------------------------------------------------------------------

- vDump –

Dumps modules in a variety of ways (NOTE: NOT GAMES, THIS DOES NOT ENABLE PIRACY).

*Credits: zecoxao for vitadump(old/new), st4rk for alternative kernel dump, xerpi for both.

--------------------------------------------------------------------------------

- nids.txt / db.yml -

Some lists of functions names / NIDs / libraries / modules to be used with IDA, VitaDecompiler or other tools that will come later.

--------------------------------------------------------------------------------

Using the Tools:
--------------------------------------------------------------------------------


- Usbhostfs usage :

(WARNING – This mod may not work on testkits or devkits)

(WARNING – We have not tested with Vitashell’s USB mode)

(WARNING – Many errors will occur, do not use when using the vita for daily use)

Required Dependencies:

	Windows: mingw-w64-x86_64-libusb, mingw-w64-x86_64-readline
	
	(Use mingw64 to run, not msys?)
	
	Linux: libusb-1.0-0, readline (their dev packages)

	
	First you must compile everything, we may provide a Windows x86_64 binary due to the complexity of its compilation. (This includes the plugin (root folder), usbhost_pc, and pspsh).

The compiled plugin is a kernel plugin and should go under the kernel section of the config.txt included with Taihen. (You may use a plugin launcher such as the one xerpi has made)

When you are sure the plugin is running*, follow the next steps according to the operating system:

Linux:

	You may just run “usbhostfs_pc” in SUPER USER mode. Then run pspsh and connect the vita.

Windows:

	1. Plugin the vita into the pc.
	
	2. Download the program “Zadig”. This will provide generic USB drivers for the vita.
	
	3. In Zadig, make sure that “Options>List All Devices” is checked.
	
	4. In the dropdown menu, select the “PS Vita”. Make sure it is not the Type B driver, this means that the plugin is not running. 

	5. In the dropdown menu in which the green arrow is pointing, select “libusb-win32”.
	6. Install the driver.
	
	7. Unplug then restart the vita.
	
	8. Open the mingw64.exe included with msys (it is in the installation path). Make sure all the dependencies are installed listed above. (You may want to use Administration privileges)
	
	9. Be sure the plugin is running on the vita and plug it in to the PC. (You may ignore the -7 errors returned)
	
	10. Run usbhostfs_pc using mingw64.exe

	11. Run pspsh.exe using mingw64.exe
	
*NOTE: You can check “ux0:dump/psp2shell.txt” for logs that have not been sent through USB, make sure the folder exists first.
	
To check if it works, a simple way is to open Molecular Shell and activate FTP.

You should now have USB logging.

Use in your application you wish to log:

	ksceDebugPrintf, printf (when SceLibc is included such as in games), or sceClibPrintf


- VitaDecompilerMod usage :

Dependencies:

VitaDecompiler requires capstone (a disassembler) libraries and libyaml. (capstone must be compiled from source on Windows). Make sure you install the *.a and header files to the right directories.

	Run: ./vitadecompiler binary db.yml > output.c
	
Interpreting the Output:

The top section is a printed NIDS table which gives the exports and imports of a module. This information will provide the offsets, virtual address, NID, library name, library NID, and the NID Name (or generated name).

Each function has a virtual address and offset displayed next to it. This offset given (if not for a function that has a NID) maybe hooked with Taihen.

Most strings or values are accompanied by a g_text_addr which gives you the original address. The address is then checked repeatedly until a non-address is found. 


- vita-unmake-fself

Dependencies: zlib

	Run: ./vita-unmake-fself binary
(You most likely will be able to drag-and-drop also)
The output will be produced in the same folder with .elf appended on to the end of the original file name.


- That Hooker got NID

Dependencies: zlib, libyaml

	Run: ./THGN binary <all/library_name/exports/imports> <kernel/user> db.yml <sys:1/0>
	
All: Every NID will be hooked. This will try to hook as an export at first, then attempt hook it as an import.
Library_name: Every NID of a library (such as “SceCtrl”) within the module specified will be hooked. This will try to hook as an export at first, then attempt hook it as an import.
Exports: Every export NID will be hooked.
Imports: Every import NID will be hooked.
Kernel: The generated code will work in kernel space.
User: The generated code will work in user space.
Sys: You may choose 1 or 0 to enable or disable syscall mode. You may omit this argument. Some functions will not log unless it enters syscall mode. If you do not see anything within your logs, you may try this option. Try not to use it.


- vDump usage :
Everything is contained within the vpk. Follow the onscreen instructions provided in the application.

Methods:

VitaDump(old) – Only works with user mode modules, this method dumps the sections out of memory.

VitaDump(new) – MAKE SURE YOU RUN THE KPLUGIN (scroll down to kplugin and hit X to enable it). This method uses system functions to decrypt segments of SYSTEM applications.


- ELF-builder usage :

To reverse PSVita, you need some dumps of the PSVita modules. These dumps are either memory dumps (St4rk's vitadump) or decrypted SELF (zecoxao's vitadump).

Now you also have a ALL IN ONE solution : VDump. But in case you want to decrypt quickly a lot of SELFs, you will keep using zecoxao's vitadump.

zecoxao's vitadump outputs only compressed segments. But to reverse, you have to use decompressed segment, or better : ELF file format.

So when you have segments and want to transform them into an ELF, use this tool.

The output .elf are valid for RE and they can be rebuilded to SELF if needed using vita-make-fself.

Please read the provided README.txt for How to use informations.


Further thanks:

zecoxao, xerpi, Team_molecule, TheFlow, Freakler, sys(yasen), Nkekev, SilicaAndPina, mr.gas, MajorTom, motoharu
