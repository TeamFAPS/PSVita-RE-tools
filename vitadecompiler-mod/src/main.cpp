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
	
	ALSO:
		Vitadecompiler mod by @dots_tb, @CelesteBlue123
	
*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <zlib.h>
#include <fstream>

#include <capstone/capstone.h>

#include "main.h"
#include "vita.h"
#include "utils.h"
#include "elf.h"
#include "self.h"
#include "instructions.h"
#include "relocate.h"

#include "import/vita-import.h"

#define INDENT "\t"
#define MAX_ARGS 11

// TODO: compose conditionals
// TODO: write evaluate functions to simplify code
// TODO: output idc
// TODO: aliases
// TODO: input data section <

/*
	Ideas:
	- Save labels in a list for every subroutine
	- Labels hold information about what registers they depend on
*/

struct SymbolEntry {
	int type;
	std::string name;
	int n_args;
	int args_stats[MAX_ARGS];
};

struct RegisterAssignmentEntry {
	std::string assign;
	uint32_t addr;
	bool update_flags;
};

typedef std::map<uint32_t, SymbolEntry *> SymbolMap;

typedef std::map<uint32_t, uint32_t> MovwMap;
typedef std::map<uint32_t, bool> IgnoreMap;

typedef std::map<int, RegisterAssignmentEntry *> RegisterAssignmentMap;

static SymbolMap g_symbol_map;

static MovwMap g_movw_map;
static IgnoreMap g_ignore_map;

static RegisterAssignmentMap g_reg_assign_map;

static bool g_use_flags = false;

static std::string g_condition_reg_1;
static std::string g_condition_reg_2;

static csh handle;

static uint32_t g_text_addr = 0, g_text_size = 0, g_data_size = 0, g_mod_info_offset = 0, g_data_addr = 0;
static uint8_t *g_text_seg = 0;
static uint8_t *g_data_seg = 0;

static std::ofstream fout;
static std::string path;

static uint32_t *isascii4(uintptr_t *text) {
	char *string = (char *)(text);
				
	int i = 0, len = -1;
	if((text >(uintptr_t *)g_data_seg &&  text + 4 < (uintptr_t *)g_data_seg + g_data_size)
		||(text>(uintptr_t *)g_text_seg && text + 4< (uintptr_t *)g_text_seg + g_text_size))
		len = strnlen(string,4);
	else
		return NULL;
	while (string[i] >= 0x20) {
		i++;
	}
	if ((i>0&&i >= len&&len!=0)||(text>(uintptr_t *)g_text_seg && text< (uintptr_t *)g_text_seg + g_text_size&&i==0&&string[i] ==0))
		return (uint32_t*)text;
	return NULL;
}
int ReadFile(const char *file, uint8_t *buf, size_t size)
{
	FILE *f = fopen(file, "rb");

	if (!f)
		return -1;

	int rd = fread(buf, 1, size, f);
	fclose(f);

	return rd;
}

static bool isConditional(cs_arm *arm) {
	if (arm == NULL)
		return false;

	return arm->cc != ARM_CC_AL && arm->cc != ARM_CC_INVALID;
}

static void freeRegAssignMap() {
	for (auto it = g_reg_assign_map.begin(); it != g_reg_assign_map.end(); it++) {
		delete it->second;
	}

	g_reg_assign_map.clear();
}

static void clearRegAssignMapAfterBranch() {
	delete g_reg_assign_map[ARM_REG_R0];
	delete g_reg_assign_map[ARM_REG_R1];
	delete g_reg_assign_map[ARM_REG_R2];
	delete g_reg_assign_map[ARM_REG_R3];

	g_reg_assign_map[ARM_REG_R0] = NULL;
	g_reg_assign_map[ARM_REG_R1] = NULL;
	g_reg_assign_map[ARM_REG_R2] = NULL;
	g_reg_assign_map[ARM_REG_R3] = NULL;
}

static std::string getImmString(uint32_t imm, bool is_mem)
{
	char buf[64];

	if (((int)imm >= -9 && (int)imm < 0) || ((int)imm >= 0 && (int)imm <= 9) ||
		(is_mem && (imm & 0x80000000))) {
		snprintf(buf, sizeof(buf), "%d", imm);
	} else {
		snprintf(buf, sizeof(buf), "0x%X", imm);
	}

	return buf;
}

static std::string getInstructionCode(cs_arm *arm, const std::string& mnemonic)
{
	// Is known instruction?
	if (instructions.count(mnemonic) == 0)
		return "";

	std::string code(instructions[mnemonic]);

	// Instructions with 2 op's only
	if (arm->op_count == 2) {
		if (instructions2.count(mnemonic) > 0) {
			code = instructions2[mnemonic];
		}
	}

	// Update flags
	if (arm->update_flags) {
		if (mnemonic.compare("cmp") != 0 &&
			mnemonic.compare("cmn") != 0 &&
			mnemonic.compare("tst") != 0) {
			code += "\\FLAGS = op0;";
		}
	}

	return code;
}

