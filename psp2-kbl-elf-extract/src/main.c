#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "elf.h"
#include "sce_module_info.h"
#include "unarzl.h"

#define ET_SCE_RELEXEC 0xFE04
#define ET_SCE_EXEC 0xFE00
#define ET_SCE_UNK 0xFFA5
#define ET_SCE_UNK_EXEC 0x0002

#define PATH_BUFFER_MAX_SIZE 256


int extract_file(char* output_path, void *addr, uint32_t size) {
	printf("Saving to: %s\nSize: %X bytes\n", output_path, size);
	FILE *fp = fopen(output_path, "wb");
	fwrite(addr, 1, size, fp);
	fclose(fp);
	return 0;
}

void print_usage(char *argv_0) {
	printf("\n\nUsage: %s kernel_boot_loader.elf.seg1\n", argv_0);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	FILE *fp = fopen(argv[1], "rb");
	fseek(fp, 0, SEEK_END);
	uint32_t filesize = ftell(fp);
	void *buffer = malloc(filesize);
	fseek(fp, 0, SEEK_SET);
	fread(buffer, filesize, 1, fp);
	fclose(fp);

	// TODO: get kernel_boot_loader's first segment directly from kernel_boot_loader ELF

	char output_path[PATH_BUFFER_MAX_SIZE];

	for (uint32_t offset = 0; offset < filesize-8; ++offset) {
		//printf("%X\n", offset);
		uint32_t size = 0;
		void *current_addr = buffer + offset;
		Elf32_Ehdr *ehdr = current_addr;
		if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
extract_elf:
			printf("Found ELF at file offset 0x%X\n", offset);
			
			// Get module info address
			SceModuleInfo_common *mod_info = 0;
			if (ehdr->e_shnum > 0) {
				Elf32_Shdr *shdrs = current_addr + ehdr->e_shoff;
				for (uint32_t i = 0; i < ehdr->e_shnum; i++) {
					char *sh_name = current_addr + shdrs[ehdr->e_shstrndx].sh_offset + shdrs[i].sh_name;
					if (!strcmp(".sceModuleInfo.rodata", sh_name))
						mod_info = current_addr + shdrs[i].sh_offset;
				}
			}
			if (mod_info == 0) {
				if (ehdr->e_type == ET_SCE_RELEXEC || (ehdr->e_type == ET_SCE_EXEC && ehdr->e_entry)) {
					Elf32_Phdr *phdrs = current_addr + ehdr->e_phoff;
					mod_info = current_addr + phdrs[0].p_offset + ehdr->e_entry;
				} else {
					Elf32_Phdr *phdrs = current_addr + ehdr->e_phoff;
					mod_info = current_addr + phdrs[0].p_paddr;
				}
			}
			printf("Module info base at file offset 0x%X\n", mod_info);
			
			// Get ELF size
			if (ehdr->e_shnum > 0) {
				Elf32_Shdr *shdrs = current_addr + ehdr->e_shoff;
				size = shdrs[ehdr->e_shnum - 1].sh_offset + shdrs[ehdr->e_shnum - 1].sh_size;
			} else {
				Elf32_Phdr *phdrs = current_addr + ehdr->e_phoff;
				size = ehdr->e_phoff + ehdr->e_phnum * sizeof(Elf32_Phdr);
				size += 0x10 - (size & 0xF);
				for (int i = 0; i < ehdr->e_phnum; ++i) {
					uint32_t new_size = phdrs[i].p_offset + phdrs[i].p_filesz;
					if (new_size > size)
						size = new_size;
				}
			}
			
			snprintf(output_path, PATH_BUFFER_MAX_SIZE, "%s.elf", mod_info->modname);
			extract_file(output_path, current_addr, size);
		} else if (!strncmp((char *)current_addr, "ARZL", 4)) {
			printf("Found ARZL compressed data at file offset 0x%X\n", offset);
			size = filesize - offset; // Since actual decompressed size is unknown, better keep too much than not enough
			current_addr = unarzl(current_addr, &size);
			ehdr = current_addr;
			if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
				goto extract_elf;
			} else {
				snprintf(output_path, PATH_BUFFER_MAX_SIZE, "arzl_decompressed_data_0x%X.bin", offset);
				extract_file(output_path, current_addr, size);
			}
		}
	}
	free(buffer);

	return 0;
}