/*
 *	Copyright 2021 Andrey Terekhov, Ivan S. Arkhipov
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "mipsgen.h"
#include "AST.h"
#include "hash.h"
#include "ir.h"
#include "operations.h"
#include "tree.h"
#include "uniprinter.h"


#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG

#define unimplemented()\
	do {\
		printf("\nИспользуется нереализованная фича (смотри %s:%d)\n", __FILE__, __LINE__);\
	} while(0)

#define unreachable() \
	do {\
		printf("\nДостигнут участок кода, который считался недосягаемым (смотри %s:%d)\n", __FILE__, __LINE__);\
	} while(0)

#else

inline unimplemented(const char* msg)
{
	printf("\nКод для использованной функции не реализован: %s\n", msg);
	system_error(node_unexpected);
}
inline unreachable(const char* msg)
{
	printf("\nКод для использованной функции не реализован: %s\n", msg);
	system_error(node_unexpected);
}

#endif

// // #ifndef max
// // 	#define max(a, b) ((a) > (b) ? (a) : (b))
// // #endif


// // static const size_t BUFFER_SIZE = 65536;			/**< Размер буфера для тела функции */
// // static const size_t HASH_TABLE_SIZE = 1024;			/**< Размер хеш-таблицы для смещений и регистров */
// // static const bool IS_ON_STACK = true;				/**< Хранится ли переменная на стеке */

// // static const size_t WORD_LENGTH = 4;				/**< Длина слова данных */
// // static const size_t HALF_WORD_LENGTH = 2;			/**< Длина половины слова данных */

// // static const size_t LOW_DYN_BORDER = 0x10010000;	/**< Нижняя граница динамической памяти */
// // static const size_t HEAP_DISPL = 8000;				/**< Смещение кучи относительно глобальной памяти */

// // static const size_t SP_SIZE = 4;					/**< Размер регистра $sp для его сохранения */
// // static const size_t RA_SIZE = 4;					/**< Размер регистра $ra для его сохранения */

// // static const size_t TEMP_FP_REG_AMOUNT = 12;		/**< Количество временных регистров для чисел с плавающей точкой */
// // static const size_t TEMP_REG_AMOUNT = 10;			/**< Количество обычных временных регистров */
// // static const size_t ARG_REG_AMOUNT = 4;				/**< Количество регистров-аргументов для функций */

// // static const size_t PRESERVED_REG_AMOUNT = 8;		/**< Количество сохраняемых регистров общего назначения */
// // static const size_t PRESERVED_FP_REG_AMOUNT = 10;	/**< Количество сохраняемых регистров с плавающей точкой */

// // static const bool FROM_LVALUE = 1;					/**< Получен ли rvalue из lvalue */

// // /**< Смещение в стеке для сохранения оберегаемых регистров, без учёта оптимизаций */
// // static const size_t FUNC_DISPL_PRESEREVED = /* за $sp */ 4 + /* за $ra */ 4 +
// // 											/* fs0-fs10 (одинарная точность): */ 5 * 4 + /* s0-s7: */ 8 * 4;

#define mips_printf(enc, ...)\
	do {\
		uni_printf(enc->io, __VA_ARGS__);\
	} while(0)

// // Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition
typedef enum mips_register
{
	MIPS_R_ZERO,				/**< Always has the value 0 */
	MIPS_R_AT,				/**< Temporary, generally used by assembler */

	MIPS_R_V0,
	MIPS_R_V1,				/**< Used for expression evaluations and to hold the integer
							and pointer type function return values */

	MIPS_R_A0,
	MIPS_R_A1,
	MIPS_R_A2,
	MIPS_R_A3,				/**< Used for passing arguments to functions; values are not
							preserved across function calls */

	MIPS_R_T0,
	MIPS_R_T1,
	MIPS_R_T2,
	MIPS_R_T3,
	MIPS_R_T4,
	MIPS_R_T5,
	MIPS_R_T6,
	MIPS_R_T7,				/**< Temporary registers used for expression evaluation;
							values are not preserved across function calls */

	MIPS_R_S0,
	MIPS_R_S1,
	MIPS_R_S2,
	MIPS_R_S3,
	MIPS_R_S4,
	MIPS_R_S5,
	MIPS_R_S6,
	MIPS_R_S7,				/**< Saved registers; values are preserved across function calls */

	MIPS_R_T8,
	MIPS_R_T9,				/**< Temporary registers used for expression evaluations;
							values are not preserved across function calls. When
							calling position independent functions $25 (MIPS_R_T9) must contain
							the address of the called function */

	MIPS_R_K0,
	MIPS_R_K1,				/**< Used only by the operating system */

	MIPS_R_GP,				/**< Global pointer and context pointer */
	MIPS_R_SP,				/**< Stack pointer */
	MIPS_R_FP,				/**< Saved register (like s0-s7) or frame pointer */
	MIPS_R_RA,				/**< Return address. The return address is the location to
							which a function should return control */

	// Регистры для работы с числами с плавающей точкой
	// Для чисел с двойной точностью используется пара регистров:
	// - регистр с чётным номером содержит младшие 32 бита числа;
	// - регистр с нечётным номером содержит старшие 32 бита числа.
	MIPS_R_FV0,
	MIPS_R_FV1,
	MIPS_R_FV2,
	MIPS_R_FV3,				/**< Used to hold floating-point type function results;
							single-precision uses $f0 and double-precision uses
							the register pair $f0..$f1 */

	MIPS_R_FA0,
	MIPS_R_FA1,
	MIPS_R_FA2,
	MIPS_R_FA3,				/**< Used for passing arguments to functions */

	MIPS_R_FT0,
	MIPS_R_FT1,
	MIPS_R_FT2,
	MIPS_R_FT3,
	MIPS_R_FT4,
	MIPS_R_FT5,
	MIPS_R_FT6,
	MIPS_R_FT7,
	MIPS_R_FT8,
	MIPS_R_FT9,
	MIPS_R_FT10,
	MIPS_R_FT11,				/**< Temporary registers */

	MIPS_R_FS0,
	MIPS_R_FS1,
	MIPS_R_FS2,
	MIPS_R_FS3,
	MIPS_R_FS4,
	MIPS_R_FS5,
	MIPS_R_FS6,
	MIPS_R_FS7,
	MIPS_R_FS8,
	MIPS_R_FS9,
	MIPS_R_FS10,
	MIPS_R_FS11				/**< Saved registers; their values are preserved across function calls */
} mips_register;