static std::string trimMnemonic(cs_arm *arm, std::string mnemonic, std::string* cond_flags, std::string *cond_regs)
{
	// Remove '.w' suffix
	mnemonic = mnemonic.substr(0, mnemonic.find(".w"));

	// Remove condition suffix
	if (isConditional(arm)) {
		std::string cond = mnemonic.substr(mnemonic.size()-2, 2);
	    if (conditions.count(cond) > 0) {
			if (cond_flags != NULL)
				*cond_flags = conditions[cond].cond_flags;
			
			if (cond_regs != NULL)
				*cond_regs = conditions[cond].cond_regs;
			
			mnemonic = mnemonic.substr(0, mnemonic.size()-2);
		}
	}

	// Remove 's' suffix
	if (arm->update_flags) {
		if (mnemonic[mnemonic.size()-1] == 's')
			mnemonic = mnemonic.substr(0, mnemonic.size()-1);
	}

	return mnemonic;
}

static void ignoreAddr(int reg) {
/*	// Do not ignore other registers than r0-r3
	if (!(reg >= ARM_REG_R0 && reg < ARM_REG_R4))
		return;
*/
	if (g_reg_assign_map[reg] != NULL) {
		if (g_reg_assign_map[reg]->update_flags && g_use_flags)
			return;

		g_ignore_map[g_reg_assign_map[reg]->addr] = true;
	}
}

static std::string getArgsString(int n_args, bool input, bool analyse)
{
	std::stringstream output;

	for (int i = 0; i < ((n_args < MAX_ARGS) ? n_args : MAX_ARGS); i++) {
		if (input) {
			if (i < 4) {
				if (g_reg_assign_map[ARM_REG_R0+i] != NULL) {
					output << g_reg_assign_map[ARM_REG_R0+i]->assign << ", ";
					
					if (analyse)
						ignoreAddr(ARM_REG_R0+i);
				} else {
					output << "a" << (i+1) << ", ";
				}
			} else {
				output << "*(sp+" << ((i-4)*4) << "), ";
			}
		} else {
			output << "int arg" << (i+1) << ", ";
		}
	}

	std::string out_str = output.str();
	return out_str.substr(0, out_str.size()-2);
}

static void handleShift(std::string& reg, cs_arm_op *op)
{
	if (op->shift.type != ARM_SFT_INVALID && op->shift.value) {
		std::string shift_op = shifts[op->shift.type];

		char buf[256];

		if (op->shift.type < ARM_SFT_ASR_REG) {
			snprintf(buf, sizeof(buf), "(%s %s 0x%X)", reg.c_str(), shift_op.c_str(), op->shift.value);
		} else {
			snprintf(buf, sizeof(buf), "(%s %s %s)", reg.c_str(), shift_op.c_str(), getRegName(handle, op->shift.value).c_str());
		}

		reg = buf;
	}
}
static uintptr_t *checkDwordValue(uint32_t *value) {
	uintptr_t *currentPtr = (uintptr_t *)value;
	uint32_t currentAddr = (uint32_t)*currentPtr;
	if(currentAddr == 0)
		return currentPtr;
	while(currentAddr > g_text_addr && currentAddr < g_text_addr + g_text_size) {
		currentPtr = (uintptr_t *)((uintptr_t)g_text_seg + currentAddr - g_text_addr);
		currentAddr = (uint32_t)*currentPtr;
	}
	while(currentAddr > g_data_addr && currentAddr < g_data_addr + g_data_size) {
		currentPtr = (uintptr_t *)((uintptr_t)g_data_seg + currentAddr - g_data_addr);
		currentAddr = (uint32_t)*currentPtr;
	}		
	return currentPtr;
}
static void assignRegister(cs_insn *insn, const std::string& mnemonic, uint32_t addr, std::string& code, bool analyse) {
	cs_arm *arm = &(insn->detail->arm);

	cs_arm_op *op0 = &(arm->operands[0]);

	// Is conditional instruction
	if (isConditional(arm)) {
		// Do not compose it
		delete g_reg_assign_map[op0->mem.base];
		g_reg_assign_map[op0->mem.base] = NULL;
	} else {
		std::string assign = code.substr(5, code.find(";")-5);
		
		if (mnemonic.compare("movt") == 0) {
			if (g_reg_assign_map[op0->mem.base] != NULL) {
				if (isHex(g_reg_assign_map[op0->mem.base]->assign) ||
					isDec(g_reg_assign_map[op0->mem.base]->assign)) {
					uint32_t movt_val = strToHex(assign.substr(0, assign.find(" ")));
					uint32_t movw_val = strToHex(g_reg_assign_map[op0->mem.base]->assign);
					
					uint32_t value = movt_val | movw_val;
					
					char buf[1024];
					snprintf(buf, sizeof(buf), "%08X", value, value);
					uintptr_t *dword_addr;
					uint32_t *ret =NULL;
					if (value > g_text_addr && value < g_text_addr+g_text_size) {
						if(!(ret=isascii4((uintptr_t *)(g_text_seg+value-g_text_addr)))) {
							dword_addr = checkDwordValue(&value);
							ret=isascii4(dword_addr);
						}
						if (!ret) {
							value &= ~0x1;
							if (g_symbol_map[value] != NULL) {
								if(g_symbol_map[value]->type & SYMBOL_SUBROUTINE) {
									snprintf(buf, sizeof(buf), (g_symbol_map[value]->name).c_str());
								//} else if (g_symbol_map[value]->type & SYMBOL_STRING){
								//	snprintf(buf, sizeof(buf), "/*text_%08X*/ 0x%08X", value, *dword_addr);
								} else 
									snprintf(buf, sizeof(buf), "/*text_%08X*/ 0x%08X", value, *dword_addr);
							}
						} else {
							snprintf(buf, sizeof(buf), "/*s_text_%08X*/ \"%s\"", value, ret);
						}
					} else if(value > g_data_addr && value < g_data_addr+g_data_size) {
							if(!(ret=isascii4((uintptr_t *)(g_data_seg+value-g_data_addr)))) {
								dword_addr = checkDwordValue(&value);
								ret=isascii4(dword_addr);
							}
							if(ret)
								snprintf(buf, sizeof(buf), "/*s_data_%08X*/ \"%s\"", value, ret);
							else
								snprintf(buf, sizeof(buf), "/*data_%08X*/ 0x%08X", value, *dword_addr);
	
					} else {
						snprintf(buf, sizeof(buf), "/*data_%08X*/", value);
	
					}
					
					std::string newline_esc(buf);
					replaceAll(newline_esc, "\n", "()n");
					strcpy(buf,newline_esc.c_str());
					code = getRegName(handle, op0->mem.base) + " = " + buf + ";";
					
					assign = buf;
					
					if (analyse)
						ignoreAddr(op0->mem.base);
				}
			}
		}
		
		// Register assignment
		if (g_reg_assign_map[op0->mem.base] == NULL)
			g_reg_assign_map[op0->mem.base] = new RegisterAssignmentEntry();
		
		RegisterAssignmentEntry *entry = g_reg_assign_map[op0->mem.base];
		entry->assign = assign;
		entry->addr = addr;
		entry->update_flags = arm->update_flags;
	}
}

