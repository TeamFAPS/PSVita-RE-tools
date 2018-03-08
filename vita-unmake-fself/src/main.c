//
// vita-unmake-fself -- by @dots-tb and @CelesteBlue-dev
// inspired by vitasdk vita-make-fself
// Thanks to: team_molecule, theflow, vitasdk members

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <zlib.h>

#include "self.h"
#include "elf.h"

static void usage(char *argv[]) {
	printf("%s binary", argv[0]);
}


int main(int argc, const char **argv) {
	FILE *fin = NULL;
	FILE *fout = NULL;
	if (argc != 2) {
		usage(argv);
		return 1;
	}
	char output_path[128];
	sprintf(output_path, "%s.elf", argv[1]);
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
	SCE_header *shdr = (SCE_header*)(input);
	if(shdr->magic != 0x454353) {
		perror("Not a self");
		goto error;
	}
	segment_info *sinfo = (segment_info *)(input + shdr->section_info_offset);
	ELF_header *ehdr = (ELF_header *)(input + shdr->elf_offset);
	fout = fopen(output_path, "wb");
	if (!fout) {
		perror("Failed to open output file");
		goto error;
	}
	fseek(fout, 0, SEEK_SET);
	if(!memcmp(input + shdr->header_len,"\177ELF\1\1\1", 8)) {
		ehdr->e_shoff = 0;
		ehdr->e_shnum = 0;
		ehdr->e_shstrndx = 0;
		fwrite(input + shdr->header_len, sizeof(ELF_header) + sizeof(Elf32_Phdr)*ehdr->e_phnum, 1, fout);		
	}
	for (int i=0; i < ehdr->e_phnum; i++) {
		uint8_t *destination = (uint8_t *)(input + sinfo[i].offset);
		Elf32_Phdr *phdr = (Elf32_Phdr *)(input + shdr->phdr_offset);
		if (sinfo[i].compression == 2) {
			size_t sz = phdr[i].p_memsz ? phdr[i].p_memsz : phdr[i].p_filesz;
			destination = (uint8_t *)calloc(1, sz);
			if (!destination) {
				perror("Error could not allocate memory.");
				goto error;
			}
			int ret = uncompress(destination, (long unsigned int*)&sz, (input + sinfo[i].offset), sinfo[i].length);
			if (ret != Z_OK) {
				fprintf(stderr, "Warning: could not uncompress segment %d, (No segment?), will copy segment: %d", i, ret);
				// destination = input + sinfo[i].offset;
			}
		}
		ELF_header *ehdr = (ELF_header *)(destination);
		if (!memcmp(destination, "\177ELF\1\1\1", 8)) {
			ehdr->e_shoff = 0;
			ehdr->e_shnum = 0;
			ehdr->e_shstrndx = 0;
		}
		fseek(fout, phdr[i].p_offset, SEEK_SET);
		fwrite(destination, phdr[i].p_filesz, 1, fout);
	}
	fclose(fout);
	return 0;
error:
	if (fin)
		fclose(fin);
	if (fout)
		fclose(fout);
	return 1;
	exit(0);
	return 0;
}