// Назначение команд взято из документации MIPS® Architecture for Programmers
// Volume II-A: The MIPS32® Instruction
// Set Manual 2016
typedef enum mips_ic
{
	MIPS_IC_MOVE,		/**< MIPS Pseudo-Instruction. Move the contents of one register to another */
	MIPS_IC_LI,			/**< MIPS Pseudo-Instruction. Load a constant into a register */
	MIPS_IC_NOT,		/**< MIPS Pseudo-Instruction. Flips the bits of the source register and
							stores them in the destination register (не из вышеуказанной книги) */

	MIPS_IC_ADDI,		/**< To add a constant to a 32-bit integer. If overflow occurs, then trap */
	MIPS_IC_SLL,		/**< To left-shift a word by a fixed number of bits */
	MIPS_IC_SRA,		/**< To execute an arithmetic right-shift of a word by a fixed number of bits */
	MIPS_IC_ANDI,		/**< To do a bitwise logical AND with a constant */
	MIPS_IC_XORI,		/**< To do a bitwise logical Exclusive OR with a constant */
	MIPS_IC_ORI,		/**< To do a bitwise logical OR with a constant */

	MIPS_IC_ADD,		/**< To add 32-bit integers. If an overflow occurs, then trap */
	MIPS_IC_SUB,		/**< To subtract 32-bit integers. If overflow occurs, then trap */
	MIPS_IC_MUL,		/**< To multiply two words and write the result to a GPR */
	MIPS_IC_DIV,		/**< DIV performs a signed 32-bit integer division, and places
							the 32-bit quotient result in the destination register */
	MIPS_IC_MOD,		/**< MOD performs a signed 32-bit integer division, and places
							the 32-bit remainder result in the destination register.
							The remainder result has the same sign as the dividend */
	MIPS_IC_SLLV,		/**< To left-shift a word by a variable number of bits */
	MIPS_IC_SRAV,		/**< To execute an arithmetic right-shift of a word by a variable number of bits */
	MIPS_IC_AND,		/**< To do a bitwise logical AND */
	MIPS_IC_XOR,		/**< To do a bitwise logical Exclusive OR */
	MIPS_IC_OR,			/**< To do a bitwise logical OR */

	MIPS_IC_SW,			/**< To store a word to memory */
	MIPS_IC_LW,			/**< To load a word from memory as a signed value */

	MIPS_IC_JR,			/**< To execute a branch to an instruction address in a register */
	MIPS_IC_JAL,		/**< To execute a procedure call within the current 256MB-aligned region */
	MIPS_IC_J,			/**< To branch within the current 256 MB-aligned region */

	MIPS_IC_BEQZ,
	MIPS_IC_BNEZ,
	MIPS_IC_BEQ,
	MIPS_IC_BLT,
	MIPS_IC_BLE,
	// MIPS_IC_BLEZ,		/**< Branch on Less Than or Equal to Zero.
	// 						To test a GPR then do a PC-relative conditional branch */
	// MIPS_IC_BLTZ,		/**< Branch on Less Than Zero.
	// 						To test a GPR then do a PC-relative conditional branch */
	// MIPS_IC_BGEZ,		/**< Branch on Greater Than or Equal to Zero.
	// 						To test a GPR then do a PC-relative conditional branch */
	// MIPS_IC_BGTZ,		/**< Branch on Greater Than Zero.
	// 						To test a GPR then do a PC-relative conditional branch */
	// MIPS_IC_BEQ,		/**< Branch on Equal.
	// 						To compare GPRs then do a PC-relative conditional branch */
	// MIPS_IC_BNE,		/**< Branch on Not Equal.
	// 						To compare GPRs then do a PC-relative conditional branch */

	MIPS_IC_LA,			/**< Load the address of a named memory
							location into a register (не из вышеуказанной книги)*/

	MIPS_IC_SLTIU,		/**< Set on Less Than Immediate Unsigned.
							To record the result of an unsigned less-than comparison with a constant. */

	MIPS_IC_NOP,		/**< To perform no operation */

	/** Floating point operations. Single precision. */
	MIPS_IC_ADD_S,		/**< To add FP values. */
	MIPS_IC_SUB_S,		/**< To subtract FP values. */
	MIPS_IC_MUL_S,		/**< To multiply FP values. */
	MIPS_IC_DIV_S,		/**< To divide FP values. */

	MIPS_IC_ABS_S,		/**< Floating Point Absolute Value*/
	MIPS_IC_ABS,		/**< GPR absolute value (не из вышеуказанной книги). MIPS Pseudo-Instruction. */

	MIPS_IC_S_S,		/**< MIPS Pseudo instruction. To store a doubleword from an FPR to memory. */
	MIPS_IC_L_S,		/**< MIPS Pseudo instruction. To load a doubleword from memory to an FPR. */

	MIPS_IC_LI_S,		/**< MIPS Pseudo-Instruction. Load a FP constant into a FPR. */

	MIPS_IC_MOV_S,		/**< The value in first FPR is placed into second FPR. */

	MIPS_IC_MFC_1,		/**< Move word from Floating Point.
							To copy a word from an FPU (CP1) general register to a GPR. */
	MIPS_IC_MFHC_1,		/**< To copy a word from the high half of an FPU (CP1)
							general register to a GPR. */

	MIPS_IC_CVT_D_S,	/**< To convert an FP value to double FP. */
	MIPS_IC_CVT_S_W,	/**< To convert fixed point value to single FP. */
	MIPS_IC_CVT_W_S,	/**< To convert single FP to fixed point value */
} mips_ic;