static void composeRegister(std::string& reg_str, int reg, bool analyse)
{
	// Do not compose sp reg
	if (reg == ARM_REG_R13)
		return;

	// Register is in memory, can be composed
	if (g_reg_assign_map[reg] != NULL && g_reg_assign_map[reg]->assign.size() < 128) {
		std::string assign(g_reg_assign_map[reg]->assign);
		
		bool is_string = isString(assign);
		bool is_atom = assign.find(" ") == std::string::npos;

		bool set_brackets = !is_string && !is_atom;
		
		// Open brackets if needed
		if (set_brackets)
			reg_str = "(";
		else
			reg_str = "";
		
		// Composing
		reg_str += assign;
		
		// Close brackets if needed
		if (set_brackets)
			reg_str += ")";
		
		if (analyse)
			ignoreAddr(reg);
	}
}

static void parseOperations(cs_insn *insn, const std::string& mnemonic, uint32_t addr, std::string& code, bool analyse) {
	cs_arm *arm = &(insn->detail->arm);

	bool is_cmp = mnemonic.compare("cmp") == 0;

	for (int i = 0; i < arm->op_count; i++) {
		std::stringstream op_name;
		op_name << "op" << i;

		cs_arm_op *op = &(arm->operands[i]);
		switch((int)op->type) {
			case ARM_OP_REG:
			{
				std::string reg = getRegName(handle, op->mem.base);
				
				// Composing
				if (code.find(op_name.str() + " = ") == std::string::npos) {
					composeRegister(reg, op->mem.base, analyse);
				}
				
				// Handle shift
				if (i == arm->op_count-1) {
					handleShift(reg, op);
				}
				
				// Is comparison. TODO
				if (is_cmp) {
					if (i == 0) {
						g_condition_reg_1 = reg;
					} else if (i == 1) {
						g_condition_reg_2 = reg;
					}
					/*
					if (analyse)
						g_ignore_map[addr] = true;
					*/
				}
				
				// Replace operand
				replaceAll(code, op_name.str(), reg);
				
				break;
			}
			
			case ARM_OP_IMM:
			{
				// Replace operand
				replaceAll(code, op_name.str(), getImmString(op->imm, false));
				break;
			}
			
			case ARM_OP_MEM:
			{
				std::string str;
				std::string reg = getRegName(handle, op->mem.base);
				
				// Composing
				if (code.find(op_name.str() + " = ") == std::string::npos) {
					composeRegister(reg, op->mem.base, analyse);
				}
				
				if (op->mem.index != ARM_REG_INVALID) {
					std::string reg2 = getRegName(handle, op->mem.index);
					handleShift(reg2, &(arm->operands[0]));
					str = reg + " + " + reg2;
				} else {
					str = reg + " + " + getImmString(op->mem.disp, true);
				}
				
				// Simplify. TODO: replace this by evaluator
				size_t pos = str.find(" + 0", str.size()-4);
				if (pos != std::string::npos)
					str = str.substr(0, pos);
				
				// Replace operand
				replaceAll(code, op_name.str(), str);
				
				break;
			}
		}
	}
}

