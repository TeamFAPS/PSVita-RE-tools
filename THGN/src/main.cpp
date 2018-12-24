//That Hookers Got NIDs
//Made by @dots_tb dedicated to the following frenchies: @CelesteBlue123 and @Nkekev
//Thanks:
//xerpi for his hooking code: https://pastebin.com/GeARzPqw
//theFlow for nid tables source stuff
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <zlib.h>

#include "import/vita-import.h"
#include "elf.h"
#include "self.h"

enum { Export, Import, All};
struct HookEntry {
	int nid;
	int lib_nid;
	int nid_type;
	char *lib_name;
	char *nid_name;
};


typedef std::vector<HookEntry*> Hooks;

static vita_imports_t *imports;

char *findNameByNid(const char *libname, uint32_t nid)
{
	for (int i = 0; i < imports->n_libs; i++) {
		vita_imports_lib_t *lib_entry = imports->libs[i];
		if (lib_entry == NULL)
			continue;

		for (int i = 0; i < lib_entry->n_modules; i++) {
			vita_imports_module_t *module_entry = lib_entry->modules[i];
			
			if (module_entry && strcmp(module_entry->name, libname) == 0) {
				for (int i = 0; i < module_entry->n_functions; i++) {
					vita_imports_stub_t *stub_entry = module_entry->functions[i];
					
					if (stub_entry && stub_entry->NID == nid)
						return stub_entry->name;
				}

				return NULL;
			}
		}
	}

	return NULL;
}


static void convertToImportsTable3xx(SceImportsTable2xx *import_2xx, SceImportsTable3xx *import_3xx)
{
	memset(import_3xx, 0, sizeof(SceImportsTable3xx));

	if (import_2xx->size == sizeof(SceImportsTable2xx)) {
		import_3xx->size = import_2xx->size;
		import_3xx->lib_version = import_2xx->lib_version;
		import_3xx->attribute = import_2xx->attribute;
		import_3xx->num_functions = import_2xx->num_functions;
		import_3xx->num_vars = import_2xx->num_vars;
		import_3xx->module_nid = import_2xx->module_nid;
		import_3xx->lib_name = import_2xx->lib_name;
		import_3xx->func_nid_table = import_2xx->func_nid_table;
		import_3xx->func_entry_table = import_2xx->func_entry_table;
		import_3xx->var_nid_table = import_2xx->var_nid_table;
		import_3xx->var_entry_table = import_2xx->var_entry_table;
	} else if (import_2xx->size == sizeof(SceImportsTable3xx)) {
		memcpy(import_3xx, import_2xx, sizeof(SceImportsTable3xx));
	}
}

int getExports(SceModuleInfo *mod_info, uint8_t *segment1, uint32_t vaddr, Hooks *nids, const char *lib) {
	int count = 0;
	uint32_t i = (uint32_t)mod_info->expTop;
	while(i  < mod_info->expBtm) {
		
		SceExportsTable *exp_table = (SceExportsTable *)(segment1 + i);
		char *lib_name = (char *)(segment1 + exp_table->lib_name - vaddr);
		uint32_t *nid_table = (uint32_t *)(segment1 + exp_table->nid_table - vaddr);		
		for (int j = 0; j < exp_table->num_functions; j++) {
			if(!exp_table->lib_name)
				continue;
			else if(lib && strcmp(lib_name,lib))
				continue;
			
			uint32_t nid = nid_table[j];
			HookEntry *hook = (HookEntry*)malloc(sizeof(HookEntry));
			hook->nid = nid;
			hook->lib_nid = exp_table->module_nid;
			hook->nid_type = Export;
			hook->lib_name = (char *)(segment1 + exp_table->lib_name - vaddr);
			char name[64];
				
			char *funcName = findNameByNid(lib_name, nid);
			if (funcName)
				snprintf(name, sizeof(name), "_%s", funcName);
			else
				snprintf(name, sizeof(name), "%s_%08X", lib_name, nid);
			hook->nid_name = (char*)malloc(sizeof(name));
			strcpy(hook->nid_name, name);
			nids->push_back(hook);
			count++;
		}
		 i += exp_table->size;
	 }
}

