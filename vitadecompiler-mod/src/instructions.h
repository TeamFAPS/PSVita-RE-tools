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

#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include <capstone/capstone.h>

struct Condition {
	std::string cond_flags;
	std::string cond_regs;
};

std::map<std::string, Condition> conditions = {
	{ "eq", { "if (Z == 1) // ==", "if (COND_X == COND_Y)" } },
	{ "ne", { "if (Z == 0) // !=", "if (COND_X != COND_Y)" } },

	{ "hs", { "if (C == 1) // unsigned >=", "if ((unsigned)(COND_X) >= (unsigned)(COND_Y))" } },
	{ "lo", { "if (C == 0) // unsigned <", "if ((unsigned)(COND_X) < (unsigned)(COND_Y))" } },
	{ "hi", { "if (C == 1 && Z == 0) // unsigned >", "if ((unsigned)(COND_X) > (unsigned)(COND_Y))" } },
	{ "ls", { "if (C == 0 || Z == 1) // unsigned <=", "if ((unsigned)(COND_X) <= (unsigned)(COND_Y))" } },

	{ "ge", { "if (N == V) // signed >=", "if ((signed)(COND_X) >= (signed)(COND_Y))" } },
	{ "lt", { "if (N != V) // signed <", "if ((signed)(COND_X) < (signed)(COND_Y))" } },
	{ "gt", { "if (Z == 0 && N == V) // signed >", "if ((signed)(COND_X) > (signed)(COND_Y))" } },
	{ "le", { "if (Z == 1 || N != V) // signed <=", "if ((signed)(COND_X) <= (signed)(COND_Y))" } },

	{ "mi", { "if (N == 1) // signed < 0", "" } },
	{ "pl", { "if (N == 0) // signed > 0", "" } },

	{ "vs", { "if (V == 1) // Signed overflow", "" } },
	{ "vc", { "if (V == 0) // No signed overflow", "" } },

	{ "al", { "if (1) // Always", "" } },
};

std::map<int, std::string> shifts = {
	{ ARM_SFT_ASR, ">>" },
	{ ARM_SFT_LSL, "<<" },
	{ ARM_SFT_LSR, ">>" },
	// { ARM_SFT_ROR, "" },
	// { ARM_SFT_RRX, "" },

	{ ARM_SFT_ASR_REG, ">>" },
	{ ARM_SFT_LSL_REG, "<<" },
	{ ARM_SFT_LSR_REG, ">>" },
	// { ARM_SFT_ROR_REG, "" },
	// { ARM_SFT_RRX_REG, "" },
};

std::map<std::string, std::string> instructions = {
	// Comparisons
	{ "cmp", "FLAGS = op0 - op1;" },
	{ "cmn", "FLAGS = op0 + op1;" },
	{ "tst", "FLAGS = op0 & op1;" },

	// Branches
	{ "cbz", "if (op0 == 0)\\\tgoto symbol;" },
	{ "cbnz", "if (op0 != 0)\\\tgoto symbol;" },

	{ "b", "goto symbol;" },

	{ "bl", "a1 = symbol(...);" },
	{ "blx", "a1 = symbol(...);" },

	// Arithmetic operations
	{ "add", "op0 = op1 + op2;" },
	{ "addw", "op0 = op1 + op2;" },
	{ "adc", "op0 = op1 + op2;" },

	{ "sub", "op0 = op1 - op2;" },
	{ "subw", "op0 = op1 - op2;" },
	{ "rsb", "op0 = op2 - op1;" },
	{ "sbc", "op0 = op1 - op2;" },

	{ "asr", "op0 = op1 >> op2;" },
	{ "asl", "op0 = op1 << op2;" },

	{ "mul", "op0 = op1 * op2;" },
	{ "mla", "op0 = op1 * op2 + op3;" },
	{ "mls", "op0 = op1 * op2 - op3;" },

	{ "umull", "op0 = (op2 * op3) << 32;\\op1 = (op2 * op3) & 0xFFFFFFFF;" },

	// Logical operations
	{ "and", "op0 = op1 & op2;" },
	{ "bic", "op0 = op1 & ~op2;" },
	{ "eor", "op0 = op1 ^ op2;" },
	{ "orr", "op0 = op1 | op2;" },

	{ "lsr", "op0 = op1 >> op2;" },
	{ "lsl", "op0 = op1 << op2;" },

	{ "ubfx", "op0 = (op1 >> op2) & ((1 << op3) - 1);" },

	{ "rev", "op0 = __builtin_bswap32(op1);" },
	{ "rev16", "op0 = __builtin_bswap16(op1);" },

	// Move operations
	{ "mov", "op0 = op1;" },
	{ "mvn", "op0 = ~op1;" },
	{ "movt", "op0 = op10000 | op0;" },
	{ "movw", "op0 = op1;" },
	{ "uxtb", "op0 = (uint8_t)op1;" },
	{ "uxth", "op0 = (uint16_t)op1;" },
	{ "sxtb", "op0 = (int8_t)op1;" },
	{ "sxth", "op0 = (int16_t)op1;" },

	// Load operations
	{ "ldr", "op0 = *(uint32_t *)(op1);" },
	{ "ldrb", "op0 = *(uint8_t *)(op1);" },
	{ "ldrsb", "op0 = *(int8_t *)(op1);" },
	{ "ldrh", "op0 = *(uint16_t *)(op1);" },
	{ "ldrsh", "op0 = *(int16_t *)(op1);" },
	{ "ldrd", "op0 = *(uint32_t *)(op2);\\op1 = *(uint32_t *)(op2 + 0x4);" },

	// Store operations
	{ "str", "*(uint32_t *)(op1) = op0;" },
	{ "strb", "*(uint8_t *)(op1) = op0;" },
	{ "strsb", "*(int8_t *)(op1) = op0;" },
	{ "strh", "*(uint16_t *)(op1) = op0;" },
	{ "strsh", "*(int16_t *)(op1) = op0;" },
	{ "strd", "*(uint32_t *)(op2) = op0;\\*(uint32_t *)(op2 + 0x4) = op1;" },
	
	// TODO: ldm/stm
};

std::map<std::string, std::string> instructions2 = {
	// Arithmetic operations
	{ "add", "op0 = op0 + op1;" },
	{ "sub", "op0 = op0 - op1;" },
	{ "mul", "op0 = op0 * op1;" },

	{ "asr", "op0 = op0 >> op1;" },
	{ "asl", "op0 = op0 << op1;" },

	// Logical operations
	{ "and", "op0 = op0 & op1;" },
	{ "bic", "op0 = op0 & ~op1;" },
	{ "eor", "op0 = op0 ^ op1;" },
	{ "orr", "op0 = op0 | op1;" },

	{ "lsr", "op0 = op0 >> op1;" },
	{ "lsl", "op0 = op0 << op1;" },
};

#endif