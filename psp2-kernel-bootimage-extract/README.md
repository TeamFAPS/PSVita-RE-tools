# psp2-kernel-bootimage-extract

A PC program that extracts kernel modules ELF files from bootimage.elf or PSPemu flash files from pcff.elf.

## Usage

#### PS Vita kernel bootimage mode

Obtain os0:/bootimage.elf using sceutils or FAGDec in ELF mode or by other means.

Run:

	./psp2-kernel-bootimage-extract bootimage.elf out_dir

Then you can look in the new folder named "outdir" that now embeds many .elf files.

#### PS Vita PSPemu flash image mode

Obtain vs0:/app/NPXS10028/pcff.elf using sceutils or FAGDec in ELF mode or by other means.

Run:

	./psp2-kernel-bootimage-extract pcff.elf out_dir

Then you can look in the new folder named "outdir" that now embeds PSPemu flash files.

## Credits
CelesteBlue, zecoxao