typedef struct mips_encoder
{
	syntax *sx;								/**< Структура syntax с таблицами */
	universal_io *io;

} mips_encoder;

mips_encoder create_mips_encoder(const syntax *const sx)
{
	mips_encoder enc;

	enc.sx = sx;
	enc.io = enc.sx->io;

	return enc;
}

static void mips_print_register(mips_encoder *const enc, const mips_register reg)
{
	switch (reg)
	{
		case MIPS_R_ZERO:
			mips_printf(enc, "$0");
			break;
		case MIPS_R_AT:
			mips_printf(enc, "$at");
			break;

		case MIPS_R_V0:
			mips_printf(enc, "$v0");
			break;
		case MIPS_R_V1:
			mips_printf(enc, "$v1");
			break;

		case MIPS_R_A0:
			mips_printf(enc, "$a0");
			break;
		case MIPS_R_A1:
			mips_printf(enc, "$a1");
			break;
		case MIPS_R_A2:
			mips_printf(enc, "$a2");
			break;
		case MIPS_R_A3:
			mips_printf(enc, "$a3");
			break;

		case MIPS_R_T0:
			mips_printf(enc, "$t0");
			break;
		case MIPS_R_T1:
			mips_printf(enc, "$t1");
			break;
		case MIPS_R_T2:
			mips_printf(enc, "$t2");
			break;
		case MIPS_R_T3:
			mips_printf(enc, "$t3");
			break;
		case MIPS_R_T4:
			mips_printf(enc, "$t4");
			break;
		case MIPS_R_T5:
			mips_printf(enc, "$t5");
			break;
		case MIPS_R_T6:
			mips_printf(enc, "$t6");
			break;
		case MIPS_R_T7:
			mips_printf(enc, "$t7");
			break;

		case MIPS_R_S0:
			mips_printf(enc, "$s0");
			break;
		case MIPS_R_S1:
			mips_printf(enc, "$s1");
			break;
		case MIPS_R_S2:
			mips_printf(enc, "$s2");
			break;
		case MIPS_R_S3:
			mips_printf(enc, "$s3");
			break;
		case MIPS_R_S4:
			mips_printf(enc, "$s4");
			break;
		case MIPS_R_S5:
			mips_printf(enc, "$s5");
			break;
		case MIPS_R_S6:
			mips_printf(enc, "$s6");
			break;
		case MIPS_R_S7:
			mips_printf(enc, "$s7");
			break;

		case MIPS_R_T8:
			mips_printf(enc, "$t8");
			break;
		case MIPS_R_T9:
			mips_printf(enc, "$t9");
			break;

		case MIPS_R_K0:
			mips_printf(enc, "$k0");
			break;
		case MIPS_R_K1:
			mips_printf(enc, "$k1");
			break;

		case MIPS_R_GP:
			mips_printf(enc, "$gp");
			break;
		case MIPS_R_SP:
			mips_printf(enc, "$sp");
			break;
		case MIPS_R_FP:
			mips_printf(enc, "$fp");
			break;
		case MIPS_R_RA:
			mips_printf(enc, "$ra");
			break;

		case MIPS_R_FV0:
			mips_printf(enc, "$f0");
			break;
		case MIPS_R_FV1:
			mips_printf(enc, "$f1");
			break;
		case MIPS_R_FV2:
			mips_printf(enc, "$f2");
			break;
		case MIPS_R_FV3:
			mips_printf(enc, "$f3");
			break;

		case MIPS_R_FT0:
			mips_printf(enc, "$f4");
			break;
		case MIPS_R_FT1:
			mips_printf(enc, "$f5");
			break;
		case MIPS_R_FT2:
			mips_printf(enc, "$f6");
			break;
		case MIPS_R_FT3:
			mips_printf(enc, "$f7");
			break;
		case MIPS_R_FT4:
			mips_printf(enc, "$f8");
			break;
		case MIPS_R_FT5:
			mips_printf(enc, "$f9");
			break;
		case MIPS_R_FT6:
			mips_printf(enc, "$f10");
			break;
		case MIPS_R_FT7:
			mips_printf(enc, "$f11");
			break;
		case MIPS_R_FT8:
			mips_printf(enc, "$f16");
			break;
		case MIPS_R_FT9:
			mips_printf(enc, "$f17");
			break;
		case MIPS_R_FT10:
			mips_printf(enc, "$f18");
			break;
		case MIPS_R_FT11:
			mips_printf(enc, "$f19");
			break;

		case MIPS_R_FA0:
			mips_printf(enc, "$f12");
			break;
		case MIPS_R_FA1:
			mips_printf(enc, "$f13");
			break;
		case MIPS_R_FA2:
			mips_printf(enc, "$f14");
			break;
		case MIPS_R_FA3:
			mips_printf(enc, "$f15");
			break;

		case MIPS_R_FS0:
			mips_printf(enc, "$f20");
			break;
		case MIPS_R_FS1:
			mips_printf(enc, "$f21");
			break;
		case MIPS_R_FS2:
			mips_printf(enc, "$f22");
			break;
		case MIPS_R_FS3:
			mips_printf(enc, "$f23");
			break;
		case MIPS_R_FS4:
			mips_printf(enc, "$f24");
			break;
		case MIPS_R_FS5:
			mips_printf(enc, "$f25");
			break;
		case MIPS_R_FS6:
			mips_printf(enc, "$f26");
			break;
		case MIPS_R_FS7:
			mips_printf(enc, "$f27");
			break;
		case MIPS_R_FS8:
			mips_printf(enc, "$f28");
			break;
		case MIPS_R_FS9:
			mips_printf(enc, "$f29");
			break;
		case MIPS_R_FS10:
			mips_printf(enc, "$f30");
			break;
		case MIPS_R_FS11:
			mips_printf(enc, "$f31");
			break;
	}
}