static void translateCode(cs_insn *insn, const std::string& mnemonic, uint32_t addr, std::string& code, bool analyse)
{
	cs_arm *arm = &(insn->detail->arm);

	// A conditional is dependant on the flags
	if (isConditional(arm))
		g_use_flags = true;

	// It has been updated, set to false
	if (arm->update_flags) {
		g_use_flags = false;
		
		// Reset condition regs too
		g_condition_reg_1 = "";
		g_condition_reg_2 = "";
	}

	// Parse operations
	parseOperations(insn, mnemonic, addr, code, analyse);

	// Register assignment
	if (code[3] == '=' && code.compare(5, 6, "symbol") != 0) {
		assignRegister(insn, mnemonic, addr, code, analyse);
	}
	
	// Replace symbol
	for (int i = 0; i < 2; i++) {
		cs_arm_op *op = &(arm->operands[i]);
		if (g_symbol_map[op->imm] != NULL) {
			if (g_symbol_map[op->imm]->type & (SYMBOL_SUBROUTINE | SYMBOL_LABEL)) {
				replaceAll(code, "symbol", g_symbol_map[op->imm]->name);
				
				if (g_symbol_map[op->imm]->type & SYMBOL_SUBROUTINE)
					replaceAll(code, "...", getArgsString(g_symbol_map[op->imm]->n_args, true, analyse));
			}
		} else {
			if (i == 1)
				replaceAll(code, "symbol", insn->op_str);
		}
	}
	
	// Clear args map after branch
	if (mnemonic.compare("bl") == 0 || mnemonic.compare("blx") == 0) {
		clearRegAssignMapAfterBranch();
		
		// Flags are no longer up-to-date, set to false
		g_use_flags = false;
	}
}

static void pseudoCode(cs_insn *insn, uint32_t addr)
{
	std::string indent = INDENT;
	std::string cond_flags = "";
	std::string cond_regs = "";

	cs_arm *arm = &(insn->detail->arm);

	std::string mnemonic = trimMnemonic(arm, insn->mnemonic, &cond_flags, &cond_regs);

	// Get instruction code
	std::string code = getInstructionCode(arm, mnemonic);
	if (code.empty()) {
		std::string op_str(insn->op_str);
		if (mnemonic.compare("bx") == 0 && op_str.compare("lr") == 0)
			fout << indent << "return a1;" << std::endl;
		else if (mnemonic.compare("pop") == 0)
			fout << indent << "return a1; // " << mnemonic << " " << insn->op_str << std::endl;
		else if (mnemonic.compare("push") == 0)
			fout << indent << "// " << mnemonic << " " << insn->op_str << std::endl;
		else if (mnemonic.compare("nop") == 0 || mnemonic.compare(0, 2, "it") == 0)
			fout << ""; // Nothing
		else
			fout << indent << "asm(\"" << mnemonic << " " << insn->op_str << "\\n\");" << std::endl;
		
		return;
	}

	// Translate code
	translateCode(insn, mnemonic, addr, code, false);

	// Condition
	if (cond_flags != "" || cond_regs != "") {
		// TODO: compose conditions
		/*if (g_condition_reg_1 != "" && g_condition_reg_2 != "") {
			replaceAll(cond_regs, "COND_X", g_condition_reg_1);
			replaceAll(cond_regs, "COND_Y", g_condition_reg_2);

			std::cout << indent << cond_regs << "" << std::endl;
		} else {*/
			fout << indent << cond_flags << "" << std::endl;
		//}
		
		indent += "\t";
	}

	// Replace '\' with newline
	replaceAll(code, "\\", "\n" + indent);
	replaceAll(code, "()n", "\\n");
	if (g_ignore_map[addr]) return;

	// Output
	fout << indent;

	if (g_ignore_map[addr]) {
		fout << "// ";
		replaceAll(code, INDENT, INDENT "// ");
	}

	fout << code << std::endl;
}

