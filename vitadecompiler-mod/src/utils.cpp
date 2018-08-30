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
#include <map>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>

#include <capstone/capstone.h>

std::string getRegName(int handle, unsigned int reg_id)
{
	(void)handle;

	switch (reg_id) {
		case ARM_REG_R0: return "a1";
		case ARM_REG_R1: return "a2";
		case ARM_REG_R2: return "a3";
		case ARM_REG_R3: return "a4";
		
		case ARM_REG_R4: return "v1";
		case ARM_REG_R5: return "v2";
		case ARM_REG_R6: return "v3";
		case ARM_REG_R7: return "v4";
		case ARM_REG_R8: return "v5";
		case ARM_REG_R9: return "sb";
		case ARM_REG_R10: return "sl";
		case ARM_REG_R11: return "fp";

		case ARM_REG_R12: return "ip";
		case ARM_REG_R13: return "sp";
		case ARM_REG_R14: return "lr";
		case ARM_REG_R15: return "pc";
		
		default: return "unk";
	}
}

void replaceAll(std::string& subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

bool isString(const std::string& s) {
	return s[0] == '"' && s[s.size()-1] == '"';
}

bool isHex(const std::string& s)
{
	return s.size() > 2 && s.compare(0, 2, "0x") == 0 && s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
}

bool isDec(const std::string& s)
{
	return !s.empty() && s.find_first_not_of("-0123456789") == std::string::npos;
}

uint32_t strToHex(const std::string& s)
{
	return strtoul(s.c_str(), NULL, 16);
}

int strToDec(const std::string& s)
{
	return strtol(s.c_str(), NULL, 0x10);
}
