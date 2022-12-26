//vita-inject-elf
//@dots_tb @CelesteBlue123
// Thanks to: Motoharu, team_molecule, theflow

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <zlib.h>

#include "self.h"
#include "elf.h"
#include "sha256.h"

typedef struct {
        uint8_t *compressed_section;
        long section_offset;
        long section_size;
        int section_index;
} SectionDescr_t;

int section_descr_compare( const void *arg1, const void *arg2)
{
        return ((const SectionDescr_t *)arg1)->section_offset - ((const SectionDescr_t *)arg2)->section_offset;
}

static void usage(const char **argv)
{
	printf("%s <.elf/.bin> <.elf/.bin>",argv[0]);
}

int main(int argc, const char **argv)
{
	FILE *files[2] = {NULL, NULL};
	FILE *fout = NULL;
	uint8_t *elf_file = NULL,
		*self_file = NULL;
		
	if (argc != 3) {
		usage(argv);
		return 1;
	}

	files[0] = fopen(argv[1], "rb");
	files[1]  = fopen(argv[2], "rb");
	
	if (!files[0] || !files[1]) {
		perror("Failed to open file");
		goto error;
	}
	
	char output_path[1024];
		
	size_t self_sz,
			elf_sz;
	for(int i = 0; i < 2; i++) {
		fseek(files[i], 0, SEEK_END);
		size_t sz = ftell(files[i]);
		fseek(files[i], 0, SEEK_SET);
		uint8_t *input = calloc(1, sz);	
		if (!input) {
			perror("Failed to allocate buffer for input file");
			goto error;
		}
		if (fread(input, sz, 1, files[i]) != 1) {
			static const char s[] = "Failed to read input file";
			if (feof(files[i]))
				fprintf(stderr, "%s: unexpected end of file\n", s);
			else
				perror(s);
			goto error;
		}
		
		fclose(files[i]);
		files[i] = NULL;
		SCE_header *shdr = (SCE_header*)(input);
		if(shdr->magic != 0x454353) {
			if(memcmp(input, "\177ELF\1\1\1",8)==0) {
				elf_file = input;
				elf_sz = sz;
			} else {
				fprintf(stderr, "Unkown magic\n");
				free(input);
				goto error;
			}
		} else {
			strncpy(output_path, argv[i + 1], sizeof(output_path) - 1);
			self_file = input;
			self_sz = sz;
		}
	}

	if(!self_file || !elf_file) {
		fprintf(stderr, "Could not find elf or self\n");
		goto error;
	}
	
	fout = fopen(output_path, "wb");
	if (!fout) {
		perror("Failed to open output file");
		goto error;
	}
	
	SCE_header *shdr = (SCE_header*)(self_file);
	Elf32_Ehdr *ehdr_self = (Elf32_Ehdr*)(self_file + shdr->elf_offset);
	Elf32_Ehdr *ehdr = (Elf32_Ehdr*)(elf_file);

	if(ehdr->e_phnum!=ehdr_self->e_phnum) {
		fprintf(stderr, "Section headers number mismatch\n");
		goto error;
	}

	Elf32_Phdr *phdrs = (Elf32_Phdr*)(elf_file + ehdr->e_phoff);
	segment_info *sinfo = (segment_info *)(self_file + shdr->section_info_offset);
	SectionDescr_t *sections = (SectionDescr_t *)malloc(ehdr->e_phnum*sizeof(SectionDescr_t));

	for(int i = 0; i < ehdr->e_phnum; i++) {
		sections[i].compressed_section = (uint8_t *)malloc(phdrs[i].p_filesz);
                sections[i].section_offset = sinfo[i].offset;
                sections[i].section_size = phdrs[i].p_filesz;
                sections[i].section_index = i;
		if(!sections[i].compressed_section) {
			fprintf(stderr, "Could not allocate memory for compression for section %i!\n", i);
			goto error;
		}
		int ret = compress(sections[i].compressed_section, (long unsigned int *)&sections[i].section_size, elf_file + phdrs[i].p_offset, phdrs[i].p_filesz);
		if(ret != Z_OK) {
			fprintf(stderr, "Compressing error: %i\n", ret);
			free(sections[i].compressed_section);
			continue;
		}
	}

        qsort( sections, ehdr->e_phnum, sizeof(SectionDescr_t), section_descr_compare);

        size_t offset_correction = 0;
        for(int i = 0; i < ehdr->e_phnum; i++) {
                int init_index = sections[i].section_index;
                size_t seg_sz = sections[i].section_size;

                sinfo[init_index].offset += offset_correction;
                if(sinfo[init_index].length != seg_sz) {
			fprintf(stderr, "Compressed %d to diff size %lx - %lx\n", init_index, sinfo[init_index].length, seg_sz);
			if(sinfo[init_index].length < seg_sz) {
				fprintf(stderr, "Compressed size too big!\n");
				offset_correction += seg_sz - sinfo[init_index].length;
				fprintf(stderr, "Offset correction set: %lx\n", offset_correction);
				sinfo[init_index].length = seg_sz;
			}				
		}

        }
        for(int i = 0; i < ehdr->e_phnum; i++) {
                int init_index = sections[i].section_index;

		fseek(fout, sinfo[init_index].offset, SEEK_SET);
		fwrite(sections[i].compressed_section, sinfo[init_index].length, 1, fout);
		free(sections[i].compressed_section);
        }
	
	uint8_t elf_digest[0x20];
	
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, elf_file, elf_sz);
	sha256_final(&ctx, elf_digest);
	
	PSVita_CONTROL_INFO *control_info = (PSVita_CONTROL_INFO *)(self_file + shdr->controlinfo_offset);
	while(control_info->next) {
		switch(control_info->type) {
			case 4:
				memcpy(control_info->PSVita_elf_digest_info.elf_digest, &elf_digest, sizeof(elf_digest));
				break;
			case 5:
				memset(&control_info->PSVita_npdrm_info, 0, sizeof(control_info->PSVita_npdrm_info));
				break;
		}
		control_info = (PSVita_CONTROL_INFO*)((char*)control_info + control_info->size);
	}
	
	fseek(fout, 0, SEEK_SET);
	fwrite(self_file, shdr->header_len, 1, fout);
	fseek(fout, shdr->header_len, SEEK_SET);
	fwrite(self_file + shdr->elf_offset, sizeof(Elf32_Ehdr), 1, fout);
	fseek(fout, shdr->header_len + ehdr->e_phoff, SEEK_SET);
	fwrite(self_file + shdr->phdr_offset, ehdr->e_phnum, ehdr->e_phentsize, fout);
	
	fclose(fout);

	return 0;
error:
	for(int i = 0; i < 2; i++) {
		if(files[i]) {
			fclose(files[i]);
		}
	}
	if(fout) 
		fclose(fout);
	if(self_file)
		free(self_file);
	if(elf_file)
		free(elf_file);
	return 1;
	exit(0);
	return 0;
}
