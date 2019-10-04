#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "elf.h"


#define PATH_BUFFER_MAX_SIZE 256
#define MODULE_NAME_MAX_LEN 27

typedef struct _scemoduleinfo_common { // size is 0x20
  unsigned short attribute;
  unsigned char version[2];
  char name[MODULE_NAME_MAX_LEN];
  char struct_type;
} sceModuleInfo_common;

typedef struct _scemoduleinfo_prx2arm_3 { // size is 0x54
  sceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr libent_top;
  Elf32_Addr libent_end;
  Elf32_Addr libstub_top;
  Elf32_Addr libstub_end;
  Elf32_Word dbg_fingerprint;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr tls_start;
  Elf32_Addr tls_filesz;
  Elf32_Addr tls_memsz;
  Elf32_Addr exidx_start;
  Elf32_Addr exidx_end;
} sceModuleInfo_prx2arm_3;

typedef struct _scemoduleinfo_prx2arm_6 { // size is 0x5C
  sceModuleInfo_common c;
  Elf32_Addr gp_value;
  Elf32_Addr libent_top;
  Elf32_Addr libent_end;
  Elf32_Addr libstub_top;
  Elf32_Addr libstub_end;
  Elf32_Word dbg_fingerprint;
  Elf32_Addr tls_start;
  Elf32_Addr tls_filesz;
  Elf32_Addr tls_memsz;
  Elf32_Addr start_entry;
  Elf32_Addr stop_entry;
  Elf32_Addr exidx_start;
  Elf32_Addr exidx_end;
  Elf32_Addr extab_start;
  Elf32_Addr extab_end;
} sceModuleInfo_prx2arm_6;

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
	char *buffer = (char *)malloc(filesize);
	fseek(fp, 0, SEEK_SET);
	fread(buffer, filesize, 1, fp);
	fclose(fp);	

	int extract_file(char* path, uint32_t offset, uint32_t size) {
		printf("Saving to: %s\n", path);
		FILE *fp = fopen(path, "wb");
		fwrite(buffer + offset, 1, size, fp);
		fclose(fp);
		return 0;
	}

	char path[PATH_BUFFER_MAX_SIZE];

	for (uint32_t offset = 0; offset < filesize; ++offset) {
		uint32_t size = 0;
		Elf32_Ehdr *elf = (void *)buffer + offset;
		if (!memcmp(elf->e_ident, "\177ELF\1\1\1", 8)) {
			printf("Found ELF at 0x%X\n", offset);
			sceModuleInfo_common *mod_info;
			if (elf->e_entry) {
				Elf32_Phdr *phdrs = (void *)buffer + offset + elf->e_phoff;
				mod_info = (void *)buffer + offset + phdrs[0].p_offset + elf->e_entry;
				for (int i = 0; i < elf->e_phnum; ++i)
					size += phdrs[i].p_filesz;
			} else {
				if (elf->e_shnum > 0) {
					Elf32_Shdr *shdrs = (void *)buffer + offset + elf->e_shoff;
					for (int i = 0; i < elf->e_shnum; i++) {
						char *sh_name = (void *)buffer + offset + shdrs[elf->e_shstrndx].sh_offset + shdrs[i].sh_name;
						if (!strcmp(".sceModuleInfo.rodata", sh_name)) {
							mod_info = (void *)buffer + offset + shdrs[i].sh_offset;
						}
					}
					size = shdrs[elf->e_shnum - 1].sh_offset + shdrs[elf->e_shnum - 1].sh_size;
				}
			}
			snprintf(path, PATH_BUFFER_MAX_SIZE, "%s.elf", mod_info->name);
			extract_file(path, offset, size);
		} else if (!strncmp((char *)(buffer+offset), "ARZL", 4)) {
			printf("Found ARZL compressed data at 0x%X\n", offset);
			snprintf(path, PATH_BUFFER_MAX_SIZE, "arzl_compressed_data_0x%X.bin", offset);
			size = filesize - offset;
			extract_file(path, offset, size);
		}
	}
	free(buffer);

	return 0;
}