mips_register mips_get_temp_register(mips_encoder *const enc, const item_t id)
{
	(void) enc;
	switch (id)
	{
		case 0:
			return MIPS_R_T0;
		case 1:
			return MIPS_R_T1;
		case 2:
			return MIPS_R_T2;
		case 3:
			return MIPS_R_T3;
		case 4:
			return MIPS_R_T4;
		case 5:
			return MIPS_R_T5;
		case 6:
			return MIPS_R_T6;
		case 7:
			return MIPS_R_T7;
		case 8:
			return MIPS_R_T8;
		case 9:
			return MIPS_R_T9;
		default:
			// FIXME: Error handling.
			return MIPS_R_T0;
	}
}
mips_register mips_get_temp_float_register(mips_encoder *const enc, const item_t id)
{
	(void) enc;
	// FIXME: Dev.
	switch (id)
	{
		case 0:
			return MIPS_R_T0;
		case 1:
			return MIPS_R_T1;
		case 2:
			return MIPS_R_T2;
		case 3:
			return MIPS_R_T3;
		case 4:
			return MIPS_R_T4;
		case 5:
			return MIPS_R_T5;
		case 6:
			return MIPS_R_T6;
		case 7:
			return MIPS_R_T7;
		case 8:
			return MIPS_R_T8;
		case 9:
			return MIPS_R_T9;
		default:
			// FIXME: Error handling.
			return MIPS_R_T0;
	}
}


static void mips_print_ic(mips_encoder *const enc, const mips_ic ic)
{
	switch (ic)
	{
		case MIPS_IC_MOVE:
			mips_printf(enc, "move");
			break;
		case MIPS_IC_LI:
			mips_printf(enc, "li");
			break;
		case MIPS_IC_LA:
			mips_printf(enc, "la");
			break;
		case MIPS_IC_NOT:
			mips_printf(enc, "not");
			break;

		case MIPS_IC_ADDI:
			mips_printf(enc, "addi");
			break;
		case MIPS_IC_SLL:
			mips_printf(enc, "sll");
			break;
		case MIPS_IC_SRA:
			mips_printf(enc, "sra");
			break;
		case MIPS_IC_ANDI:
			mips_printf(enc, "andi");
			break;
		case MIPS_IC_XORI:
			mips_printf(enc, "xori");
			break;
		case MIPS_IC_ORI:
			mips_printf(enc, "ori");
			break;

		case MIPS_IC_ADD:
			mips_printf(enc, "add");
			break;
		case MIPS_IC_SUB:
			mips_printf(enc, "sub");
			break;
		case MIPS_IC_MUL:
			mips_printf(enc, "mul");
			break;
		case MIPS_IC_DIV:
			mips_printf(enc, "div");
			break;
		case MIPS_IC_MOD:
			mips_printf(enc, "mod");
			break;
		case MIPS_IC_SLLV:
			mips_printf(enc, "sllv");
			break;
		case MIPS_IC_SRAV:
			mips_printf(enc, "srav");
			break;
		case MIPS_IC_AND:
			mips_printf(enc, "and");
			break;
		case MIPS_IC_XOR:
			mips_printf(enc, "xor");
			break;
		case MIPS_IC_OR:
			mips_printf(enc, "or");
			break;

		case MIPS_IC_SW:
			mips_printf(enc, "sw");
			break;
		case MIPS_IC_LW:
			mips_printf(enc, "lw");
			break;

		case MIPS_IC_JR:
			mips_printf(enc, "jr");
			break;
		case MIPS_IC_JAL:
			mips_printf(enc, "jal");
			break;
		case MIPS_IC_J:
			mips_printf(enc, "j");
			break;

		case MIPS_IC_BEQZ:
			mips_printf(enc, "beqz");
			break;
		case MIPS_IC_BNEZ:
			mips_printf(enc, "bnez");
			break;
		case MIPS_IC_BEQ:
			mips_printf(enc, "beq");
			break;
		case MIPS_IC_BLT:
			mips_printf(enc, "blt");
			break;
		case MIPS_IC_BLE:
			mips_printf(enc, "ble");
			break;

		case MIPS_IC_SLTIU:
			mips_printf(enc, "sltiu");
			break;

		case MIPS_IC_NOP:
			mips_printf(enc, "nop");
			break;

		case MIPS_IC_ADD_S:
			mips_printf(enc, "add.s");
			break;
		case MIPS_IC_SUB_S:
			mips_printf(enc, "sub.s");
			break;
		case MIPS_IC_MUL_S:
			mips_printf(enc, "mul.s");
			break;
		case MIPS_IC_DIV_S:
			mips_printf(enc, "div.s");
			break;

		case MIPS_IC_ABS_S:
			mips_printf(enc, "abs.s");
			break;
		case MIPS_IC_ABS:
			mips_printf(enc, "abs");
			break;

		case MIPS_IC_S_S:
			mips_printf(enc, "s.s");
			break;
		case MIPS_IC_L_S:
			mips_printf(enc, "l.s");
			break;

		case MIPS_IC_LI_S:
			mips_printf(enc, "li.s");
			break;

		case MIPS_IC_MOV_S:
			mips_printf(enc, "mov.s");
			break;

		case MIPS_IC_MFC_1:
			mips_printf(enc, "mfc1");
			break;
		case MIPS_IC_MFHC_1:
			mips_printf(enc, "mfhc1");
			break;

		case MIPS_IC_CVT_D_S:
			mips_printf(enc, "cvt.d.s");
			break;
		case MIPS_IC_CVT_S_W:
			mips_printf(enc, "cvt.s.w");
			break;
		case MIPS_IC_CVT_W_S:
			mips_printf(enc, "cvt.w.s");
			break;
	}
}

