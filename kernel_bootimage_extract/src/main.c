#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "elf.h"


#define MAX_PATH_BUFFER_SIZE 256
#define ELF_BASE_VADDR 0x81000000
#define TEXT_SEGMENT_CHUNK_SIZE 0x400

typedef struct {
 uint32_t module_path_offset;
 uint32_t elf_offset;
 uint32_t elf_size;
} ENTRY_TABLE_NEW;

typedef struct {
 uint32_t elf_size;
 char module_path[256];
} MODULE_HEADER_OLD;

// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}


int main(int argc, char **argv){
	
	if (argc < 3){
		printf ("\nUsage: kernel_bootimage_extract.exe bootimage.elf outdir\n");
		return -1;
	}
	
	FILE *fp = fopen(argv[1], "rb");
	if (fp <= 0) {
		printf ("\nUsage: kernel_bootimage_extract.exe bootimage.elf outdir\n");
		printf ("\nNo input file found !\n");
		return -1;
	}
	
	rmdir(argv[2]);
	mkdir(argv[2], 777);
	char outdir[MAX_PATH_BUFFER_SIZE];
	sprintf(outdir, "%s/", argv[2]);
	
	unsigned char *text_segment_chunk = (unsigned char *) malloc(TEXT_SEGMENT_CHUNK_SIZE);
	fseek(fp, 0, SEEK_SET);
	fread(text_segment_chunk, TEXT_SEGMENT_CHUNK_SIZE, 1, fp);
	Elf32_Ehdr *elf = (Elf32_Ehdr *)text_segment_chunk;
	Elf32_Phdr *phdrs = (Elf32_Phdr *) (text_segment_chunk + elf->e_phoff);
	uint32_t text_seg_offset = phdrs[0].p_offset;
	printf("Text segment offset: %x\n", text_seg_offset);
	uint32_t bootimage_text_segment_size = *(uint32_t *)(text_segment_chunk + text_seg_offset + 0xC0) - ELF_BASE_VADDR;
	printf("bootimage text segment size:    0x%X\n", bootimage_text_segment_size);
	
	if (bootimage_text_segment_size ==  phdrs[0].p_filesz) { // Old bootimage format
		uint32_t offset = *(uint32_t *)(text_segment_chunk + text_seg_offset + 0xC4) - ELF_BASE_VADDR;
		printf("embedded modules start offset:    0x%X\n", offset);
		while ((text_seg_offset + offset) < bootimage_text_segment_size){
			unsigned char *data_in_buf = (unsigned char *) malloc(sizeof(MODULE_HEADER_OLD));
			fseek(fp, text_seg_offset + offset, SEEK_SET);
			fread(data_in_buf, sizeof(MODULE_HEADER_OLD), 1, fp);	
			MODULE_HEADER_OLD* module_header = (MODULE_HEADER_OLD *)data_in_buf;

			printf("elf size:     0x%X\n", module_header->elf_size);			
			printf("module path:    %s\n", module_header->module_path);
			unsigned char *string_buf = (unsigned char *) malloc(MAX_PATH_BUFFER_SIZE);
			string_buf = (unsigned char *) (str_replace(module_header->module_path, "os0:kd/", outdir));
			string_buf = (unsigned char *) (str_replace(string_buf, ".skprx", ".elf"));
			printf("output path:    %s\n", string_buf);
			
			FILE *fout = fopen(string_buf, "wb");
			free(string_buf);
			unsigned char *elf_buffer = (unsigned char *) malloc(module_header->elf_size);
			fseek(fp, text_seg_offset + offset + sizeof(MODULE_HEADER_OLD), SEEK_SET);
			fread(elf_buffer, module_header->elf_size, 1, fp);
			fwrite(elf_buffer, 1, module_header->elf_size, fout);
			fclose(fout);
			free(elf_buffer);
			
			offset += (sizeof(MODULE_HEADER_OLD) + module_header->elf_size);
			printf("next module offset:    0x%X\n", offset);
			free(data_in_buf);
		}
	} else { // New bootimage format
		uint32_t *temp = (uint32_t *) malloc(8);
		fseek(fp, phdrs[0].p_offset + 0xE0, SEEK_SET);
		fread(temp, 8, 1, fp);
		uint32_t table_size = (temp[1] - temp[0]) / sizeof(ENTRY_TABLE_NEW);
		free(temp);
		printf("table_size:     0x%X\n", table_size);
		for (uint32_t i = 0; i+1 < table_size; i++){
			unsigned char *data_in_buf = (unsigned char *) malloc(sizeof(ENTRY_TABLE_NEW));
			fseek(fp, phdrs[0].p_offset + 0xE4 + sizeof(ENTRY_TABLE_NEW) * i, SEEK_SET);
			fread(data_in_buf, sizeof(ENTRY_TABLE_NEW), 1, fp);
			ENTRY_TABLE_NEW *entry_table = (ENTRY_TABLE_NEW *)data_in_buf;
			
			printf("module_path_offset:    0x%X\n", entry_table->module_path_offset - ELF_BASE_VADDR);
			printf("elf_offset:       0x%X\n", entry_table->elf_offset - ELF_BASE_VADDR);
			
			printf("elf size:     0x%X\n", entry_table->elf_size);
			unsigned char *string_buf = (unsigned char *) malloc(MAX_PATH_BUFFER_SIZE);
			fseek(fp, phdrs[0].p_offset + entry_table->module_path_offset - ELF_BASE_VADDR, SEEK_SET);
			fread(string_buf, MAX_PATH_BUFFER_SIZE, 1, fp);
			printf("module path:    %s\n", string_buf);
			string_buf = (unsigned char *) (str_replace(string_buf, "os0:kd/", outdir));
			string_buf = (unsigned char *) (str_replace(string_buf, ".skprx", ".elf"));
			printf("output path:    %s\n", string_buf);
			
			FILE *fout = fopen(string_buf, "wb");
			free(string_buf);
			fseek(fp, phdrs[0].p_offset + entry_table->elf_offset - ELF_BASE_VADDR, SEEK_SET);
			unsigned char *elf_buffer = (unsigned char *) malloc(entry_table->elf_size);
			fread(elf_buffer, entry_table->elf_size, 1, fp);
			fwrite(elf_buffer, 1, entry_table->elf_size, fout);
			fclose(fout);
			
			free(data_in_buf);
			free(elf_buffer);
		}
	}
	free(elf);
	fclose(fp);
	
	return 0;
}