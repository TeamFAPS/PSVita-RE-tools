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


int extract_file(char* output_path, void *addr, unsigned int size) {
	printf("Saving file to: %s\nFile size: %d bytes.\n", output_path, size);
	FILE *fp = fopen(output_path, "wb");
	fwrite(addr, 1, size, fp);
	fclose(fp);
	return 0;
}

char output_path[PATH_BUFFER_MAX_SIZE];
char tmp_path[PATH_BUFFER_MAX_SIZE];

unsigned int extract_elf(void* addr) {
	unsigned int size = 0;
	// Get module info
	unsigned int module_info_offset = get_module_info_offset(addr);
	if (module_info_offset == 0) {
		printf("Could not get module info!\n");
		return 0;
	}
	SceModuleInfo_common* mod_info_common = (SceModuleInfo_common*) (addr + module_info_offset);
	printf("%s module detected.\n", mod_info_common->modname);

	// Get ELF size
	Elf32_Ehdr* ehdr = (Elf32_Ehdr*) addr;
	if (ehdr->e_shnum > 0) {
		Elf32_Shdr *shdrs = addr + ehdr->e_shoff;
		size = shdrs[ehdr->e_shnum - 1].sh_offset + shdrs[ehdr->e_shnum - 1].sh_size;
	} else {
		Elf32_Phdr *phdrs = addr + ehdr->e_phoff;
		size = ehdr->e_phoff + ehdr->e_phnum * sizeof(Elf32_Phdr);
		size += 0x10 - (size & 0xF);
		for (int i = 0; i < ehdr->e_phnum; ++i) {
			unsigned int new_size = phdrs[i].p_offset + phdrs[i].p_filesz;
			if (new_size > size)
				size = new_size;
		}
	}
	printf("ELF size: %d bytes.\n", size);

	snprintf(tmp_path, PATH_BUFFER_MAX_SIZE, "%s/%s.elf", output_path, mod_info_common->modname);
	extract_file(tmp_path, addr, size);
	return size;
}

void print_usage(char *argv_0) {
	printf("\n\nUsage: %s kernel_boot_loader.elf.seg1 [out_dir]\n", argv_0);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	FILE *fin = fopen(argv[1], "rb");
	if (!fin) {
		perror("Failed to open input file");
		goto error;
	}
	fseek(fin, 0, SEEK_END);
	unsigned int filesize = ftell(fin);
	void *buffer = malloc(filesize);
	fseek(fin, 0, SEEK_SET);
	fread(buffer, filesize, 1, fin);
	fclose(fin);

	if (argc == 2)
		sprintf(output_path, "kbl_elf_out");
	else
		sprintf(output_path, "%s", argv[2]);
	mkdir(output_path, 777);

	for (unsigned int offset = 0; offset < filesize-8; offset += 1) {
		void *current_addr = buffer + offset;
		Elf32_Ehdr* ehdr = (Elf32_Ehdr*) current_addr;
		if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
			printf("Found ELF at file offset 0x%X\n", offset);
			extract_elf(current_addr);
		} else if (!strncmp((char *)current_addr, "ARZL", 4)) {
			printf("Found LZRA compressed data at file offset 0x%X\n", offset);
			unsigned int tmp_size = filesize - offset; // Since compressed size is unknown yet, better keep too much than not enough
			void* work_buffer = malloc(tmp_size);
			memcpy(work_buffer, current_addr, tmp_size);
			work_buffer = unarzl(work_buffer, &tmp_size);
			ehdr = (Elf32_Ehdr*) work_buffer;
			if (!memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
				printf("Found LZRA encoded ELF at file offset 0x%X\n", offset);
				extract_elf(work_buffer);
			} else {
				snprintf(tmp_path, PATH_BUFFER_MAX_SIZE, "%s/lzra_decompressed_data_0x%X.bin", output_path, offset);
				extract_file(tmp_path, work_buffer, tmp_size);
			}
		}
	}
	free(buffer);

error:
	if (fin)
		fclose(fin);
	exit(0);
	return 0;
}