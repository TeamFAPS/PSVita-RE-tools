#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>

#include "elf.h"
#include "sce_module_info.h"
#include "module_info_parser.h"

#if (defined(_WIN32) || defined(__WIN32__))
#define mkdir(A, B) mkdir(A)
#endif


#define PATH_BUFFER_MAX_SIZE 256
#define BOOTIMAGE_ELF_BASE_VADDR 0x81000000
#define ELF_HEADER_CHUNK_SIZE 0x400


typedef struct {
  uint32_t path_offset;
  uint32_t file_offset;
  uint32_t file_size;
} ENTRY_TABLE_NEW;

typedef struct {
  uint32_t file_size;
  char path[256];
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
    for (count = 0; tmp = strstr(ins, rep); ++count)
        ins = tmp + len_rep;

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

void generateFolders(const char *path, const char *dest_path) {
	char temp_path[PATH_BUFFER_MAX_SIZE];
	strcpy(temp_path, path);
	char *folder = (char *)&temp_path;
	folder++;
	char current_path[PATH_BUFFER_MAX_SIZE];
	strcpy(current_path, dest_path);
	char *end_path;
	while ((end_path = strchr(folder, '/'))!= NULL) {
		*end_path = 0;
		snprintf(current_path, PATH_BUFFER_MAX_SIZE, "%s/%s", current_path, folder);
		printf("Creating folder %s\n", current_path);
		mkdir(current_path, 0777);
		folder = end_path + 1;
	}
}

int extract_file(char* output_path, void *addr, uint32_t size) {
	printf("Saving to: %s\nSize: %X bytes\n", output_path, size);
	FILE *fp = fopen(output_path, "wb");
	fwrite(addr, 1, size, fp);
	fclose(fp);
	return 0;
}

void print_usage(char *argv_0) {
	printf("Usage: %s in.elf [out_dir]\n", argv_0);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}
	_Bool pcff_mode = false;
	uint32_t seg_num = 0;
	uint32_t entries_start_offset = 0xE4;
	FILE *fp = fopen(argv[1], "rb");
	if (fp <= 0) {
		printf ("\nNo input file found!\n");
		print_usage(argv[0]);
		return -2;
	}
	uint32_t slide = BOOTIMAGE_ELF_BASE_VADDR; // TODO: check if this is always text segment virtual address (parse ELF header)
	char out_dir[PATH_BUFFER_MAX_SIZE];
	if (argc == 2)
		sprintf(out_dir, "kernel_bootimage_out");
	else
		sprintf(out_dir, "%s", argv[2]);
	void* elf_header_chunk = (void *) malloc(ELF_HEADER_CHUNK_SIZE);
	Elf32_Ehdr* ehdr = (Elf32_Ehdr *) elf_header_chunk;
	fread(ehdr, ELF_HEADER_CHUNK_SIZE, 1, fp);
	if (memcmp(ehdr->e_ident, "\177ELF\1\1\1", 8)) {
		print_usage(argv[0]);
		return -3;
	}
	unsigned int module_info_offset = get_module_info_offset(elf_header_chunk);
	if (module_info_offset == 0) {
		printf("Could not get module info!\n");
		return -4;
	}
	SceModuleInfo_common* mod_info_common = (SceModuleInfo_common*) malloc(sizeof(SceModuleInfo_common));
	fseek(fp, module_info_offset, SEEK_SET);
	fread(mod_info_common, sizeof(SceModuleInfo_common), 1, fp);
	printf("%s module detected.\n", mod_info_common->modname);
	if (!strcmp("SceSblPcffBin", mod_info_common->modname)) {
		pcff_mode = true;
		seg_num = 1;
		entries_start_offset = 0;
	} else if (strcmp("SceKernelBootimage", mod_info_common->modname)) {
		printf("This file does not embed a kernel bootimage.\n");
		return -3;
	}
	mkdir(out_dir, 0777);
	Elf32_Phdr *phdrs = (Elf32_Phdr *) (elf_header_chunk + ehdr->e_phoff);
	uint32_t text_seg_offset = phdrs[0].p_offset;
	printf("Text segment offset: %x\n", text_seg_offset);
	uint32_t bootimage_text_segment_size = *(uint32_t *)(elf_header_chunk + text_seg_offset + 0xC0) - slide;
	printf("bootimage text segment size:    0x%X\n", bootimage_text_segment_size);
	if (bootimage_text_segment_size ==  phdrs[0].p_filesz) { // Old bootimage format
		uint32_t offset = *(uint32_t *)(elf_header_chunk + text_seg_offset + 0xC4) - slide;
		printf("embedded modules start offset:    0x%X\n", offset);
		while ((text_seg_offset + offset) < bootimage_text_segment_size){
			unsigned char *data_in_buf = (unsigned char *) malloc(sizeof(MODULE_HEADER_OLD));
			fseek(fp, text_seg_offset + offset, SEEK_SET);
			fread(data_in_buf, sizeof(MODULE_HEADER_OLD), 1, fp);
			MODULE_HEADER_OLD* module_header = (MODULE_HEADER_OLD *)data_in_buf;
			printf("elf size: 0x%X\n", module_header->file_size);
			printf("module path: %s\n", module_header->path);
			unsigned char *string_buf = (unsigned char *) malloc(PATH_BUFFER_MAX_SIZE);
			string_buf = (unsigned char *) (str_replace(module_header->path, "os0:kd", out_dir));
			string_buf = (unsigned char *) (str_replace(string_buf, ".skprx", ".elf"));
			printf("output path: %s\n", string_buf);
			unsigned char *elf_buffer = (unsigned char *) malloc(module_header->file_size);
			fseek(fp, text_seg_offset + offset + sizeof(MODULE_HEADER_OLD), SEEK_SET);
			fread(elf_buffer, module_header->file_size, 1, fp);
			extract_file(string_buf, elf_buffer, module_header->file_size);
			free(string_buf);
			free(elf_buffer);
			offset += (sizeof(MODULE_HEADER_OLD) + module_header->file_size);
			printf("next module offset: 0x%X\n", offset);
			free(data_in_buf);
		}
	} else { // New bootimage or pcff format
		ENTRY_TABLE_NEW *entry_table;
		uint32_t i = 0;
		unsigned char *data_in_buf = (unsigned char *) malloc(sizeof(ENTRY_TABLE_NEW));
		fseek(fp, phdrs[seg_num].p_offset + entries_start_offset + sizeof(ENTRY_TABLE_NEW) * i, SEEK_SET);
		fread(data_in_buf, sizeof(ENTRY_TABLE_NEW), 1, fp);
		entry_table = (ENTRY_TABLE_NEW *)data_in_buf;
		if (pcff_mode)
			slide = *((uint32_t *)(data_in_buf)) & 0xFFFF0000;
		do {
			printf("path_offset:    0x%X\n", entry_table->path_offset - slide);
			printf("file_offset:       0x%X\n", entry_table->file_offset - slide);
			printf("file_size:     0x%X\n", entry_table->file_size);
			unsigned char *string_buf = (unsigned char *) malloc(PATH_BUFFER_MAX_SIZE);
			fseek(fp, phdrs[seg_num].p_offset + entry_table->path_offset - slide, SEEK_SET);
			fread(string_buf, PATH_BUFFER_MAX_SIZE, 1, fp);
			printf("module path:    %s\n", string_buf);
			generateFolders(string_buf, out_dir);
			unsigned char *string_buf_2 = (unsigned char *) malloc(PATH_BUFFER_MAX_SIZE);
			if (pcff_mode)
				snprintf(string_buf_2, PATH_BUFFER_MAX_SIZE, "%s/%s", out_dir, string_buf);
			else {
				string_buf_2 = (unsigned char *) (str_replace(string_buf, "os0:kd", out_dir));
				string_buf_2 = (unsigned char *) (str_replace(string_buf_2, ".skprx", ".elf"));
			}
			free(string_buf);
			printf("output path:    %s\n", string_buf_2);
			fseek(fp, phdrs[seg_num].p_offset + entry_table->file_offset - slide, SEEK_SET);
			unsigned char *elf_buffer = (unsigned char *) malloc(entry_table->file_size);
			fread(elf_buffer, entry_table->file_size, 1, fp);
			extract_file(string_buf_2, elf_buffer, entry_table->file_size);
			free(string_buf_2);
			free(elf_buffer);
			i++;
			fseek(fp, phdrs[seg_num].p_offset + entries_start_offset + sizeof(ENTRY_TABLE_NEW) * i, SEEK_SET);
			fread(data_in_buf, sizeof(ENTRY_TABLE_NEW), 1, fp);
			entry_table = (ENTRY_TABLE_NEW *)data_in_buf;
		} while (entry_table->path_offset != 0);
	}
	fclose(fp);
	return 0;
}