static void mips_print_rvalue(mips_encoder *const enc, const rvalue *const value)
{
	const syntax *const sx = enc->sx;

	switch (rvalue_get_kind(value))
	{
		case RVALUE_KIND_TEMP:
		{
			mips_register register_;
			if (type_is_floating(rvalue_get_type(value)))
				register_ = mips_get_temp_float_register(enc, temp_rvalue_get_id(value));
			else if (type_is_integer(sx, rvalue_get_type(value)))
				register_ = mips_get_temp_register(enc, temp_rvalue_get_id(value));
			mips_print_register(enc, register_);
			break;
		}
		case RVALUE_KIND_IMM:
			mips_printf(enc, "%d", imm_int_rvalue_get_value(value));
			break;
	}
}
static void mips_print_lvalue(mips_encoder *const enc, const lvalue *const value)
{
	switch (value->kind)
	{
		case LVALUE_KIND_LOCAL:
		{
			mips_printf(enc, "%d(", -local_lvalue_get_displ(value));
			mips_print_register(enc, MIPS_R_SP);
			mips_printf(enc, ")");
			break;
		}
		case LVALUE_KIND_GLOBAL:
			break;
		case LVALUE_KIND_PARAM:
		{
			mips_printf(enc, "-(");
			mips_print_register(enc, MIPS_R_SP);
			mips_printf(enc, ")");
			break;
		}
		default:
			break;
	}
}
static void mips_print_label(mips_encoder *const enc, const label *const lbl)
{
	switch (lbl->kind)
	{
		case LABEL_KIND_MAIN:
			mips_printf(enc, "MAIN");
			break;
		case LABEL_KIND_FUNC:
			mips_printf(enc, "FUNC");
			break;
		case LABEL_KIND_BEGIN:
			mips_printf(enc, "BEGIN");
			break;
		case LABEL_KIND_NEXT:
			mips_printf(enc, "NEXT");
			break;
		case LABEL_KIND_THEN:
			mips_printf(enc, "THEN");
			break;
		// case LABEL_KIND_FUNCEND:
		// 	mips_printf(enc, "FUNCEND");
		// 	break;
		// case LABEL_KIND_STRING:
		// 	mips_printf(enc, "STRING");
			break;
		case LABEL_KIND_ELSE:
			mips_printf(enc, "ELSE");
			break;
		case LABEL_KIND_END:
			mips_printf(enc, "END");
			break;
		case LABEL_KIND_BEGIN_CYCLE:
			mips_printf(enc, "BEGIN_CYCLE");
			break;
		case LABEL_KIND_OR:
			mips_printf(enc, "OR");
			break;
		case LABEL_KIND_AND:
			mips_printf(enc, "AND");
			break;
		// case LABEL_KIND_CASE:
		// 	mips_printf(enc, "CASE");
			break;
	}
	mips_printf(enc, "%" PRIitem, lbl->id);
}

static void mips_gen_nop(mips_encoder *const enc)
{

}

static void mips_to_code_b(mips_encoder *const enc, const label *const label_)
{
	mips_print_label(enc, label_);
	mips_printf(enc, ":");
	mips_printf(enc, "\n");
}

static void mips_to_code_instr_rr(mips_encoder *const enc, const mips_ic ic, const rvalue *const op1, const rvalue *const op2)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_rvalue(enc, op1);
	mips_printf(enc, ", ");
	mips_print_rvalue(enc, op2);
	mips_printf(enc, "\n");
}
static void mips_to_code_instr_lr(mips_encoder *const enc, const mips_ic ic, const lvalue *const op1, const rvalue *const op2)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_lvalue(enc, op1);
	mips_printf(enc, ", ");
	mips_print_rvalue(enc, op2);
	mips_printf(enc, "\n");
}
static void mips_to_code_instr_rl(mips_encoder *const enc, const mips_ic ic, const rvalue *const op1, const lvalue *const op2)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_rvalue(enc, op1);
	mips_printf(enc, ", ");
	mips_print_lvalue(enc, op2);
	mips_printf(enc, "\n");
}
static void mips_to_code_instr_rrr(mips_encoder *const enc, const mips_ic ic, const rvalue *const op1, const rvalue *const op2, const rvalue *const op3)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_rvalue(enc, op1);
	mips_printf(enc, ", ");
	mips_print_rvalue(enc, op2);
	mips_printf(enc, ", ");
	mips_print_rvalue(enc, op3);
	mips_printf(enc, "\n");
}
static void mips_to_code_instr_b(mips_encoder *const enc, const mips_ic ic, const label *const op1)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_label(enc, op1);
	mips_printf(enc, "\n");
}
static void mips_to_code_instr_rb(mips_encoder *const enc, const mips_ic ic, const rvalue *const op1, const label *const op2)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_rvalue(enc, op1);
	mips_printf(enc, ", ");
	mips_print_label(enc, op2);
	mips_printf(enc, "\n");
}
static void mips_to_code_instr_rrb(mips_encoder *const enc, const mips_ic ic, const rvalue *const op1, const rvalue *const op2, const label *const op3)
{
	mips_printf(enc, "\t");
	mips_print_ic(enc, ic);
	mips_printf(enc, " ");
	mips_print_rvalue(enc, op1);
	mips_printf(enc, ", ");
	mips_print_rvalue(enc, op2);
	mips_printf(enc, ", ");
	mips_print_label(enc, op3);
	mips_printf(enc, "\n");
}