int getImports(SceModuleInfo *mod_info, uint8_t *segment1, uint32_t vaddr, Hooks *nids, const char *lib) {
	int count = 0;
	uint32_t i = (uint32_t)mod_info->impTop;
	while(i  < mod_info->impBtm) {
		SceImportsTable2xx *import_2xx = (SceImportsTable2xx *)(segment1 + i);
		SceImportsTable3xx imp_table;
		convertToImportsTable3xx(import_2xx, &imp_table);
		char *lib_name = (char *)(segment1 + (imp_table.lib_name - vaddr));
		uint32_t *nid_table = (uint32_t *)(segment1 + imp_table.func_nid_table - vaddr);		
		uint32_t imports_addr = imp_table.func_nid_table;
		for (int j = 0; j < imp_table.num_functions; j++) {	
			if(!imp_table.lib_name)
						continue;	
			if(lib && strcmp(lib_name,lib))
				continue;
			uint32_t nid = nid_table[j];

			HookEntry *hook = (HookEntry*)malloc(sizeof(HookEntry));
			hook->nid = nid;
			hook->lib_nid = imp_table.module_nid;
			hook->nid_type = Import;
			hook->lib_name = (char *)(segment1 + imp_table.lib_name - vaddr);
			char name[64];
			char *funcName = findNameByNid(lib_name, nid);
			if (funcName)
				snprintf(name, sizeof(name), "_%s", funcName);
			else
				snprintf(name, sizeof(name), "%s_%08X", lib_name, nid);
			hook->nid_name = (char*)malloc(sizeof(char) *strlen(name));
			strcpy(hook->nid_name, name);
			nids->push_back(hook);
			count++;
		}
		 i += imp_table.size;
	 }
	 
}

static uint8_t *handleSegments(int seg, uint8_t *input, SCE_header *shdr) {
	segment_info *sinfo = (segment_info *)(input + shdr->section_info_offset);
	uint8_t *destination = (uint8_t *)(input + sinfo[seg].offset);
	Elf32_Phdr *phdr = (Elf32_Phdr *)(input + shdr->phdr_offset);
	
	if(sinfo[seg].compression == 2) {
		size_t sz = phdr[seg].p_memsz ? phdr[seg].p_memsz : phdr[seg].p_filesz;
		destination = (uint8_t *)calloc(1,sinfo[seg].length);
		
		if (!destination) {
			perror("Error could not allocate memory.");
			return 0;
		}
			
		int ret = uncompress(destination, (long unsigned int*)&sinfo[seg].length, input + sinfo[seg].offset, sinfo[seg].length);
		
		if(ret != Z_OK) {
			fprintf(stderr, "Warning: could not uncompress segment %d, (No segment?): %d\n",seg, ret);
			//destination = input + sinfo[i].offset;
		}
	}
	return destination;
}


static void usage(char *argv[])
{
	printf("%s binary <all/library_name/exports/imports> <kernel/user> db.yml <sys:1/0>\n",argv[0]);
}

// Example: hooker mono-vita.suprx all