static int decompile()
{
	cs_err err = cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &handle);
	if (err) {
		return -1;
	}

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	cs_insn *insn = cs_malloc(handle);

	bool first_import = true, first = true;

	const uint8_t *code = g_text_seg;
	uint32_t size = g_mod_info_offset;
	uint32_t address = g_text_addr;
	
	std::ofstream fheader(path + ".h");
	fheader << std::hex <<  std::showbase;
	if(!fheader) {
		std::cerr << "Error creating header..." << std::endl;
		return 0;
	}
	std::cerr << "Exporting header file to: " << path << ".h" << std::endl;
	while (size > 0) {
		// Reset at new subroutine/label
		if (g_symbol_map[address] != NULL) {
			freeRegAssignMap();
			g_use_flags = false;
		}

		// Import
		if (g_symbol_map[address] != NULL) {
			if (g_symbol_map[address]->type & SYMBOL_IMPORT) {
				if (first_import) {
					fout << "}\n\n";
					first_import = false;
				}
				
				fout << "int " << g_symbol_map[address]->name << "(" << getArgsString(g_symbol_map[address]->n_args, false, false) << ");" << std::endl;
				fheader << "int " << g_symbol_map[address]->name << "(" << getArgsString(g_symbol_map[address]->n_args, false, false) << ");" << std::endl;
				
				address += 0x10;
				code += 0x10;
				size -= 0x10;
				continue;
			}
		}

		// Subroutine/label start
		if (g_symbol_map[address] != NULL) {

			if (g_symbol_map[address]->type & SYMBOL_SUBROUTINE) {
				if (first) {
					first = false;
				} else {
					fout << "}\n\n";
				}
				fout << "//VADDR: " << address << " OFF: " << address - g_text_addr << std::endl;
				if (g_symbol_map[address]->type & SYMBOL_IMPORT) {
					fout << "// Imported\n";
				} else if (g_symbol_map[address]->type & SYMBOL_EXPORT) {
					fout <<  "// Exported\n";
				}

				fheader << "int " << g_symbol_map[address]->name << "(" << getArgsString(g_symbol_map[address]->n_args, false, false) << ");" << std::endl;
				fout << "int " << g_symbol_map[address]->name << "(" << getArgsString(g_symbol_map[address]->n_args, false, false) << ")" << std::endl << "{" << std::endl;
			} else if (g_symbol_map[address]->type & SYMBOL_LABEL) {
				fout << std::endl << g_symbol_map[address]->name << ":" << std::endl;
				fout << "//VADDR: " << address << " OFF: " << address - g_text_addr << std::endl;
			}
		}

		// Disassemble
		uint32_t addr = address;

		int res = cs_disasm_iter(handle, &code, (size_t *)&size, (uint64_t *)&address, insn);
		if (res == 0) {
			uint32_t opcode;
			//memcpy(&opcode, code + address-g_text_addr, sizeof(uint32_t));
			//fout << "\tasm(\".word " << opcode << "\\n\");\n";
			fout << "\tasm(\"" << insn->mnemonic << " " << insn->op_str << "\\n\");\n";
			address += 2;
			code += 2;
			size -= 2;

			continue;
		}

		// Output pseudo code
		pseudoCode(insn, addr);
	}

	cs_free(insn, 1);

	cs_close(&handle);

	fheader.close();
	return 0;
}

static int analyseCode() {
	cs_err err = cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &handle);
	if (err) {
		return -1;
	}

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	cs_insn *insn = cs_malloc(handle);

	const uint8_t *code = g_text_seg;
	uint32_t size = g_mod_info_offset;
	uint32_t address = g_text_addr;

	while (size > 0) {
		// Reset at new subroutine/label
		if (g_symbol_map[address] != NULL) {
			freeRegAssignMap();
			g_use_flags = false;
		}

		// Disassemble
		uint32_t addr = address;

		int res = cs_disasm_iter(handle, &code, (size_t *)&size, (uint64_t *)&address, insn);
		if (res == 0) {
			address += 2;
			code += 2;
			size -= 2;
			continue;
		}

		cs_arm *arm = &(insn->detail->arm);

		std::string mnemonic = trimMnemonic(arm, insn->mnemonic, NULL, NULL);

		// Get instruction code
		std::string code = getInstructionCode(arm, mnemonic);
		if (code.empty())
			continue;

		// Translate code
		translateCode(insn, mnemonic, addr, code, true);
	}

	cs_free(insn, 1);

	cs_close(&handle);

	return 0;
}

