#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "elf.h"
#include "unarzl.h"

#define ET_SCE_RELEXEC 0xFE04
#define ET_SCE_EXEC 0xFE00
#define ET_SCE_UNK 0xFFA5
#define ET_SCE_UNK_EXEC 0x0002

#define PATH_BUFFER_MAX_SIZE 256

#define MODULE_NAME_MAX_LEN 27

typedef struct sceModuleInfo_common { // size is 0x20
  unsigned short modattr;
  unsigned char modver[2];
  char name[MODULE_NAME_MAX_LEN];
  char infover;
} sceModuleInfo_common;

typedef struct sceModuleInfo_prx2arm_v3 { // size is 0x54
  sceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr libent_top;
  Elf32_Addr libent_btm;
  Elf32_Addr libstub_top;
  Elf32_Addr libstub_btm;
  Elf32_Word fingerprint;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr tls_start;
  Elf32_Addr tls_filesz;
  Elf32_Addr tls_memsz;
  Elf32_Addr exidx_top;
  Elf32_Addr exidx_btm;
} sceModuleInfo_prx2arm_v3;

typedef struct sceModuleInfo_prx2arm_v6 { // size is 0x5C
  sceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr libent_top;
  Elf32_Addr libent_btm;
  Elf32_Addr libstub_top;
  Elf32_Addr libstub_btm;
  Elf32_Word fingerprint;
  Elf32_Addr tls_start;
  Elf32_Addr tls_filesz;
  Elf32_Addr tls_memsz;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr exidx_top;
  Elf32_Addr exidx_btm;
  Elf32_Addr extab_top;
  Elf32_Addr extab_btm;
} sceModuleInfo_prx2arm_v6;

int extract_file(char* output_path, void *addr, uint32_t size) {
	printf("Saving to: %s\n", output_path);
	FILE *fp = fopen(output_path, "wb");
	fwrite(addr, 1, size, fp);
	fclose(fp);
	return 0;
}

void print_usage(char *argv_0) {
	printf("\n\nUsage: %s kernel_boot_loader.elf.seg1\n", argv_0);
}

int main(int argc, char **argv){
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

	// TODO: get first segment directly from ELF

	char output_path[PATH_BUFFER_MAX_SIZE];

	for (uint32_t offset = 0; offset < filesize; ++offset) {
		uint32_t size = 0;
		void *current_addr = buffer + offset;
		Elf32_Ehdr *ehdr = current_addr;
		if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
			printf("Found ELF at 0x%X\n", offset);
			
			// Get module info address
			sceModuleInfo_common *mod_info = 0;
			if (ehdr->e_shnum > 0) {
				Elf32_Shdr *shdrs = current_addr + ehdr->e_shoff;
				for (int i = 0; i < ehdr->e_shnum; i++) {
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
			printf("Module info base at 0x%X\n", mod_info);
			
			// Get ELF size
			if (ehdr->e_shnum > 0) {
				Elf32_Shdr *shdrs = current_addr + ehdr->e_shoff;
				size = shdrs[ehdr->e_shnum - 1].sh_offset + shdrs[ehdr->e_shnum - 1].sh_size;
			} else {
				Elf32_Phdr *phdrs = current_addr + ehdr->e_phoff;
				size = ehdr->e_phoff;
				for (int i = 0; i < ehdr->e_phnum; ++i)
					size += phdrs[i].p_filesz;
			}
			
			snprintf(output_path, PATH_BUFFER_MAX_SIZE, "%s.elf", mod_info->name);
			extract_file(output_path, current_addr, size);
		} else if (!strncmp((char *)current_addr, "ARZL", 4)) {
			printf("Found ARZL compressed data at 0x%X\n", offset);
			snprintf(output_path, PATH_BUFFER_MAX_SIZE, "arzl_compressed_data_0x%X.bin", offset);
			size = filesize - offset; // Since actual decompressed size is unknown, better too much than not enough
			extract_file(output_path, current_addr, size);

			current_addr = unarzl(current_addr, &size);

			ehdr = current_addr;
			if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
				printf("Found ELF at 0x%X\n", offset);
				
				// Get module info address
				sceModuleInfo_common *mod_info = 0;
				if (ehdr->e_shnum > 0) {
					Elf32_Shdr *shdrs = current_addr + ehdr->e_shoff;
					for (int i = 0; i < ehdr->e_shnum; i++) {
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
				printf("Module info base at 0x%X\n", mod_info);
				
				// Get ELF size
				if (ehdr->e_shnum > 0) {
					Elf32_Shdr *shdrs = current_addr + ehdr->e_shoff;
					size = shdrs[ehdr->e_shnum - 1].sh_offset + shdrs[ehdr->e_shnum - 1].sh_size;
				} else {
					Elf32_Phdr *phdrs = current_addr + ehdr->e_phoff;
					size = ehdr->e_phoff;
					for (int i = 0; i < ehdr->e_phnum; ++i)
						size += phdrs[i].p_filesz;
				}
				
				snprintf(output_path, PATH_BUFFER_MAX_SIZE, "%s.elf", mod_info->name);
				extract_file(output_path, current_addr, size);
			} else {
				snprintf(output_path, PATH_BUFFER_MAX_SIZE, "arzl_decompressed_data_0x%X.bin", offset);
				extract_file(output_path, current_addr, size);
			}
		}
	}
	free(buffer);

	return 0;
}