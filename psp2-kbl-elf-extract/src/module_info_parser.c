#include <string.h>
#include <stdio.h>

#include "elf.h"
#include "sce_module_info.h"

unsigned int get_module_info_offset(void* elf_header) {
	unsigned int ret = 0;
	Elf32_Ehdr* ehdr = (Elf32_Ehdr*) elf_header;
	if (ehdr->e_shnum > 0) {
		Elf32_Shdr* shdrs = (Elf32_Shdr*) (elf_header + ehdr->e_shoff);
		for (unsigned int i = 0; i < ehdr->e_shnum; ++i) {
			if (shdrs[ehdr->e_shstrndx].sh_offset > 0x40000 || shdrs[i].sh_name > 0x4000)
				return 0;
			char* sh_name = (char*) (elf_header + shdrs[ehdr->e_shstrndx].sh_offset + shdrs[i].sh_name);
			if (!strncmp(".sceModuleInfo.rodata", sh_name, sizeof(".sceModuleInfo.rodata"))) {
				ret = shdrs[i].sh_offset;
				break;
			}
		}
	}
	if (ret == 0) {
		if (ehdr->e_type == ET_SCE_RELEXEC || (ehdr->e_type == ET_SCE_EXEC && ehdr->e_entry)) {
			Elf32_Phdr* phdrs = elf_header + ehdr->e_phoff;
			ret = phdrs[0].p_offset + ehdr->e_entry;
		} else {
			Elf32_Phdr* phdrs = elf_header + ehdr->e_phoff;
			ret = phdrs[0].p_paddr;
		}
	}
	return ret;
}