int main(int argc, char *argv[])
{
	printf("That Hookers got NIDS\n");
	printf("Made by @dots_tb dedicated to the following frenchies: @CelesteBlue123 and @Nkekev\n");
	
	FILE *fin = NULL;
	if (argc < 5) {
		usage(argv);
		return 1;
	}
	imports = vita_imports_load(argv[4], 0);
	
	char output_path[128];
	sprintf(output_path,"%s_hooker.c",argv[1]);
	fin = fopen(argv[1], "rb");
	
	if (!fin) {
		perror("Failed to open input file");
		goto error;
	}
	fseek(fin, 0, SEEK_END);
	size_t sz = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	uint8_t *input = (uint8_t *)calloc(1, sz);	
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
	ELF_header *ehdr = (ELF_header *)(input);
	uint8_t *segment1 = (uint8_t *)input;
	Elf32_Phdr *phdr;
	std::ofstream fout(output_path);
	if(!fout) {
		fprintf(stderr, "Error could not open output file.");
		goto error;
	}
	if(shdr->magic == 0x454353) {
		printf("SELF detected.\n");
		segment1 = handleSegments(0, input, shdr);
		ehdr = (ELF_header *)(input + shdr->header_len);
		phdr = (Elf32_Phdr *)(input + shdr->phdr_offset);
	} else {
		if(memcmp(input,"\177ELF\1\1\1",8)==0) {
			printf("ELF detected.");
			ehdr = (ELF_header *)(segment1);
			phdr = (Elf32_Phdr *)(segment1 + ehdr->e_phoff);
		} else  {
			fprintf(stderr, "File is not a binary");
			goto error;
		}
	}

		
	if(memcmp(segment1,"\177ELF\1\1\1",8)==0) {
		printf("ELF header found in segment 1, will use segment header.\n");
		ehdr = (ELF_header *)(segment1);
		phdr = (Elf32_Phdr *)(segment1 + ehdr->e_phoff);
		segment1 += phdr[0].p_offset;
	}
	
	SceModuleInfo *mod_info = (SceModuleInfo *)(segment1 + ehdr->e_entry);
	
	printf("Module name: %s\n", mod_info->name);
	
	Hooks table;
	int mode;
	std::string modeText;
	if(strcmp(argv[2],"exports") == 0) {
		getExports(mod_info, segment1, phdr[0].p_vaddr, &table, NULL);
		mode = Export;
		modeText = "Export";
	} else if(strcmp(argv[2],"imports") == 0) {
		getImports(mod_info, segment1, phdr[0].p_vaddr, &table, NULL);
		mode = Import;
		modeText = "Import";
	} else if(strcmp(argv[2],"all") == 0){
		getExports(mod_info, segment1, phdr[0].p_vaddr, &table, NULL);
		getImports(mod_info, segment1, phdr[0].p_vaddr, &table, NULL);
		mode = All;
		modeText = "Export";
	} else {
		printf("Hooking the library: %s\n",argv[2]);
		int ret = -1;
		if((ret = getExports(mod_info, segment1, phdr[0].p_vaddr, &table, argv[2]))>0) {
			mode = Export;
			modeText = "Export";
		}
		if(getImports(mod_info, segment1, phdr[0].p_vaddr, &table, argv[2])>0) {
			mode = Import;
			modeText = "Import";
			if(ret>0)
				mode = All;
				modeText = "Export";
		}
	}
	printf("This hooker got NIDS, generating code...\n");
	fout << "//Generated by That Hooker has NIDS" << std::endl;
	fout << "//Made by @dots_tb dedicated to the following frenchies: @CelesteBlue123 and @Nkekev" << std::endl;
	fout << "//Based of xerpi's code found here: https://pastebin.com/GeARzPqw" << std::endl;
	for (int i = 0; i < table.size(); i++) {
		fout << "//0x"  << std::hex << table.at(i)->nid << " " << table.at(i)->nid_name << std::endl;
	}
	fout << "static unsigned int nid_table[] = {" << std::endl;
	for (int i = 0; i < table.size() - 1; i++) {
		fout << "    0x"  << std::hex << table.at(i)->nid << "," << std::endl;
	}
	
	fout << "    0x"  << std::hex << table.back()->nid << "};" << std::endl;
	fout << 
	"#define NID_TABLE_SIZE (sizeof(nid_table) / sizeof(*nid_table))\n"
	"static unsigned int lib_table[] = {\n";
	for (int i = 0; i < table.size() - 1; i++) {
		fout << "    0x"  << std::hex << table.at(i)->lib_nid << "," << std::endl;
	}
	
	fout << "    0x"  << std::hex << table.back()->lib_nid << "};" << std::endl;
	fout << 
	"static SceUID hook_uids[NID_TABLE_SIZE] = {-1, -1};\n"
	"static tai_hook_ref_t hook_refs[NID_TABLE_SIZE];\n"
	"int do_shit(char *name, unsigned int nid, int r0, int r1, int r2, int r3, int r4, int r5, int r6, int r7, int r8, int r9, int r10, int r11, int r12) \n"
	"{ \n"
	"    int i = 0, ret, state; \n";
	if(argv[5]) fout <<
	"    ENTER_SYSCALL(state);\n";
	fout << 
	"    while (nid_table[i] != nid && i < NID_TABLE_SIZE) \n"
	"        i++; \n"
	"    printf(\"%s(0x%08X, 0x%08X, 0x%08X, 0x%08X)\\n\",name,r0,r1,r2,r3); \n"
	"    ret =  TAI_CONTINUE(int, hook_refs[i], r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12); \n"
	"    printf(\"%s(0x%08X, 0x%08X, 0x%08X, 0x%08X): 0x%08X\\n\",name,r0,r1,r2,r3, ret); \n";
	if(argv[5]) fout << 
	"    EXIT_SYSCALL(state);\n";
	fout << 
	"    return ret;\n"
	"}\n"
	"#define HOOK_FUNC(name, nid) \\\n"
	"    int name ## _hook(int r0, int r1, int r2, int r3, int r4, int r5, int r6, int r7, int r8, int r9, int r10, int r11, int r12) {\\\n"
	"        return do_shit(# name\"\", nid , r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12); \\\n"
	"    }\n";
	for (int i = 0; i < table.size(); i++) {
		fout << "HOOK_FUNC("<< table.at(i)->nid_name << ",0x"  << std::hex << table.at(i)->nid << ")" << std::endl;
	}
	fout <<
	"#define HOOK_FUNC_PTR(name) \\\n"
	"        &name ## _hook\n"
	"static void *func_ptr_table[] = {\n";
	for (int i = 0; i < table.size() - 1; i++) {
		fout << "    HOOK_FUNC_PTR("<< table.at(i)->nid_name << ")," << std::endl;
	}
	
	fout << "    HOOK_FUNC_PTR("<< table.back()->nid_name << ")};" << std::endl;
	fout <<
	"static void hooks_init() {\n"
	"    for (int i = 0; i < NID_TABLE_SIZE; i++)\n"
	"        hook_uids[i] = -1;\n"
	"    for (int i = 0; i < NID_TABLE_SIZE; i++) {\n";
	std::string taiFunction;
	std::string taiTrailer;
	if(strcmp(argv[3],"kernel")  == 0) {
		printf("Using kernel taihen hooks\n");
		taiTrailer = "ForKernel(KERNEL_PID,";
		taiFunction = "taiHookFunction" + modeText + taiTrailer;
	} else {
		printf("Using user taihen hooks\n");
		taiTrailer = "(";
		taiFunction = "taiHookFunction" + modeText + taiTrailer;
	}
	fout << "        hook_uids[i] = " << taiFunction << std::endl;
	fout << "                &hook_refs[i]," << std::endl;
	fout << "                \"" << mod_info->name << "\","	<< std::endl;	
	fout <<
	"                lib_table[i],\n"
	"                nid_table[i],\n"
	"                func_ptr_table[i]);\n"
	"        if (hook_uids[i] < 0) {\n"
	"            printf(\"Error hooking NID 0x%08X: 0x%08X\\n\",\n"
	"                nid_table[i], hook_uids[i]);\n";
	if(mode == All ) {
		fout << "        printf(\"Hooking imports...\");" << std::endl;
		fout << "        hook_uids[i] = taiHookFunctionImport" << taiTrailer << std::endl;
		fout << "                &hook_refs[i]," << std::endl;
		fout << "                \"" << mod_info->name << "\","	<< std::endl;	
		fout <<
		"                lib_table[i],\n"
		"                nid_table[i],\n"
		"                func_ptr_table[i]);\n"
		"            if (hook_uids[i] < 0) {\n"
		"                printf(\"Error hooking NID 0x%08X: 0x%08X\\n\",\n"
		"                    nid_table[i], hook_uids[i]);\n"
		"            }\n";
	}

	fout << 
	"        }\n"
	"    }\n"
	"}";
	fout.close();
	printf("Code was generated...: %s\n", output_path);
	return 0;
error:
	if (fin)
		fclose(fin);
	if(fout)
		fout.close();
	return 1;
	exit(0);

}
