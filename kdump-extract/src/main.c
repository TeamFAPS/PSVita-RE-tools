//module_finder
// Made by: @dots_tb @CelesteBlue123
// Thanks to: team_molecule, theflow

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "self.h"
#include "elf.h"

#define KNOWN_MODNAME "SceSysmem" //Set this to a module that you know the vaddr for

typedef struct { // size = 0x16 * sizeof(int) = 0x58
	uint32_t sysmem_seg0_addr;
	uint32_t kdump_start_offset;
	uint32_t leaked_sysmem_addr;
	uint32_t leaked_kstack_addr;
	uint32_t leaked_info[0x12];
} kdump_info;


static void usage(char *argv[]) {
	printf("Usage: %s kdump.bin\n", argv[0]);
}

int main(int argc, char **argv) {
	FILE *fin = NULL;
	if (argc != 2) {
		usage(argv);
		return 1;
	}
	fin = fopen(argv[1], "rb");
	
	if (!fin) {
		perror("Failed to open input file");
		goto error;
	}
	fseek(fin, 0, SEEK_END);
	size_t sz = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	uint8_t *input = calloc(1, sz);	
	if (!input) {
		perror("Failed to allocate buffer for input file");
		goto error;
	}
	if (fread(input, sz, 1, fin) != 1) {
		static const char s[] = "Failed to read input file";
		if (feof(fin))
			fprintf(stderr, "%s: unexpected end of file\n", s);
		else
			perror(s);
		goto error;
	}
	fclose(fin);
	fin = NULL;
	kdump_info *info = (kdump_info*)input;
	uint32_t sysmem_seg0_addr = info->sysmem_seg0_addr;
	
	printf("Sysmem seg0 vaddr set to: %08X\n\n", sysmem_seg0_addr);
	
	SceModuleInfo *mod_info = (SceModuleInfo *)(input);
	uint32_t off = 0;
	uint32_t vaddr_off = 0;
	
		uint8_t check[8] = { 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x01, 0x01};	
		while (off < sz) {
			unsigned int i = 0;
			while (i < sizeof(check)) {
				if (off > sz)
					break;
				if (*(input+off+i) != check[i]) {
					if(i==6) {
						i++;
						continue;
					}
					off++;
					i = 0;
					continue;
				}
				i++;	
			}
			if (off < sz) {
				uint32_t mod_info_off = off + sizeof(check) - 0x4;
				mod_info = (SceModuleInfo*)(input + mod_info_off);
				uint32_t ep = mod_info->expTop - sizeof(SceModuleInfo);
				printf("Module Name: %s (@0x%x)\n", mod_info->name, mod_info_off);
				printf("Module NID: 0x%X\n", mod_info->nid);
				printf("Module Entry: 0x%x\n", ep);
				
				
				
				uint32_t seg0_start = mod_info_off - ep;
				if ((int)seg0_start > 0) {
					printf("Segment 0 start: 0x%x\n", seg0_start);
					uint32_t seg0_end = off;
					for (int j = 0; j < 0x100; j++) {
						if (*(input + seg0_end + j) != *(input + seg0_end)) {
							j = 0;
							seg0_end++;
						}
					}
					printf("Repeating bit set to: %x\n", *(input + seg0_end));
					size_t seg0_sz = seg0_end - seg0_start;
					printf("Segment 0 end: 0x%x (sz: 0x%x)\n", seg0_end, (unsigned int)seg0_sz);
					if (strncmp(KNOWN_MODNAME, (char *)mod_info->name, sizeof(KNOWN_MODNAME)-1) == 0 && !vaddr_off) {
						vaddr_off = seg0_start;
						printf("Vaddr %x will correspond to offset: 0x%x\n", sysmem_seg0_addr, seg0_start);
					}
					if (vaddr_off) {
						printf("Vaddr: %x\n", sysmem_seg0_addr + seg0_start - vaddr_off);
						char outpath[128];
						snprintf(outpath, sizeof(outpath), "%s.elf", mod_info->name);
						FILE *fout = fopen(outpath, "wb");
						
						Elf32_Ehdr myhdr = { 0 };
						memcpy(myhdr.e_ident, "\177ELF\1\1\1", 8);
						myhdr.e_type = ET_CORE;
						myhdr.e_machine = 0x28;
						myhdr.e_version = 1;
						myhdr.e_entry = ep;
						myhdr.e_phoff = 0x34;
						myhdr.e_flags = 0x05000000U;
						myhdr.e_ehsize = 0x34;
						myhdr.e_phentsize = 0x20;
						myhdr.e_phnum = 1;
						
						Elf32_Phdr phdr;
						phdr.p_type = PT_LOAD;
						phdr.p_paddr = 0;
						phdr.p_align = 1;
						phdr.p_flags = (PF_R | PF_X);
						phdr.p_offset = 0xC0;
						phdr.p_vaddr = (Elf32_Addr)sysmem_seg0_addr + seg0_start - vaddr_off;
						phdr.p_memsz = seg0_sz;
						phdr.p_filesz = seg0_sz;

						
						fseek(fout, 0, SEEK_SET);
						fwrite(&myhdr, 1 , sizeof(Elf32_Ehdr), fout);
						
						fseek(fout, myhdr.e_phoff, SEEK_SET);
						fwrite(&phdr, 1 , sizeof(Elf32_Phdr), fout);
						
						fseek(fout, phdr.p_offset, SEEK_SET);
						fwrite(input + seg0_start, 1 , seg0_sz, fout);
						fclose(fout);
						
						printf("Saving ELF to: %s\n", outpath);
					}
				} else
					printf("Segment 0 is out of bounds! %x\n", seg0_start);
				printf("\n");
			}
			off++;
		}
	return 0;
	
error:
	if (fin)
		fclose(fin);
	return 1;
	exit(0);
	return 0;
}