static void mips_gen_label(mips_encoder *const enc, const label *const label_)
{
	mips_to_code_b(enc, label_);
}

static void mips_gen_move(mips_encoder *const enc, const rvalue *const src, const rvalue *const dest)
{
	const syntax *const sx = enc->sx;

	switch (src->kind)
	{
		case RVALUE_KIND_TEMP:
		{
			if (type_is_integer(sx, src->type))
				mips_to_code_instr_rr(enc, MIPS_IC_MOVE, dest, src);
			else if (type_is_floating(src->type))
				mips_to_code_instr_rr(enc, MIPS_IC_MOV_S, dest, src);
			else;
				// FIXME: some kind of error.
			break;
		}
		case RVALUE_KIND_IMM:
		{
			if (type_is_integer(sx, src->type))
			{
				//mips_to_code_instr_rr(enc, )

			}
			else if (type_is_floating(src->type))
			{

			}
			else
			{
				// FIXME: some kind of error.
			}
			break;
		}
	}
}

static void mips_gen_store(mips_encoder *const enc, const rvalue *const src, const lvalue *const dest)
{
	mips_to_code_instr_rl(enc, MIPS_IC_SW, src, dest);
}
static void mips_gen_load(mips_encoder *const enc, const lvalue *const src, const rvalue *const dest)
{
	mips_to_code_instr_rl(enc, MIPS_IC_LW, dest, src);
}
static void mips_gen_alloca(mips_encoder *const enc, const size_t size, const lvalue *const res)
{
	unimplemented();
}

static void mips_gen_add(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_ADD, dest, lhs, rhs);
}
static void mips_gen_sub(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_SUB, dest, lhs, rhs);
}
static void mips_gen_mul(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_MUL, dest, lhs, rhs);
}
static void mips_gen_div(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_DIV, dest, lhs, rhs);
}

static void mips_gen_mod(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	unimplemented();
}

static void mips_gen_fadd(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_ADD_S, dest, lhs, rhs);
}
static void mips_gen_fsub(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_SUB_S, dest, lhs, rhs);
}
static void mips_gen_fmul(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_MUL_S, dest, lhs, rhs);
}
static void mips_gen_fdiv(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_DIV_S, dest, lhs, rhs);
}

static void mips_gen_and(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_AND, dest, lhs, rhs);
}
static void mips_gen_or(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_OR, dest, lhs, rhs);
}
static void mips_gen_xor(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_XOR, dest, lhs, rhs);
}
static void mips_gen_shl(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_SLL, dest, lhs, rhs);
}
static void mips_gen_shr(mips_encoder *const enc, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const dest)
{
	mips_to_code_instr_rrr(enc, MIPS_IC_SRA, dest, lhs, rhs);
}

static void mips_gen_jmp(mips_encoder *const enc, const label *const label_)
{
	mips_to_code_instr_b(enc, MIPS_IC_J, label_);
}
static void mips_gen_jmpz(mips_encoder *const enc, const label *const label_, const rvalue *const value)
{
	mips_to_code_instr_rb(enc, MIPS_IC_BEQZ, value, label_);
}
static void mips_gen_jmpnz(mips_encoder *const enc, const label *const label_, const rvalue *const value)
{
	mips_to_code_instr_rb(enc, MIPS_IC_BNEZ, value, label_);
}
static void mips_gen_jmpeq(mips_encoder *const enc, const label *const label_, const rvalue *const lhs, const rvalue *const rhs)
{
	mips_to_code_instr_rrb(enc, MIPS_IC_BEQ, lhs, rhs, label_);
}
static void mips_gen_jmplt(mips_encoder *const enc, const label *const label_, const rvalue *const lhs, const rvalue *const rhs)
{
	mips_to_code_instr_rrb(enc, MIPS_IC_BLT, lhs, rhs, label_);
}
static void mips_gen_jmple(mips_encoder *const enc, const label *const label_, const rvalue *const lhs, const rvalue *const rhs)
{
	mips_to_code_instr_rrb(enc, MIPS_IC_BLE, lhs, rhs, label_);
}

static void mips_gen_ftoi(mips_encoder *const enc, const rvalue *const src, const rvalue *const dest)
{
	unimplemented();
}
static void mips_gen_itof(mips_encoder *const enc, const rvalue *const src, const rvalue *const dest)
{
	unimplemented();
}

static void mips_gen_param(mips_encoder *const enc, const rvalue *const value)
{
	unimplemented();
}
static void mips_gen_call(mips_encoder *const enc, const item_t function, const rvalue *const res)
{
	unimplemented();
}
static void mips_gen_ret(mips_encoder *const enc, const rvalue *const value)
{
	unimplemented();
}