static int analyseArguments() {
	int n_args = 0;

	cs_err err = cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &handle);
	if (err) {
		return -1;
	}

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	cs_insn *insn = cs_malloc(handle);

	const uint8_t *code = g_text_seg;
	uint32_t size = g_mod_info_offset;
	uint32_t address = g_text_addr;

	while (size > 0) {
		// Disassemble
		uint32_t addr = address;

		int res = cs_disasm_iter(handle, &code, (size_t *)&size, (uint64_t *)&address, insn);
		if (res == 0) {
			address += 2;
			code += 2;
			size -= 2;
			continue;
		}

		// Reset at new subroutine/label
		if (g_symbol_map[addr] != NULL) {
			n_args = 0;
		}

		cs_arm *arm = &(insn->detail->arm);

		std::string mnemonic = trimMnemonic(arm, insn->mnemonic, NULL, NULL);

		// Get instruction code
		std::string code = getInstructionCode(arm, mnemonic);
		if (code.empty())
			continue;

		if (mnemonic.compare("str") == 0) {
			cs_arm_op *op = &(arm->operands[1]);

			if (op->mem.base == ARM_REG_R13) {
				if (op->mem.disp >= 0 && op->mem.disp < 0xC) {
					int n_stack_args = (op->mem.disp >> 2) + 1;
					if (n_args <= 4+n_stack_args)
						n_args = 4+n_stack_args;
				}
			}
		} else if (mnemonic.compare("strd") == 0) {
			cs_arm_op *op = &(arm->operands[2]);

			if (op->mem.base == ARM_REG_R13) {
				if (op->mem.disp >= 0 && op->mem.disp < 0xC) {
					int n_stack_args = (op->mem.disp >> 2) + 1;
					if (n_args <= 4+n_stack_args)
						n_args = 4+n_stack_args;
				}
			}
		} else {
			if (code.find("op0 = ") != std::string::npos) {
				cs_arm_op *op = &(arm->operands[0]);
				
				if (op->mem.base >= ARM_REG_R0 && op->mem.base < ARM_REG_R4) {
					if (op->mem.base == ARM_REG_R0 && n_args <= 0)
						n_args = 1;
					else if (op->mem.base == ARM_REG_R1 && n_args <= 1)
						n_args = 2;
					else if (op->mem.base == ARM_REG_R2 && n_args <= 2)
						n_args = 3;
					else if (op->mem.base == ARM_REG_R3 && n_args <= 3)
						n_args = 4;
				}
				
				if (mnemonic.compare("ldrd") == 0) {
					cs_arm_op *op = &(arm->operands[1]);
					
					if (op->mem.base >= ARM_REG_R0 && op->mem.base < ARM_REG_R4) {
						if (op->mem.base == ARM_REG_R0 && n_args <= 0)
							n_args = 1;
						else if (op->mem.base == ARM_REG_R1 && n_args <= 1)
							n_args = 2;
						else if (op->mem.base == ARM_REG_R2 && n_args <= 2)
							n_args = 3;
						else if (op->mem.base == ARM_REG_R3 && n_args <= 3)
							n_args = 4;
					}
				}
			}
		}

		// Function call
		if (mnemonic.compare("bl") == 0 || mnemonic.compare("blx") == 0) {
			cs_arm_op *op = &(arm->operands[0]);

			if ((uint32_t)op->imm >= g_text_addr && (uint32_t)op->imm < (g_text_addr+g_text_size)) {
				if (g_symbol_map[op->imm]) {					
					if (n_args < MAX_ARGS)
						g_symbol_map[op->imm]->args_stats[n_args]++;
				}
			}

			n_args = 0;
		}
	}

	for (auto it = g_symbol_map.begin(); it != g_symbol_map.end(); it++) {
		SymbolEntry *entry = it->second;
		
		if (entry != NULL && entry->type & SYMBOL_SUBROUTINE) {
			int reg = 0, max = 0;
			
			for (int i = 0; i < MAX_ARGS; i++) {
				if (entry->args_stats[i] > max) { // TODO: or >=
					max = entry->args_stats[i];
					reg = i;
				}
			}
			
			entry->n_args = reg;
		}
	}

	cs_free(insn, 1);

	cs_close(&handle);

	return 0;
}

void addSymbol(uint32_t addr, std::string name, int type)
{
	if (g_symbol_map[addr] == NULL) {
		SymbolEntry *s = new SymbolEntry();
		s->type = type;
		s->name = name;
		g_symbol_map[addr] = s;
	}	
}
static int analyseSymbols(bool secondRun)
{
	cs_err err = cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &handle);
	if (err) {
		return -1;
	}

	cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

	cs_insn *insn = cs_malloc(handle);

	const uint8_t *code = g_text_seg;
	uint32_t size = g_mod_info_offset;
	uint32_t address = g_text_addr;

	while (size > 0) {
		// Disassemble
		int res = cs_disasm_iter(handle, &code, (size_t *)&size, (uint64_t *)&address, insn);
		if (res == 0) {
			address += 2;
			code += 2;
			size -= 2;
			continue;
		}

		cs_arm *arm = &(insn->detail->arm);

		std::string mnemonic = trimMnemonic(arm, insn->mnemonic, NULL, NULL);

		if (!secondRun) {
			bool isBranch = mnemonic.compare("b") == 0;
			if (isBranch || mnemonic.compare("bl") == 0 || mnemonic.compare("blx") == 0) {
				cs_arm_op *op = &(arm->operands[0]);

				if ((uint32_t)op->imm >= g_text_addr && (uint32_t)op->imm < (g_text_addr+g_text_size)) {
					if (g_symbol_map[op->imm] == NULL) {
						char name[64];
						snprintf(name, sizeof(name), "%s_%08X", isBranch ? "loc" : "sub", op->imm);
						addSymbol(op->imm, name, isBranch ? SYMBOL_LABEL : SYMBOL_SUBROUTINE);
					}
				}
			}
			
			if (mnemonic.compare("cbz") == 0 || mnemonic.compare("cbnz") == 0) {
				cs_arm_op *op = &(arm->operands[1]);

				if ((uint32_t)op->imm >= g_text_addr && (uint32_t)op->imm < (g_text_addr+g_text_size)) {
					if (g_symbol_map[op->imm] == NULL) {
						char name[64];
						snprintf(name, sizeof(name), "loc_%08X", op->imm);
						addSymbol(op->imm, name, SYMBOL_LABEL);
					}
				}
			}
			
			if (mnemonic.compare("mov") == 0 || mnemonic.compare("movw") == 0) {
				cs_arm_op *op0 = &(arm->operands[0]);
				cs_arm_op *op1 = &(arm->operands[1]);
				g_movw_map[op0->mem.base] = op1->imm;
			}

			if (mnemonic.compare("movt") == 0) {
				cs_arm_op *op0 = &(arm->operands[0]);
				cs_arm_op *op1 = &(arm->operands[1]);
				uint32_t value = (op1->imm << 16) | g_movw_map[op0->mem.base];

				if (value >= g_text_addr && value < g_text_addr+g_text_size) {
					//if (value < g_text_addr+g_mod_info_offset || !isascii4((uintptr_t*)value)) {//not sure about this
					if (value > g_text_addr+g_mod_info_offset||isascii4((uintptr_t*)(g_text_seg + (value-g_text_addr)))) {//not sure about this
						// Is ascii string with minimum length of 4
						char *string = (char *)(g_text_seg + (value-g_text_addr));
						std::string string_data(string);
					
						addSymbol(value, string_data, SYMBOL_STRING);
					} else {	
						char name[64];
						snprintf(name, sizeof(name), "sub_%08X", value & ~0x1);
						addSymbol(value & ~0x1, name, SYMBOL_SUBROUTINE);					


					}
				}
			}
		} else {
			const uint8_t *code2 = code;
			uint32_t size2 = size;
			uint32_t address2 = address;
			cs_disasm_iter(handle, &code2, (size_t *)&size2, (uint64_t *)&address2, insn);

			std::string mnemonic2(insn->mnemonic);

			if (mnemonic.compare("pop") == 0 || mnemonic.compare("bx") == 0) {
				uint32_t addr = address;
				if (mnemonic2.compare("nop") == 0)
					addr = address2;
				if (g_symbol_map[addr] == NULL) {
					char name[64];
					snprintf(name, sizeof(name), "sub_%08X", addr);
					addSymbol(addr, name, SYMBOL_SUBROUTINE);
				}
			}
		}
	}

	if (secondRun) {
		// Subroutine at text_addr+0 doesn't exist yet
		if (g_symbol_map[g_text_addr] == NULL) {
			char name[64];
			snprintf(name, sizeof(name), "sub_%08X", g_text_addr);
			addSymbol(g_text_addr, name, SYMBOL_SUBROUTINE);
		}
	}

	cs_free(insn, 1);

	cs_close(&handle);

	return 0;
}

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

