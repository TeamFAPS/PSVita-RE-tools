#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "elf.h"
#include "types.h"
#include "sce_module_info.h"
#include "module_info_parser.h"

#define ELF_HEADER_CHUNK_SIZE 0x400
#define MAX_FUNCTION_NAME_BUFFER_SIZE 256

typedef struct {
 uint32_t is_syscall;
 uint32_t function_nid;
 uint32_t function_name_offset;
} ENTRY_TABLE;


int get_movw_value_from_instr(int opcode) {
	int value = (opcode >> 8) & 0xFF;
	value |= (opcode << 4) & 0x700;
	value |= (opcode >> 7) & 0x800;
	value |= (opcode >> 12) & 0xF000;
	return value;
}

void print_usage(char *argv_0) {
	printf("\n\nUsage: %s syslibtrace.elf [-f]\n", argv_0);
}

int main(int argc, char **argv) {
	if (argc < 2){
		print_usage(argv[0]);
		return -1;
	}
	
	FILE *fp = fopen(argv[1], "rb");
	int extra_args = 0;
	if (argc >= 3)
		if (!strcmp(argv[2], "-f"))
			extra_args = 1;
	
	void *text_segment_chunk = malloc(ELF_HEADER_CHUNK_SIZE);
	fseek(fp, 0, SEEK_SET);
	fread(text_segment_chunk, ELF_HEADER_CHUNK_SIZE, 1, fp);
	Elf32_Ehdr *elf = (Elf32_Ehdr *)text_segment_chunk;
	Elf32_Phdr *phdrs = (Elf32_Phdr *) (text_segment_chunk + elf->e_phoff);
	uint32_t text_seg_offset = phdrs[0].p_offset;
	uint32_t slide = phdrs[0].p_vaddr;
	
	unsigned int module_info_offset = get_module_info_offset(text_segment_chunk);
	if (module_info_offset == 0) {
		printf("Could not get module info!\n");
		return -2;
	}
	SceModuleInfo_common* mod_info_common = malloc(sizeof(SceModuleInfo_common));
	fseek(fp, module_info_offset, SEEK_SET);
	fread(mod_info_common, sizeof(SceModuleInfo_common), 1, fp);
	if (strncmp("SceSysLibTrace", mod_info_common->modname, sizeof("SceSysLibTrace"))) {
		printf("This module is not SceSysLibTrace!\n");
		return -3;
	}
	// TODO: add support for other module info version
	if (mod_info_common->infover != 6) {
		printf("This module has an unsupported module info version!\n");
		return -4;
	}
	free(mod_info_common);
	SceModuleInfo_v6* mod_info = malloc(sizeof(SceModuleInfo_v6));
	fseek(fp, module_info_offset, SEEK_SET);
	fread(mod_info, sizeof(SceModuleInfo_v6), 1, fp);

	SceLibEntryTable_common* ent_common = malloc(sizeof(SceLibEntryTable_common));
	fseek(fp, text_seg_offset + mod_info->ent_top, SEEK_SET);
	fread(ent_common, sizeof(SceLibEntryTable_common), 1, fp);
	if (ent_common->size != 0x20) {
		printf("This module has an unsupported library entry size: %X\n", ent_common->size);
		return -5;
	}
	
	unsigned int exp_lib_num = (mod_info->ent_btm - mod_info->ent_top) / sizeof(SceLibEntryTable_20);
	SceLibEntryTable_20* ent_table = malloc(sizeof(SceLibEntryTable_20)*exp_lib_num);
	fseek(fp, text_seg_offset + mod_info->ent_top, SEEK_SET);
	fread(ent_table, sizeof(SceLibEntryTable_20)*exp_lib_num, 1, fp);
	
	SceLibEntryTable_20* exp_lib = NULL;
	for (int i = 0; i< exp_lib_num; ++i) {
		SceLibEntryTable_20* current = &ent_table[i];
		char* libname = malloc(MODULE_NAME_MAX_LEN);
		fseek(fp, text_seg_offset + current->libname - slide, SEEK_SET);
		fread(libname, MODULE_NAME_MAX_LEN, 1, fp);
		if (!strncmp("SceNidsymtblForDriver", libname, sizeof("SceNidsymtblForDriver"))) {
			exp_lib = current;
			break;
		}
	}
	if (exp_lib == NULL) {
		printf("This module does not have SceNidsymtblForDriver library!\n");
		return -6;
	}

	unsigned int* nid_table = malloc(sizeof(unsigned int)*exp_lib->c.nfunc);
	fseek(fp, text_seg_offset + exp_lib->nid_table - slide, SEEK_SET);
	fread(nid_table, sizeof(unsigned int)*exp_lib->c.nfunc, 1, fp);
	
	unsigned int* entry_table = malloc(sizeof(unsigned int)*exp_lib->c.nfunc);
	fseek(fp, text_seg_offset + exp_lib->entry_table - slide, SEEK_SET);
	fread(entry_table, sizeof(unsigned int)*exp_lib->c.nfunc, 1, fp);
	
	unsigned int sceNidsymtblGetTableForDriver_instr_offset = 0;
	for (int i = 0; i < exp_lib->c.nfunc; ++i) {
		if (nid_table[i] == 0x138A9106) {
			sceNidsymtblGetTableForDriver_instr_offset = entry_table[i] - slide - 1; // -1 because THUMB instruction mode
			break;
		}
	}
	if (sceNidsymtblGetTableForDriver_instr_offset == 0) {
		printf("This module does not have sceNidsymtblGetTableForDriver function!\n");
		return -7;
	}
	
	unsigned int* sceNidsymtblGetTableForDriver_instr = malloc(sizeof(unsigned int));
	fseek(fp, text_seg_offset + sceNidsymtblGetTableForDriver_instr_offset, SEEK_SET);
	fread(sceNidsymtblGetTableForDriver_instr, sizeof(unsigned int), 1, fp);
	uint32_t entries_start_offset = get_movw_value_from_instr(be32((u8*)sceNidsymtblGetTableForDriver_instr));

	ENTRY_TABLE* syslibtrace_entry_table;
	uint32_t i = 0;
	char *entry_buf = (char *) malloc(sizeof(ENTRY_TABLE));
	fseek(fp, text_seg_offset + entries_start_offset + sizeof(ENTRY_TABLE)*i, SEEK_SET);
	fread(entry_buf, sizeof(ENTRY_TABLE), 1, fp);
	syslibtrace_entry_table = (ENTRY_TABLE*)entry_buf;
	do {
		char *function_name = (char *) malloc(MAX_FUNCTION_NAME_BUFFER_SIZE);
		fseek(fp, text_seg_offset + syslibtrace_entry_table->function_name_offset - slide, SEEK_SET);
		fread(function_name, MAX_FUNCTION_NAME_BUFFER_SIZE, 1, fp);
		//printf("%s\n", function_name);
		
		if (extra_args)
			printf("%s,0x%08X,%s\n", function_name, syslibtrace_entry_table->function_nid, syslibtrace_entry_table->is_syscall ? "true" : "false");
		else
			printf("0x%08X,%s,%s\n", syslibtrace_entry_table->function_nid, function_name, syslibtrace_entry_table->is_syscall ? "true" : "false");
		free(function_name);
		
		i++;
		fseek(fp, text_seg_offset + entries_start_offset + sizeof(ENTRY_TABLE)*i, SEEK_SET);
		fread(entry_buf, sizeof(ENTRY_TABLE), 1, fp);
	} while (syslibtrace_entry_table->function_nid != 0xFFFFFFFF);
	fclose(fp);
	
	return 0;
}