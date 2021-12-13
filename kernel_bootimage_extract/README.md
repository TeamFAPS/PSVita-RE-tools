# kernel_bootimage_extract
A PC tool that extracts embedded kernel modules ELFs from bootimage.elf or embedded ePSP flash files from pcff.elf.

## Usage

#### PSVita kernel bootimage mode
Obtain os0:/bootimage.elf using sceutils or FAGDec in ELF mode or by other means.

Run:

	./kernel_bootimage_extract bootimage.elf outdir

Then you can look in the new folder named "outdir" that now embeds many .elf files.

#### ePSP flash image mode
Obtain vs0:/app/NPXS10028/pcff.elf using sceutils or FAGDec in ELF mode or by other means.

Run:

	./kernel_bootimage_extract -pcff pcff.elf outdir

Then you can look in the new folder named "outdir" that now embeds ePSP flash files.

## Credits
CelesteBlue, zecoxao, dots-tb
