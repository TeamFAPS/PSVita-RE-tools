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

#ifndef __UTILS_H__
#define __UTILS_H__

std::string getRegName(int handle, unsigned int reg_id);

void replaceAll(std::string& subject, const std::string& search, const std::string& replace);

bool isString(const std::string& s);

bool isHex(const std::string& s);
bool isDec(const std::string& s);

uint32_t strToHex(const std::string& s);
int strToDec(const std::string& s);

#endif