static void mips_gen_extern(mips_encoder *const enc, const item_t id, const item_t type)
{
	(void) type;
	//mips_to_code_extern(enc, id);
	unimplemented();
}
static void mips_gen_global(mips_encoder *const enc, const item_t id, const item_t type)
{
	(void) type;
	//mips_to_code_extern(enc, id);
	unimplemented();
}
static void mips_gen_funciton_begin(mips_encoder *const enc, const item_t id, const item_t type)
{

	unimplemented();
}
static void mips_gen_funciton_end(mips_encoder *const enc, const item_t id, const item_t type)
{
	unimplemented();
}

static void mips_gen_begin(mips_encoder *const enc)
{
	// Подпись "GNU As:" для директив GNU
	// Подпись "MIPS Assembler:" для директив ассемблера MIPS

	mips_printf(enc, "\t.section .mdebug.abi32\n");	// ?
	mips_printf(enc, "\t.previous\n");				// следующая инструкция будет перенесена в секцию, описанную выше
	mips_printf(enc, "\t.nan\tlegacy\n");				// ?
	mips_printf(enc, "\t.module fp=xx\n");			// ?
	mips_printf(enc, "\t.module nooddspreg\n");		// ?
	mips_printf(enc, "\t.abicalls\n");				// ?
	mips_printf(enc, "\t.option pic0\n");				// как если бы при компиляции была включена опция "-fpic" (что означает?)
	mips_printf(enc, "\t.text\n");					// последующий код будет перенесён в текстовый сегмент памяти
	// выравнивание последующих данных / команд по границе, кратной 2^n байт (в данном случае 2^2 = 4)
	mips_printf(enc, "\t.align 2\n");

	// делает метку main глобальной -- её можно вызывать извне кода (например, используется при линковке)
	mips_printf(enc, "\n\t.globl\tmain\n");
	mips_printf(enc, "\t.ent\tmain\n");				// начало процедуры main
	mips_printf(enc, "\t.type\tmain, @function\n");	// тип "main" -- функция
	mips_printf(enc, "main:\n");

	// инициализация gp
	// "__gnu_local_gp" -- локация в памяти, где лежит Global Pointer
	mips_printf(enc, "\tlui $gp, %%hi(__gnu_local_gp)\n");
	mips_printf(enc, "\taddiu $gp, $gp, %%lo(__gnu_local_gp)\n");

	// FIXME: сделать для $ra, $sp и $fp отдельные глобальные rvalue
	mips_printf(enc, "\tmove $fp, $sp\n");
	mips_printf(enc, "\taddi $sp, $sp, -4\n");
	mips_printf(enc, "\tsw $ra, 0($sp)\n");
	mips_printf(enc, "\tli $t0, 268500992\n");
	mips_printf(enc, "\tsw $t0, -8060($gp)\n");
	mips_printf(enc, "\n");

	mips_printf(enc, "\t.rdata\n");
	mips_printf(enc, "\t.align 2\n");
	mips_printf(enc, "\t.text\n");
	mips_printf(enc, "\t.align 2\n");

	mips_printf(enc, "\tjal MAIN\n");
	mips_printf(enc, "\tlw $ra, 0($sp)\n");
	mips_printf(enc, "\tjr $ra\n");
}
static void mips_gen_end(mips_encoder *const enc)
{
	mips_printf(enc, "\t\n");
	mips_printf(enc, "\t\n");
	mips_printf(enc, "\t\n");
	mips_printf(enc, "\t\n");
	mips_printf(enc, "\t\n");
	mips_printf(enc, "\t\n");

	mips_printf(enc, "# defarr\n");
	mips_printf(enc, "# объявление одномерного массива\n");
	mips_printf(enc, "# $a0 -- адрес первого элемента\n");
	mips_printf(enc, "# $a1 -- размер измерения\n");
	mips_printf(enc, "DEFARR1:\n");
	mips_printf(enc, "\tsw $a1, 4($a0)\t\t\t# Сохранение границы\n");
	mips_printf(enc, "\tli $v0, 4\t\t\t\t# Загрузка размера слова\n");
	mips_printf(enc, "\tmul $v0, $v0, $a1\t\t# Подсчёт размера первого измерения массива в байтах\n");
	mips_printf(enc, "\tsub $v0, $a0, $v0\t\t# Считаем адрес после конца массива, т.е. $v0 -- на слово ниже последнего элемента\n");
	mips_printf(enc, "\taddi $v0, $v0, -4\n");
	mips_printf(enc, "\tjr $ra\n");

	mips_printf(enc, "# объявление многомерного массива, но сначала обязана вызываться процедура DEFARR1\n");
	mips_printf(enc, "# $a0 -- адрес первого элемента\n");
	mips_printf(enc, "# $a1 -- размер измерения\n");
	mips_printf(enc, "# $a2 -- адрес первого элемента предыдущего измерения\n");
	mips_printf(enc, "# $a3 -- размер предыдущего измерения\n");
	mips_printf(enc, "DEFARR2:\n");
	mips_printf(enc, "\tsw $a0, 0($a2)\t\t\t# Сохраняем адрес в элементе предыдущего измерения\n");
	mips_printf(enc, "\tmove $t0, $ra\t\t\t# Запоминаем $ra, чтобы он не затёрся\n");
	mips_printf(enc, "\tjal DEFARR1\t\t\t\t# Выделение памяти под массив\n");
	mips_printf(enc, "\tmove $ra, $t0\t\t\t# Восстанавливаем $ra\n");
	mips_printf(enc, "\taddi $a2, $a2, -4\t\t# В $a2 следующий элемент в предыдущем измерении\n");
	mips_printf(enc, "\taddi $a0, $v0, -4\t\t# В $a0 первый элемент массива в текущем измерении, плюс выделяется место под размеры\n");
	mips_printf(enc, "\taddi $a3, $a3, -1\t\t# Уменьшаем счётчик\n");
	mips_printf(enc, "\tbne $a3, $0, DEFARR2\t# Прыгаем, если ещё не всё выделили\n");
	mips_printf(enc, "\tjr $ra\n");


	mips_printf(enc, "\t.end\tmain\n");
	mips_printf(enc, "\t.size\tmain, .-main\n");
}


