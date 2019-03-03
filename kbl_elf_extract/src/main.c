#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "elf.h"

typedef struct {
	uint16_t attr;
	uint16_t ver;
	uint8_t name[27];
	uint8_t type;
	uint32_t gp;
	uint32_t expTop;
	uint32_t expBtm;
	uint32_t impTop;
	uint32_t impBtm;
	uint32_t nid;
	uint32_t unk[3];
	uint32_t start;
	uint32_t stop;
	uint32_t exidxTop;
	uint32_t exidxBtm;
	uint32_t extabTop;
	uint32_t extabBtm;
} SceModuleInfo;

int main(int argc, char **argv){
	
	if (argc < 2) {
		printf("\nusage: kbl_elf_extract file.elf/ \n");
		return -1;
	}
	
	FILE *fp = fopen(argv[1], "rb");
	fseek(fp, 0, SEEK_END);
	uint32_t sz = ftell(fp);
	char *input = (char *) malloc(sz);
	fseek(fp, 0, SEEK_SET);
	fread(input, sz, 1, fp);
	fclose(fp);	
	
	uint32_t offset = 0;
	while (offset < sz) {
		Elf32_Ehdr *elf;
		elf = (void *)input + offset;
		if (!memcmp(elf->e_ident, "\177ELF\1\1\1", 8)) {
			printf("Found ELF @%x\n", offset);
			char outname[32];
			
			Elf32_Phdr *phdrs = (void *)input + offset + elf->e_phoff;
			SceModuleInfo *mod_info = (void *)input + offset + phdrs[0].p_offset + elf->e_entry;
			printf("Mod_info  @%x\n", offset + phdrs[0].p_offset + elf->e_entry);
			printf("EP  @%x\n", elf->e_entry);
			if (!elf->e_entry) {
				printf("NO ENTRYPOINT DETECTED! Seeking .sceModuleInfo.rodata... \n");
				if (elf->e_shnum > 0) {
					Elf32_Shdr *shdr = (void *)input + offset+ elf->e_shoff;
					for (int i = 0; i < elf->e_shnum; i++) {
						char *sh_name = (void *)input + offset + shdr[elf->e_shstrndx].sh_offset + shdr[i].sh_name;
						printf("%i %s @%x (sz: %x)\n", i, sh_name, shdr[i].sh_offset, shdr[i].sh_size);
						if (!strcmp(".sceModuleInfo.rodata", sh_name)) {
							printf("Found .sceModuleInfo.rodata!\n");
							mod_info = (void *)input + offset + shdr[i].sh_offset;
						}
					}
				} 
			}
			snprintf(outname, 32, "%s.elf", mod_info->name);
			printf("Saving to: %s\n", outname);
			fp = fopen(outname, "wb");
			
			Elf32_Shdr *shdrs = (void *)input + offset + elf->e_shoff;
			fwrite(input + offset, 1, shdrs[elf->e_shnum  - 1].sh_offset + shdrs[elf->e_shnum  - 1].sh_size, fp);
			fclose(fp);
		} else if (!strncmp((char *)(input+offset), "ARZL", 4)) {
			printf("Found ARZL compressed data @%x\n", offset);
			
			char outname[32];
			snprintf(outname, 32, "arzl_compressed_data_%X.bin", offset);
			printf("Saving to: %s\n", outname);
			fp = fopen(outname, "wb");
			fwrite(input + offset, 1, sz - offset, fp);
			fclose(fp);
		}
		offset++;
	}
	free(input);

	return 0;
}