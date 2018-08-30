/*
	VitaDecompiler
	Copyright (C) 2017, TheFloW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <iomanip>

#include "main.h"
#include "vita.h"

struct SyslibEntry {
	uint32_t nid;
	const char *name;
};

static SyslibEntry syslib_nids[] = {
	{ 0x70FBA1E7, "module_process_param" },
	{ 0x6C2224BA, "module_info" },
	{ 0x935CD196, "module_start" },
	{ 0x79F8E492, "module_stop" },
	{ 0x913482A9, "module_exit" },
};

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

uint32_t analyseVitaModule(uint32_t offset, uint8_t *data, uint32_t text_addr, uint32_t text_size, std::string path, std::string opt_ver)
{
	
	uint32_t mod_info_offset = 0;

/*
	uint32_t addr = 0;
	
	while (addr < text_size - 0x10) {
		if (*(uint32_t *)(data + addr) == 0x01010000 ||
			*(uint32_t *)(data + addr) == 0x01010007) {
			mod_info_offset = addr;
			break;
		}

		addr += 4;
	}

	if (mod_info_offset == 0)
		return 0;*/
	mod_info_offset  = offset;


	SceModuleInfo *mod_info = (SceModuleInfo *)(data + mod_info_offset);

	std::cerr << "Module name: " << mod_info->name << std::endl;

	uint32_t i = 0;
	
int x = 0;
	i = mod_info->expTop;
	std::ofstream fnids(path + ".nids.txt");
	std::ofstream fyml(std::string((char*)mod_info->name) + ".yml");
	std::cerr << "Exporting NIDS file to: " << path << ".nids.txt" << std::endl;
	std::cerr << "Exporting db_lookup file to: " << mod_info->name << ".yml" << std::endl;
	fnids << "//EXPORTS: " << std::endl;
	fyml <<//SHITTY DB_LOOKUP CREATION :WEARY: included
	"version: 0x2\n"
	"firmware: " << opt_ver <<"\n"
	"modules:\n"
	"  " << mod_info->name << ":" << std::endl
	<< "    nid: 0x"  << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << mod_info->nid << std::endl
	<< "    libraries:" << std::endl;
	while (i < mod_info->expBtm) {
		SceExportsTable *exp = (SceExportsTable *)(data + i);

		char *lib_name = (char *)((uintptr_t)data + (exp->lib_name - text_addr));
		uint32_t *nid_table = (uint32_t *)((uintptr_t)data + (exp->nid_table - text_addr));
		uint32_t *entry_table = (uint32_t *)((uintptr_t)data + (exp->entry_table - text_addr));

		int j;
		if(exp->lib_name) {
			fnids << std::endl << "//" << lib_name << ":" << exp->module_nid << std::endl ;
			fyml << "      " << lib_name << ":" << std::endl;
			fyml << "        nid: 0x" << std::setfill('0') << std::setw(8) << exp->module_nid << std::endl;
			if(exp->attribute&0x4000) 
				fyml << "        kernel: false" << std::endl;
			else
				fyml << "        kernel: true" << std::endl;
			fyml << "        functions:" << std::endl;
		}
		for (j = 0; j < exp->num_functions + exp->num_vars; j++) {
			uint32_t nid = nid_table[j];
			uint32_t addr = entry_table[j];

			char name[64];
			
			if (exp->lib_name) {
				char *funcName = findNameByNid(lib_name, nid);
				if (funcName)
					snprintf(name, sizeof(name), funcName);
				else
					snprintf(name, sizeof(name), "%s_%08X", lib_name, nid);
			} else {
				for (uint32_t i = 0; i < (sizeof(syslib_nids) / sizeof(SyslibEntry)); i++) {
					if (syslib_nids[i].nid == nid)
						snprintf(name, sizeof(name), "%s", syslib_nids[i].name);
				}
			}
			if(exp->lib_name) {
				fyml << "          " << lib_name << "_" << std::setfill('0') << std::setw(8) << nid << ": 0x"  << std::setfill('0') << std::setw(8) << nid  << std::endl;
			}
			fnids <<  "//" << std::dec  << x++ << std::hex <<  std::showbase <<  ": " << name << " " << nid << " @OFF: " << addr - text_addr << " VADDR: "<< addr << std::endl;

			if(addr < text_addr){
				fnids << "///Something is wrong with NID " << name << std::endl;
			}
			addSymbol(addr & ~0x1, name, SYMBOL_SUBROUTINE | SYMBOL_EXPORT);
		}

		i += exp->size;
	}

	i = mod_info->impTop;
		fnids << "//IMPORTS: " << std::endl;
x = 0;
	while (i < mod_info->impBtm) {
		SceImportsTable3xx imp;
		convertToImportsTable3xx((SceImportsTable2xx *)(data + i), &imp);

		if (imp.lib_name) {
			char *lib_name = (char *)((uintptr_t)data + (imp.lib_name - text_addr));
			uint32_t *nid_table = (uint32_t *)((uintptr_t)data + (imp.func_nid_table - text_addr));
			uint32_t *entry_table = (uint32_t *)((uintptr_t)data + (imp.func_entry_table - text_addr));
			if(lib_name)
			fnids << std::endl << "//" << lib_name << ":" << imp.module_nid << std::endl;

			int j;
			for (j = 0; j < imp.num_functions; j++) {
				uint32_t nid = nid_table[j];
				uint32_t addr = entry_table[j];
				
				char name[64];
				
				char *funcName = findNameByNid(lib_name, nid);
				if (funcName)
					snprintf(name, sizeof(name), funcName);
				else
					snprintf(name, sizeof(name), "%s_%08X", lib_name, nid);
			fnids << "//" <<  std::dec  << x++ << ": " << name << " " << std::hex <<  std::showbase << nid<< " @OFF: " << addr - text_addr << " VADDR: "<< addr << std::endl;
				
				addSymbol(addr, name, SYMBOL_SUBROUTINE | SYMBOL_IMPORT);
				addSymbol((addr +0x4 ), name, SYMBOL_SUBROUTINE | SYMBOL_IMPORT);
			}
		}

		i += imp.size;
	}
	fnids.close();
	fyml.close();
	return mod_info_offset;
}