static uint8_t *handleSegments(int compression, FILE *fin, Elf32_Phdr *phdr) {
	std::cerr << std::hex << "(" << phdr->p_vaddr << " - " << phdr->p_vaddr + phdr->p_memsz << ")(FILE OFF: " <<  phdr->p_offset << ")(FILE SZ: " <<  phdr->p_filesz << ")(MEM SZ: " <<  phdr->p_memsz << ")" << std::endl;
	
	size_t sz = phdr->p_memsz  > phdr->p_filesz  ? phdr->p_memsz : phdr->p_filesz;
	uint8_t *input = (uint8_t *)calloc(1,sz);
	if (!input) {
		perror("Error could not allocate memory.");
		return 0;
	}
	fseek(fin, phdr->p_offset, SEEK_SET);
	fread(input, phdr->p_filesz, 1, fin);
	if(compression == 2) {
		std::cerr<< "The segment is compressed" << std::endl;
		uint8_t *destination =(uint8_t *)calloc(1,sz);
		if (!destination) {
			perror("Error could not allocate memory.");
			return 0;
		}
		int ret = uncompress(destination, (long unsigned int*)&sz, input, phdr->p_filesz);
		if(ret != Z_OK) {
			fprintf(stderr, "Warning: could not uncompress segment (No segment?): %d\n",ret);
			free(destination);
			return input;
		}
		free(input);
		return destination;
	}
	return input;
}

static void usage(char *argv[])
{
	fout << argv[0] << " binary" << " yml" << " relocs<1>";
}

// Example(with relocs): vitadecompiler -r eboot.bin db.yml
// Example: vitadecompiler eboot.bin db.yml


