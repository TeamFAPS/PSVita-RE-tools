#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>

#include "elf.h"
#include "sce_module_info.h"
#include "module_info_parser.h"
#include "unarzl.h"

#if (defined(_WIN32) || defined(__WIN32__))
#define mkdir(A, B) mkdir(A)
#endif

#define PATH_BUFFER_MAX_SIZE 256


int extract_file(char* output_path, void *addr, uint32_t size) {
	printf("Saving to: %s\nSize: %X bytes\n", output_path, size);
	FILE *fp = fopen(output_path, "wb");
	fwrite(addr, 1, size, fp);
	fclose(fp);
	return 0;
}

void print_usage(char *argv_0) {
	printf("\n\nUsage: %s kernel_boot_loader.elf.seg1 [out_dir]\n", argv_0);
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
	char tmp_path[PATH_BUFFER_MAX_SIZE];
	if (argc == 2)
		sprintf(output_path, "kbl_elf_out");
	else
		sprintf(output_path, "%s", argv[2]);
	mkdir(output_path, 777);

	unsigned int size = 1;
	for (unsigned int offset = 0; offset < filesize-8; offset += size) {
		//printf("%X\n", offset);
		void *current_addr = buffer + offset;
		Elf32_Ehdr* ehdr = (Elf32_Ehdr*) current_addr;
		if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
extract_elf:
			printf("Found ELF at file offset 0x%X\n", offset);
			
			// Get module info
			unsigned int module_info_offset = get_module_info_offset(current_addr);
			if (module_info_offset == 0) {
				printf("Could not get module info!\n");
				size = 1;
				continue;
			}
			SceModuleInfo_common* mod_info_common = (SceModuleInfo_common*) (current_addr + module_info_offset);
			printf("%s module detected.\n", mod_info_common->modname);
			
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
			printf("ELF size: %i\n", size);
			
			snprintf(tmp_path, PATH_BUFFER_MAX_SIZE, "%s/%s.elf", output_path, mod_info_common->modname);
			extract_file(tmp_path, current_addr, size);
		} else if (!strncmp((char *)current_addr, "ARZL", 4)) {
			printf("Found ARZL compressed data at file offset 0x%X\n", offset);
			size = filesize - offset; // Since actual decompressed size is unknown, better keep too much than not enough
			current_addr = unarzl(current_addr, &size);
			ehdr = current_addr;
			if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
				goto extract_elf;
			} else {
				snprintf(tmp_path, PATH_BUFFER_MAX_SIZE, "%s/arzl_decompressed_data_0x%X.bin", output_path, offset);
				extract_file(tmp_path, current_addr, size);
			}
		}
	}
	free(buffer);

	return 0;
}