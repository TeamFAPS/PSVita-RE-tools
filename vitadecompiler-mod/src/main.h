/*
	VitaTool
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

#ifndef __MAIN_H__
#define __MAIN_H__

enum SymbolType {
	SYMBOL_NONE = 0,
	SYMBOL_SUBROUTINE = 0x1,
	SYMBOL_LABEL = 0x2,
	SYMBOL_STRING = 0x4,
	SYMBOL_EXPORT = 0x8,
	SYMBOL_IMPORT = 0x10,
};

void addSymbol(uint32_t addr, std::string name, int type);

char *findNameByNid(const char *libname, uint32_t nid);
#endif