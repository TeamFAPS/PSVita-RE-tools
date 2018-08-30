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

#ifndef __VITA_H__
#define __VITA_H__

typedef struct {
	uint16_t attr;
	uint16_t ver;
	uint8_t name[27];
	uint8_t type;
	uint32_t gp;
	uint32_t expTop;
	uint32_t expBtm;
	uint32_t impTop;
	uint32_t impBtm;
	uint32_t nid;
	uint32_t unk[3];
	uint32_t start;
	uint32_t stop;
	uint32_t exidxTop;
	uint32_t exidxBtm;
	uint32_t extabTop;
	uint32_t extabBtm;
} SceModuleInfo;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint32_t num_vars;
	uint32_t num_tls_vars;
	uint32_t module_nid;
	uint32_t lib_name;
	uint32_t nid_table;
	uint32_t entry_table;
} SceExportsTable;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t num_tls_vars;
	uint32_t reserved1;
	uint32_t module_nid;
	uint32_t lib_name;
	uint32_t reserved2;
	uint32_t func_nid_table;
	uint32_t func_entry_table;
	uint32_t var_nid_table;
	uint32_t var_entry_table;
	uint32_t tls_nid_table;
	uint32_t tls_entry_table;
} SceImportsTable2xx;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t unknown1;
	uint32_t module_nid;
	uint32_t lib_name;
	uint32_t func_nid_table;
	uint32_t func_entry_table;
	uint32_t var_nid_table;
	uint32_t var_entry_table;
} SceImportsTable3xx;

uint32_t analyseVitaModule(uint32_t offset, uint8_t *data, uint32_t text_addr, uint32_t text_size, std::string path, std::string opt_ver);

#endif