// ir_gen_rl_instr_func mips_gen_store;
// ir_gen_lr_instr_func mips_gen_load;
// ir_gen_sl_instr_func mips_gen_alloca;

// ir_gen_rrr_instr_func mips_gen_add;
// ir_gen_rrr_instr_func mips_gen_sub;
// ir_gen_rrr_instr_func mips_gen_mul;
// ir_gen_rrr_instr_func mips_gen_div;

// ir_gen_rrr_instr_func mips_gen_mod;

// ir_gen_rrr_instr_func mips_gen_and;
// ir_gen_rrr_instr_func mips_gen_or;
// ir_gen_rrr_instr_func mips_gen_xor;
// ir_gen_rrr_instr_func mips_gen_shl;
// ir_gen_rrr_instr_func mips_gen_shr;

// ir_gen_rrr_instr_func mips_gen_fadd;
// ir_gen_rrr_instr_func mips_gen_fsub;
// ir_gen_rrr_instr_func mips_gen_fmul;
// ir_gen_rrr_instr_func mips_gen_fdiv;

// ir_gen_rr_instr_func mips_gen_ftoi;
// ir_gen_rr_instr_func mips_gen_itof;

// ir_gen_rn_instr_func mips_gen_param;
// ir_gen_fr_instr_func mips_gen_call;
// ir_gen_rn_instr_func mips_gen_ret;

// ir_gen_bn_instr_func mips_gen_jmp;
// ir_gen_brn_instr_func mips_gen_jmpz;
// ir_gen_brn_instr_func mips_gen_jmpnz;

// ir_gen_extern_func mips_gen_extern;
// ir_gen_global_func mips_gen_global;
// ir_gen_function_func mips_gen_function_begin;
// ir_gen_function_func mips_gen_function_end;

int encode_to_mips(const workspace *const ws, syntax *const sx)
{
	const node root = node_get_root(&sx->tree);

	ir_module module = create_ir_module();
	ir_builder builder = create_ir_builder(&module, sx);

	ir_emit_module(&builder, &root);
	ir_dump(&builder);

	mips_encoder enc = create_mips_encoder(sx);

	ir_gens gens = (ir_gens) {
		.gen_begin = (ir_gen_general_func) &mips_gen_begin,
		.gen_end = (ir_gen_general_func) &mips_gen_end,

		.gen_label = (ir_gen_bn_instr_func) &mips_gen_label,

		.gen_move = (ir_gen_rrn_instr_func) &mips_gen_move,
		.gen_store = (ir_gen_rl_instr_func) &mips_gen_store,
		.gen_load = (ir_gen_lr_instr_func) &mips_gen_load,
		.gen_alloca = (ir_gen_sl_instr_func) &mips_gen_alloca,

		.gen_add = (ir_gen_rrr_instr_func) &mips_gen_add,
		.gen_sub = (ir_gen_rrr_instr_func) &mips_gen_sub,
		.gen_mul = (ir_gen_rrr_instr_func) &mips_gen_mul,
		.gen_div = (ir_gen_rrr_instr_func) &mips_gen_div,

		.gen_fadd = (ir_gen_rrr_instr_func) &mips_gen_fadd,
		.gen_fsub = (ir_gen_rrr_instr_func) &mips_gen_fsub,
		.gen_fmul = (ir_gen_rrr_instr_func) &mips_gen_fmul,
		.gen_fdiv = (ir_gen_rrr_instr_func) &mips_gen_fdiv,

		.gen_and = (ir_gen_rrr_instr_func) &mips_gen_and,
		.gen_or = (ir_gen_rrr_instr_func) &mips_gen_or,
		.gen_xor = (ir_gen_rrr_instr_func) &mips_gen_xor,
		.gen_shl = (ir_gen_rrr_instr_func) &mips_gen_shl,
		.gen_shr = (ir_gen_rrr_instr_func) &mips_gen_shr,

		.gen_jmp = (ir_gen_bn_instr_func) &mips_gen_jmp,
		.gen_jmpz = (ir_gen_brn_instr_func) &mips_gen_jmpz,
		.gen_jmpnz = (ir_gen_brn_instr_func) &mips_gen_jmpnz,
		.gen_jmpeq = (ir_gen_brrn_instr_func) &mips_gen_jmpeq,
		.gen_jmplt = (ir_gen_brrn_instr_func) &mips_gen_jmplt,
		.gen_jmple = (ir_gen_brrn_instr_func) &mips_gen_jmple,

		.gen_extern = (ir_gen_extern_func) &mips_gen_extern,
		.gen_global = (ir_gen_global_func) &mips_gen_global,
		.gen_function_begin = (ir_gen_function_func) &mips_gen_funciton_begin,
		.gen_function_end = (ir_gen_function_func) &mips_gen_funciton_end,


		.gen_mod = (ir_gen_rrr_instr_func) &mips_gen_mod,

		.gen_ftoi = (ir_gen_rr_instr_func) &mips_gen_ftoi,
		.gen_itof = (ir_gen_rr_instr_func) &mips_gen_itof,

		.gen_param = (ir_gen_rn_instr_func) &mips_gen_param,
		.gen_call = (ir_gen_fr_instr_func) &mips_gen_call,
		.gen_ret = (ir_gen_rn_instr_func) &mips_gen_ret,
	};

	ir_gen_module(&module, &gens, &enc);

	return 0;
}