int main(int argc, char *argv[])
{
	std::string fname, dbname, opt_ver = "3.60";
	int offset = 0;
	FILE *fin = NULL;
	int opt_relocs = 0, opt_yaml = 0;
	if (argc < 2) {
		usage(argv);
		return 1;
	}
	if(argv[1][0] == '-') {
		if(strchr(argv[1], 'r')) {
			opt_relocs = 1;
		}
		if(strchr(argv[1], 'y')) {
			opt_yaml = 1;
		}
		if(strchr(argv[1], 'v')) {
			opt_ver = std::string(argv[2]);
			offset++;
		}
		offset++;
	} 
	fname = std::string(argv[1+offset]);
	dbname = std::string(argv[2+offset]);		
	imports = vita_imports_load(dbname.c_str(), 0);
	fin = fopen(fname.c_str(), "rb");
	
	if (!fin) {
		perror("Failed to open input file");
		fclose(fin);
		return 0;
	}
	uint8_t *input = (uint8_t*)calloc(1, HEADER_LEN);
	if (!input) {
		perror("Failed to allocate buffer for header");
		fclose(fin);
		return 0;
	}
	if (fread(input, HEADER_LEN, 1, fin) != 1) {
		static const char s[] = "Failed to read input file";
		if (feof(fin))
			fprintf(stderr, "%s: unexpected end of file\n", s);
		else
			perror(s);
		fclose(fin);
		return 0;
	}
	path += fname;
	SCE_header *shdr = (SCE_header*)(input);
	ELF_header *ehdr = (ELF_header *)(input);
	Elf32_Phdr *phdr = (Elf32_Phdr *)(input + ehdr->e_phoff);
	if(shdr->magic == 0x454353) {
		ehdr = (ELF_header *)(input + shdr->elf_offset);
		phdr = (Elf32_Phdr *)(input + shdr->phdr_offset);
		
	} else {
		if(memcmp(input,"\177ELF\1\1\1",8)!=0) {
			std::cerr << "File is not an elf" << std::endl;
			return 0;
		}	
	}
	for(int i = 0;i < ehdr->e_phnum; i++) {
		int compression = 0;
		if(shdr->magic == 0x454353) {
			segment_info *sinfo = (segment_info *)(input + shdr->section_info_offset);
			phdr[i].p_offset = sinfo[i].offset;
			if(sinfo[i].length > phdr->p_filesz)
				phdr[i].p_filesz = sinfo[i].length;
			compression = sinfo[i].compression;
		}
		uint8_t* segment = handleSegments(compression, fin, &phdr[i]);

		switch(i) {
			case 0:
				g_text_addr = phdr[i].p_vaddr;
				g_text_size = phdr[i].p_memsz  > phdr[i].p_filesz  ? phdr[i].p_memsz : phdr[i].p_filesz;
				std::cerr << "Setting Text Segment to Segment " << i << std::endl; 
				g_text_seg = segment;
				break;
			case 1:
				g_data_addr = phdr[i].p_vaddr;
				g_data_size = phdr[i].p_memsz  > phdr[i].p_filesz  ? phdr[i].p_memsz : phdr[i].p_filesz;
				std::cerr << "Setting Data Segment to Segment " << i << std::endl; 
				g_data_seg = segment;
				break;
			default:
				if(opt_relocs&&phdr[i].p_type==0x60000000) {
					std::cerr << "Performing relocations using Segment " << i << std::endl; 
					uvl_relocate((uint32_t*)segment, phdr[i].p_filesz, phdr, g_data_seg, g_text_seg);	
				}
				//free(segment); causes crashes?
				break;
		}

	}
	fclose(fin);
	fin = NULL;
	if(!g_text_seg || !g_data_seg )
		return 0;	
	
	SceModuleInfo *mod_info = (SceModuleInfo *)(g_text_seg+ehdr->e_entry);
	if(!isascii4((uintptr_t*)mod_info->name)) {
		std::cerr << "Invalid entrypoint" << std::endl;
		std::cerr << "Attempting to find entrypoint..."  << std::endl;
		uint32_t ep = g_text_size;
		std::cerr << "Starting at:" << ep << std::endl;
		uint8_t check[16] = {0xE0,0xE3,0x1E,0xFF,0x2F,0xE1,0x00,0x00,0xA0,0xE1,0x00,0x00,0x00,0x00,0x00,0x00};	
		int i = 0;
		while(i < sizeof(check)) {
			if(g_text_addr+ep <= g_text_addr){
				std::cerr << "Cannot find entrypoint..." << std::endl;
				return 0;
			}
			if(*(g_text_seg+ep+i) != check[i]) {
				ep--;
				i = 0;
				continue;
			}
			i++;	
		}
		std::cerr << std::hex << "Found check: " << ep << std::endl;
		ep += sizeof(check) - 0x2;
		mod_info = (SceModuleInfo*)(g_text_seg + ep);
		ehdr->e_entry = ep;
		std::cerr << "**Please correct your elf**" << std::endl;
	}
	std::cerr << std::hex << "Entry Point: (" << ehdr->e_entry << ")(FILE OFF:" << ehdr->e_entry + phdr[0].p_offset << ")"<< std::endl;	
	
	g_mod_info_offset = analyseVitaModule(ehdr->e_entry, g_text_seg, g_text_addr , g_text_size, path, opt_ver);
	if (g_mod_info_offset == 0) {
		std::cerr << "Error could not find module info." << std::endl;
		g_mod_info_offset = g_text_size;
	}
	if(opt_yaml){
		free(input);
		free(g_data_seg);
		free(g_text_seg);
		exit(0);
	}
	fout.open(path + ".c");
	if(!fout) {
		std::cerr << "Error creating output..." << std::endl;
		return 0;
	}
	fout <<std::hex <<  std::showbase;
	std::cerr << "Exporting source file to: " << path << ".c" << std::endl;		
	std::cerr << "Analysing symbols..." << std::endl;
	analyseSymbols(false);
	analyseSymbols(true);

	std::cerr << "Analysing arguments..." << std::endl;
	analyseArguments();

	std::cerr << "Analysing code..." << std::endl;
	analyseCode();

	std::cerr << "Decompiling..." << std::endl;
	decompile();

	std::cerr << "Finished." << std::endl;
	free(input);
	free(g_data_seg);
	free(g_text_seg);
	fout.close();
	exit(0);
	return 0;
}
