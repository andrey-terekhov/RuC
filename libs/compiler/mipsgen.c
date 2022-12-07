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
#include "operations.h"
#include "tree.h"
#include "uniprinter.h"


#ifndef max
	#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


static const size_t BUFFER_SIZE = 65536; /**< Размер буфера для тела функции */
static const size_t HASH_TABLE_SIZE = 1024; /**< Размер хеш-таблицы для смещений и регистров */
static const bool IS_ON_STACK = true; /**< Хранится ли переменная на стеке */

static const size_t WORD_LENGTH = 4;	  /**< Длина слова данных */
static const size_t HALF_WORD_LENGTH = 2; /**< Длина половины слова данных */

static const size_t LOW_DYN_BORDER = 0x10010000; /**< Нижняя граница динамической памяти */
static const size_t HEAP_DISPL = 8000; /**< Смещение кучи относительно глобальной памяти */

static const size_t SP_SIZE = 4; /**< Размер регистра $sp для его сохранения */
static const size_t RA_SIZE = 4; /**< Размер регистра $ra для его сохранения */

static const size_t TEMP_FP_REG_AMOUNT = 12; /**< Количество временных регистров
												 для чисел с плавающей точкой */
static const size_t TEMP_REG_AMOUNT = 10; /**< Количество обычных временных регистров */
static const size_t ARG_REG_AMOUNT = 4; /**< Количество регистров-аргументов для функций */

static const size_t PRESERVED_REG_AMOUNT = 8; /**< Количество сохраняемых регистров общего назначения */
static const size_t PRESERVED_FP_REG_AMOUNT = 10; /**< Количество сохраняемых регистров с плавающей точкой */

static const bool FROM_LVALUE = 1; /**< Получен ли rvalue из lvalue */

/**< Смещение в стеке для сохранения оберегаемых регистров,
	 без учёта оптимизаций */
static const size_t FUNC_DISPL_PRESEREVED = /* за $sp */ 4 + /* за $ra */ 4 +
											/* fs0-fs10 (одинарная точность): */ 5 * 4 + /* s0-s7: */ 8 * 4 +
											/* a0-a3: */ 4 * 4;

// Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition
typedef enum MIPS_REGISTER
{
	R_ZERO, /**< Always has the value 0 */
	R_AT,	/**< Temporary, generally used by assembler */

	R_V0,
	R_V1, /**< Used for expression evaluations and to hold the integer
			  and pointer type function return values */

	R_A0,
	R_A1,
	R_A2,
	R_A3, /**< Used for passing arguments to functions; values are not
			  preserved across function calls */

	R_T0,
	R_T1,
	R_T2,
	R_T3,
	R_T4,
	R_T5,
	R_T6,
	R_T7, /**< Temporary registers used for expression evaluation;
			  values are not preserved across function calls */

	R_S0,
	R_S1,
	R_S2,
	R_S3,
	R_S4,
	R_S5,
	R_S6,
	R_S7, /**< Saved registers; values are preserved across function calls */

	R_T8,
	R_T9, /**< Temporary registers used for expression evaluations;
			  values are not preserved across function calls.  When
			  calling position independent functions $25 (R_T9) must contain
			  the address of the called function */

	R_K0,
	R_K1, /**< Used only by the operating system */

	R_GP, /**< Global pointer and context pointer */
	R_SP, /**< Stack pointer */
	R_FP, /**< Saved register (like s0-s7) or frame pointer */
	R_RA, /**< Return address. The return address is the location to
			  which a function should return control */

	// Регистры для работы с числами с плавающей точкой
	// Для чисел с двойной точностью используется пара регистров:
	// - регистр с чётным номером содержит младшие 32 бита числа;
	// - регистр с нечётным номером содержит старшие 32 бита числа.
	R_FV0,
	R_FV1,
	R_FV2,
	R_FV3, /**< used to hold floating-point type function results;
			   single-precision uses $f0 and double-precision uses
			   the register pair $f0..$f1 */

	R_FA0,
	R_FA1,
	R_FA2,
	R_FA3, /**< Used for passing arguments to functions */

	R_FT0,
	R_FT1,
	R_FT2,
	R_FT3,
	R_FT4,
	R_FT5,
	R_FT6,
	R_FT7,
	R_FT8,
	R_FT9,
	R_FT10,
	R_FT11, /**< Temporary registers */

	R_FS0,
	R_FS1,
	R_FS2,
	R_FS3,
	R_FS4,
	R_FS5,
	R_FS6,
	R_FS7,
	R_FS8,
	R_FS9,
	R_FS10,
	R_FS11 /**< Saved registers; their values are preserved across function calls */
} mips_register_t;


// Назначение команд взято из документации MIPS® Architecture for Programmers
// Volume II-A: The MIPS32® Instruction
// Set Manual 2016
typedef enum INSTRUCTION
{
	IC_MIPS_MOVE, /**< MIPS Pseudo-Instruction. Move the contents of one register to another */
	IC_MIPS_LI,	  /**< MIPS Pseudo-Instruction. Load a constant into a register */
	IC_MIPS_NOT,  /**< MIPS Pseudo-Instruction. Flips the bits of the source register and
					  stores them in the destination register (не из вышеуказанной книги) */

	IC_MIPS_ADDI, /**< To add a constant to a 32-bit integer. If overflow occurs, then trap */
	IC_MIPS_SLL,  /**< To left-shift a word by a fixed number of bits */
	IC_MIPS_SRA,  /**< To execute an arithmetic right-shift of a word by a fixed number of bits */
	IC_MIPS_ANDI, /**< To do a bitwise logical AND with a constant */
	IC_MIPS_XORI, /**< To do a bitwise logical Exclusive OR with a constant */
	IC_MIPS_ORI,  /**< To do a bitwise logical OR with a constant */

	IC_MIPS_ADD,  /**< To add 32-bit integers. If an overflow occurs, then trap */
	IC_MIPS_SUB,  /**< To subtract 32-bit integers. If overflow occurs, then trap */
	IC_MIPS_MUL,  /**< To multiply two words and write the result to a GPR */
	IC_MIPS_DIV,  /**< DIV performs a signed 32-bit integer division, and places
					   the 32-bit quotient result in the destination register */
	IC_MIPS_MOD,  /**< MOD performs a signed 32-bit integer division, and places
					   the 32-bit remainder result in the destination register.
					   The remainder result has the same sign as the dividend */
	IC_MIPS_SLLV, /**< To left-shift a word by a variable number of bits */
	IC_MIPS_SRAV, /**< To execute an arithmetic right-shift of a word by a variable number of bits */
	IC_MIPS_AND,  /**< To do a bitwise logical AND */
	IC_MIPS_XOR,  /**< To do a bitwise logical Exclusive OR */
	IC_MIPS_OR,	  /**< To do a bitwise logical OR */

	IC_MIPS_SW, /**< To store a word to memory */
	IC_MIPS_LW, /**< To load a word from memory as a signed value */

	IC_MIPS_JR,	 /**< To execute a branch to an instruction address in a register */
	IC_MIPS_JAL, /**< To execute a procedure call within the current 256MB-aligned region */
	IC_MIPS_J,	 /**< To branch within the current 256 MB-aligned region */

	IC_MIPS_BLEZ, /**< Branch on Less Than or Equal to Zero.
					  To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BLTZ, /**< Branch on Less Than Zero.
					  To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BGEZ, /**< Branch on Greater Than or Equal to Zero.
					  To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BGTZ, /**< Branch on Greater Than Zero.
					  To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BEQ,  /**< Branch on Equal.
					  To compare GPRs then do a PC-relative conditional branch */
	IC_MIPS_BNE,  /**< Branch on Not Equal.
					  To compare GPRs then do a PC-relative conditional branch */

	IC_MIPS_LA, /**< Load the address of a named memory
					location into a register (не из вышеуказанной книги)*/

	IC_MIPS_SLTIU, /**< Set on Less Than Immediate Unsigned.
						To record the result of an unsigned less-than comparison with a constant. */

	IC_MIPS_NOP, /**<To perform no operation */

	/** Floating point operations. Single precision. */
	IC_MIPS_ADD_S, /**< To add FP values. */
	IC_MIPS_SUB_S, /**< To subtract FP values. */
	IC_MIPS_MUL_S, /**< To multiply FP values. */
	IC_MIPS_DIV_S, /**< To divide FP values. */

	IC_MIPS_ABS_S,	/**< Floating Point Absolute Value*/
	IC_MIPS_ABS,	/**< GPR absolute value (не из вышеуказанной книги). MIPS Pseudo-Instruction. */

	IC_MIPS_S_S, /**< MIPS Pseudo instruction. To store a doubleword from an FPR to memory. */
	IC_MIPS_L_S, /**< MIPS Pseudo instruction. To load a doubleword from memory to an FPR. */

	IC_MIPS_LI_S, /**< MIPS Pseudo-Instruction. Load a FP constant into a FPR. */

	IC_MIPS_MOV_S, /**< The value in first FPR is placed into second FPR. */

	IC_MIPS_MFC_1,	/**< Move word from Floating Point.
						To copy a word from an FPU (CP1) general register to a GPR. */
	IC_MIPS_MFHC_1, /**< To copy a word from the high half of an FPU (CP1)
						general register to a GPR. */

	IC_MIPS_CVT_D_S, /**< To convert an FP value to double FP. */
	IC_MIPS_CVT_S_W, /**< To convert fixed point value to single FP. */
	IC_MIPS_CVT_W_S, /**< To convert single FP to fixed point value */
} mips_instruction_t;


typedef enum LABEL
{
	L_MAIN,			/**< Тип метки -- главная функция */
	L_FUNC,			/**< Тип метки -- вход в функцию */
	L_NEXT,			/**< Тип метки -- следующая функция */
	L_FUNCEND,		/**< Тип метки -- выход из функции */
	L_STRING,		/**< Тип метки -- строка */
	L_ELSE,			/**< Тип метки -- переход по else */
	L_END,			/**< Тип метки -- переход в конец конструкции */
	L_BEGIN_CYCLE,	/**< Тип метки -- переход в начало цикла */
} mips_label_t;

typedef struct label
{
	mips_label_t kind;
	size_t num;
} label;

typedef struct encoder
{
	syntax *sx; /**< Структура syntax с таблицами */

	size_t max_displ;	 /**< Максимальное смещение от $sp */
	size_t global_displ; /**< Смещение от $gp */

	hash displacements; /**< Хеш таблица с информацией о расположении идентификаторов:
							@c key		 - ссылка на таблицу идентификаторов
							@c value[0]	 - флаг, лежит ли переменная на стеке или в регистре
							@c value[1]  - смещение или номер регистра */

	mips_register_t next_register; /**< Следующий обычный регистр для выделения */
	mips_register_t next_float_register; /**< Следующий регистр с плавающей точкой для выделения */

	size_t label_num;						/**< Номер метки */
	label label_else;						/**< Метка перехода на else */
	label label_continue;					/**< Метка continue */
	label label_break;						/**< Метка break */
	size_t curr_function_ident; 			/**< Идентификатор текущей функций */

	bool registers[22]; 					/**< Информация о занятых регистрах */

	size_t scope_displ;						/**< Смещение */
} encoder;

/** Kinds of lvalue */
typedef enum LVALUE_KIND
{
	LVALUE_KIND_STACK,
	LVALUE_KIND_REGISTER,
} lvalue_kind_t;

typedef struct lvalue
{
	const lvalue_kind_t kind;		  /**< Value kind */
	const mips_register_t base_reg; /**< Base register */
	const union					  /**< Value location */
	{
		item_t reg_num; 				/**< Register where the value is stored */
		item_t displ;	/**< Stack displacement where the value is stored */
	} loc;
	const item_t type; /**< Value type */
} lvalue;

/** Kinds of rvalue */
typedef enum RVALUE_KIND
{
	RVALUE_KIND_CONST, // Значит, запомнили константу и потом обработали её
	RVALUE_KIND_REGISTER,
	RVALUE_KIND_VOID,
} rvalue_kind_t;

typedef struct rvalue
{
	const rvalue_kind_t kind; /**< Value kind */
	const item_t type;		/**< Value type */
	const bool from_lvalue;	/**< Was the rvalue instance formed from lvalue */
	const union
	{
		item_t reg_num;	  /**< Where the value is stored */
		item_t int_val;	  /**< Value of integer (character, boolean) literal */
		double float_val; /**< Value of floating literal */
		item_t str_index; /**< Index of pre-declared string */
	} val;
} rvalue;

static const rvalue RVALUE_ONE = { .kind = RVALUE_KIND_CONST, .type = TYPE_INTEGER, .val.int_val = 1 };
static const rvalue RVALUE_NEGATIVE_ONE = { .kind = RVALUE_KIND_CONST, .type = TYPE_INTEGER, .val.int_val = -1 };
static const rvalue RVALUE_ZERO = { .kind = RVALUE_KIND_CONST, .type = TYPE_INTEGER, .val.int_val = 0 };
static const rvalue RVALUE_VOID = { .kind = RVALUE_KIND_CONST };


static lvalue emit_lvalue(encoder *const enc, const node *const nd);
static void emit_binary_operation(encoder *const enc, const rvalue *const dest
	, const rvalue *const first_operand, const rvalue *const second_operand, const binary_t operator);
static rvalue emit_expression(encoder *const enc, const node *const nd);
static rvalue emit_void_expression(encoder *const enc, const node *const nd);
static void emit_statement(encoder *const enc, const node *const nd);


static size_t mips_type_size(const syntax *const sx, const item_t type)
{
	if (type_is_structure(sx, type))
	{
		size_t size = 0;
		const size_t amount = type_structure_get_member_amount(sx, type);
		for (size_t i = 0; i < amount; i++)
		{
			const item_t member_type = type_structure_get_member_type(sx, type, i);
			size += mips_type_size(sx, member_type);
		}
		return size;
	}
	else
	{
		return WORD_LENGTH;
	}
}

/**
 * Locks certain register
 * 
 * @param	enc					Encoder
 * @param	reg					Register to lock
*/
static void lock_register(encoder *const enc, const mips_register_t reg)
{
	switch (reg)
	{
		case R_T0:
		case R_T1:
		case R_T2:
		case R_T3:
		case R_T4:
		case R_T5:
		case R_T6:
		case R_T7:
			if (!enc->registers[reg - R_T0])
			{
				// Регистр занят => освобождаем
				enc->registers[reg - R_T0] = true;
			}
			return;

		case R_T8:
		case R_T9:
			if (!enc->registers[reg - R_T8 + /* индекс R_T8 в enc->registers */ 8])
			{
				enc->registers[reg - R_T8 + 8] = true;
			}
			return;

		case R_FT0:
		case R_FT1:
		case R_FT2:
		case R_FT3:
		case R_FT4:
		case R_FT5:
		case R_FT6:
		case R_FT7:
		case R_FT8:
		case R_FT9:
		case R_FT10:
		case R_FT11:
			if (!enc->registers[reg - R_FT0 + /* индекс R_FT0 в enc->registers */ TEMP_REG_AMOUNT])
			{
				enc->registers[reg - R_FT0 + TEMP_REG_AMOUNT] = true;
			}
			return;

		default: // Не временный регистр и пришли сюда => и так захвачен
			return;
	}
}

/**
 * Takes the first free register
 *
 * @param	enc					Encoder
 *
 * @return	General purpose register
 */
static mips_register_t get_register(encoder *const enc)
{
	// Ищем первый свободный регистр
	mips_register_t i = 0;
	while ((i < TEMP_REG_AMOUNT) && (enc->registers[i]))
	{
		i++;
	}

	assert(i != TEMP_REG_AMOUNT);

	// Занимаем его
	enc->registers[i] = true;

	return i + R_T0;
}

/**
 * Takes the first free floating point register
 *
 * @param	enc					Encoder
 *
 * @return	Register			Floating point register
 */
static mips_register_t get_float_register(encoder *const enc)
{
	// Ищем первый свободный регистр
	mips_register_t i = TEMP_REG_AMOUNT;
	while ((i < TEMP_FP_REG_AMOUNT + TEMP_REG_AMOUNT) && (enc->registers[i]))
	{
		i += 2; /* т.к. операции с одинарной точностью */
	}

	assert(i != TEMP_FP_REG_AMOUNT + TEMP_REG_AMOUNT);

	// Занимаем его
	enc->registers[i] = true;

	return i + R_FT0 - /* за индекс R_FT0 в enc->registers */ TEMP_REG_AMOUNT;
}

/**
 * Free register
 * 
 * @param	enc					Encoder
 * @param	reg					Register to set as free
*/
static void free_register(encoder *const enc, const mips_register_t reg)
{
	switch (reg)
	{
		case R_T0:
		case R_T1:
		case R_T2:
		case R_T3:
		case R_T4:
		case R_T5:
		case R_T6:
		case R_T7:
			if (enc->registers[reg - R_T0])
			{
				// Регистр занят => освобождаем
				enc->registers[reg - R_T0] = false;
			}
			return;

		case R_T8:
		case R_T9:
			if (enc->registers[reg - R_T8 + /* индекс R_T8 в enc->registers */ 8])
			{
				// Регистр занят => освобождаем
				enc->registers[reg - R_T8 + 8] = false;
			}
			return;

		case R_FT0:
		case R_FT1:
		case R_FT2:
		case R_FT3:
		case R_FT4:
		case R_FT5:
		case R_FT6:
		case R_FT7:
		case R_FT8:
		case R_FT9:
		case R_FT10:
		case R_FT11:
			if (enc->registers[reg - R_FT0 + /* индекс R_FT0 в enc->registers */ TEMP_REG_AMOUNT])
			{
				// Регистр занят => освобождаем
				enc->registers[reg - R_FT0 + TEMP_REG_AMOUNT] = false;
			}
			return;

		default: // Не временный регистр => освобождать не надо
			return;
	}
}

/**
 * Free register occupied by rvalue
 *
 * @param	enc					Encoder
 * @param	rval				Rvalue to be freed
 */
static void free_rvalue(encoder *const enc, const rvalue *const rval)
{
	if ((rval->kind == RVALUE_KIND_REGISTER) && (!rval->from_lvalue))
	{
		free_register(enc, rval->val.reg_num);
	}
}

/** Get MIPS assembler binary instruction from binary_t type
 *
 * @param	operation_type		Type of operation in AST
 * @param	is_imm				@c True if the instruction is immediate, @c False otherwise
 *
 * @return	MIPS binary instruction
 */
static mips_instruction_t get_bin_instruction(const binary_t operation_type, const bool is_imm)
{
	switch (operation_type)
	{
		case BIN_ADD_ASSIGN:
		case BIN_ADD:
			return (is_imm) ? IC_MIPS_ADDI : IC_MIPS_ADD;

		case BIN_SUB_ASSIGN:
		case BIN_SUB:
			return (is_imm) ? IC_MIPS_ADDI : IC_MIPS_SUB;

		case BIN_MUL_ASSIGN:
		case BIN_MUL:
			return IC_MIPS_MUL;

		case BIN_DIV_ASSIGN:
		case BIN_DIV:
			return IC_MIPS_DIV;

		case BIN_REM_ASSIGN:
		case BIN_REM:
			return IC_MIPS_MOD;

		case BIN_SHL_ASSIGN:
		case BIN_SHL:
			return (is_imm) ? IC_MIPS_SLL : IC_MIPS_SLLV;

		case BIN_SHR_ASSIGN:
		case BIN_SHR:
			return (is_imm) ? IC_MIPS_SRA : IC_MIPS_SRAV;

		case BIN_AND_ASSIGN:
		case BIN_AND:
			return (is_imm) ? IC_MIPS_ANDI : IC_MIPS_AND;

		case BIN_XOR_ASSIGN:
		case BIN_XOR:
			return (is_imm) ? IC_MIPS_XORI : IC_MIPS_XOR;

		case BIN_OR_ASSIGN:
		case BIN_OR:
			return (is_imm) ? IC_MIPS_ORI : IC_MIPS_OR;

		case BIN_EQ:
			return IC_MIPS_BEQ;
		case BIN_NE:
			return IC_MIPS_BNE;
		case BIN_GT:
			return IC_MIPS_BGTZ;
		case BIN_LT:
			return IC_MIPS_BLTZ;
		case BIN_GE:
			return IC_MIPS_BGEZ;
		case BIN_LE:
			return IC_MIPS_BLEZ;

		default:
			return IC_MIPS_NOP;
	}
}

static void mips_register_to_io(universal_io *const io, const mips_register_t reg)
{
	switch (reg)
	{
		case R_ZERO:
			uni_printf(io, "$0");
			break;
		case R_AT:
			uni_printf(io, "$at");
			break;

		case R_V0:
			uni_printf(io, "$v0");
			break;
		case R_V1:
			uni_printf(io, "$v1");
			break;

		case R_A0:
			uni_printf(io, "$a0");
			break;
		case R_A1:
			uni_printf(io, "$a1");
			break;
		case R_A2:
			uni_printf(io, "$a2");
			break;
		case R_A3:
			uni_printf(io, "$a3");
			break;

		case R_T0:
			uni_printf(io, "$t0");
			break;
		case R_T1:
			uni_printf(io, "$t1");
			break;
		case R_T2:
			uni_printf(io, "$t2");
			break;
		case R_T3:
			uni_printf(io, "$t3");
			break;
		case R_T4:
			uni_printf(io, "$t4");
			break;
		case R_T5:
			uni_printf(io, "$t5");
			break;
		case R_T6:
			uni_printf(io, "$t6");
			break;
		case R_T7:
			uni_printf(io, "$t7");
			break;

		case R_S0:
			uni_printf(io, "$s0");
			break;
		case R_S1:
			uni_printf(io, "$s1");
			break;
		case R_S2:
			uni_printf(io, "$s2");
			break;
		case R_S3:
			uni_printf(io, "$s3");
			break;
		case R_S4:
			uni_printf(io, "$s4");
			break;
		case R_S5:
			uni_printf(io, "$s5");
			break;
		case R_S6:
			uni_printf(io, "$s6");
			break;
		case R_S7:
			uni_printf(io, "$s7");
			break;

		case R_T8:
			uni_printf(io, "$t8");
			break;
		case R_T9:
			uni_printf(io, "$t9");
			break;

		case R_K0:
			uni_printf(io, "$k0");
			break;
		case R_K1:
			uni_printf(io, "$k1");
			break;

		case R_GP:
			uni_printf(io, "$gp");
			break;
		case R_SP:
			uni_printf(io, "$sp");
			break;
		case R_FP:
			uni_printf(io, "$fp");
			break;
		case R_RA:
			uni_printf(io, "$ra");
			break;

		case R_FV0:
			uni_printf(io, "$f0");
			break;
		case R_FV1:
			uni_printf(io, "$f1");
			break;
		case R_FV2:
			uni_printf(io, "$f2");
			break;
		case R_FV3:
			uni_printf(io, "$f3");
			break;

		case R_FT0:
			uni_printf(io, "$f4");
			break;
		case R_FT1:
			uni_printf(io, "$f5");
			break;
		case R_FT2:
			uni_printf(io, "$f6");
			break;
		case R_FT3:
			uni_printf(io, "$f7");
			break;
		case R_FT4:
			uni_printf(io, "$f8");
			break;
		case R_FT5:
			uni_printf(io, "$f9");
			break;
		case R_FT6:
			uni_printf(io, "$f10");
			break;
		case R_FT7:
			uni_printf(io, "$f11");
			break;
		case R_FT8:
			uni_printf(io, "$f16");
			break;
		case R_FT9:
			uni_printf(io, "$f17");
			break;
		case R_FT10:
			uni_printf(io, "$f18");
			break;
		case R_FT11:
			uni_printf(io, "$f19");
			break;

		case R_FA0:
			uni_printf(io, "$f12");
			break;
		case R_FA1:
			uni_printf(io, "$f13");
			break;
		case R_FA2:
			uni_printf(io, "$f14");
			break;
		case R_FA3:
			uni_printf(io, "$f15");
			break;

		case R_FS0:
			uni_printf(io, "$f20");
			break;
		case R_FS1:
			uni_printf(io, "$f21");
			break;
		case R_FS2:
			uni_printf(io, "$f22");
			break;
		case R_FS3:
			uni_printf(io, "$f23");
			break;
		case R_FS4:
			uni_printf(io, "$f24");
			break;
		case R_FS5:
			uni_printf(io, "$f25");
			break;
		case R_FS6:
			uni_printf(io, "$f26");
			break;
		case R_FS7:
			uni_printf(io, "$f27");
			break;
		case R_FS8:
			uni_printf(io, "$f28");
			break;
		case R_FS9:
			uni_printf(io, "$f29");
			break;
		case R_FS10:
			uni_printf(io, "$f30");
			break;
		case R_FS11:
			uni_printf(io, "$f31");
			break;
	}
}

static void instruction_to_io(universal_io *const io, const mips_instruction_t instruction)
{
	switch (instruction)
	{
		case IC_MIPS_MOVE:
			uni_printf(io, "move");
			break;
		case IC_MIPS_LI:
			uni_printf(io, "li");
			break;
		case IC_MIPS_LA:
			uni_printf(io, "la");
			break;
		case IC_MIPS_NOT:
			uni_printf(io, "not");
			break;

		case IC_MIPS_ADDI:
			uni_printf(io, "addi");
			break;
		case IC_MIPS_SLL:
			uni_printf(io, "sll");
			break;
		case IC_MIPS_SRA:
			uni_printf(io, "sra");
			break;
		case IC_MIPS_ANDI:
			uni_printf(io, "andi");
			break;
		case IC_MIPS_XORI:
			uni_printf(io, "xori");
			break;
		case IC_MIPS_ORI:
			uni_printf(io, "ori");
			break;

		case IC_MIPS_ADD:
			uni_printf(io, "add");
			break;
		case IC_MIPS_SUB:
			uni_printf(io, "sub");
			break;
		case IC_MIPS_MUL:
			uni_printf(io, "mul");
			break;
		case IC_MIPS_DIV:
			uni_printf(io, "div");
			break;
		case IC_MIPS_MOD:
			uni_printf(io, "mod");
			break;
		case IC_MIPS_SLLV:
			uni_printf(io, "sllv");
			break;
		case IC_MIPS_SRAV:
			uni_printf(io, "srav");
			break;
		case IC_MIPS_AND:
			uni_printf(io, "and");
			break;
		case IC_MIPS_XOR:
			uni_printf(io, "xor");
			break;
		case IC_MIPS_OR:
			uni_printf(io, "or");
			break;

		case IC_MIPS_SW:
			uni_printf(io, "sw");
			break;
		case IC_MIPS_LW:
			uni_printf(io, "lw");
			break;

		case IC_MIPS_JR:
			uni_printf(io, "jr");
			break;
		case IC_MIPS_JAL:
			uni_printf(io, "jal");
			break;
		case IC_MIPS_J:
			uni_printf(io, "j");
			break;

		case IC_MIPS_BLEZ:
			uni_printf(io, "blez");
			break;
		case IC_MIPS_BLTZ:
			uni_printf(io, "bltz");
			break;
		case IC_MIPS_BGEZ:
			uni_printf(io, "bgez");
			break;
		case IC_MIPS_BGTZ:
			uni_printf(io, "bgtz");
			break;
		case IC_MIPS_BEQ:
			uni_printf(io, "beq");
			break;
		case IC_MIPS_BNE:
			uni_printf(io, "bne");
			break;

		case IC_MIPS_SLTIU:
			uni_printf(io, "sltiu");
			break;

		case IC_MIPS_NOP:
			uni_printf(io, "nop");
			break;

		case IC_MIPS_ADD_S:
			uni_printf(io, "add.s");
			break;
		case IC_MIPS_SUB_S:
			uni_printf(io, "sub.s");
			break;
		case IC_MIPS_MUL_S:
			uni_printf(io, "mul.s");
			break;
		case IC_MIPS_DIV_S:
			uni_printf(io, "div.s");
			break;
		
		case IC_MIPS_ABS_S:
			uni_printf(io, "abs.s");
			break;
		case IC_MIPS_ABS:
			uni_printf(io, "abs");
			break;

		case IC_MIPS_S_S:
			uni_printf(io, "s.s");
			break;
		case IC_MIPS_L_S:
			uni_printf(io, "l.s");
			break;

		case IC_MIPS_LI_S:
			uni_printf(io, "li.s");
			break;

		case IC_MIPS_MOV_S:
			uni_printf(io, "mov.s");
			break;

		case IC_MIPS_MFC_1:
			uni_printf(io, "mfc1");
			break;
		case IC_MIPS_MFHC_1:
			uni_printf(io, "mfhc1");
			break;

		case IC_MIPS_CVT_D_S:
			uni_printf(io, "cvt.d.s");
			break;
		case IC_MIPS_CVT_S_W:
			uni_printf(io, "cvt.s.w");
			break;
		case IC_MIPS_CVT_W_S:
			uni_printf(io, "cvt.w.s");
			break;
	}
}

// Вид инструкции:	instr	fst_reg, snd_reg
static void to_code_2R(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t fst_reg, const mips_register_t snd_reg)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, fst_reg);
	uni_printf(io, ", ");
	mips_register_to_io(io, snd_reg);
	uni_printf(io, "\n");
}

// Вид инструкции:	instr	fst_reg, snd_reg, imm
static void to_code_2R_I(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t fst_reg, const mips_register_t snd_reg, const item_t imm)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, fst_reg);
	uni_printf(io, ", ");
	mips_register_to_io(io, snd_reg);
	uni_printf(io, ", %" PRIitem "\n", imm);
}

// Вид инструкции:	instr	fst_reg, imm(snd_reg)
static void to_code_R_I_R(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t fst_reg, const item_t imm, const mips_register_t snd_reg)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, fst_reg);
	uni_printf(io, ", %" PRIitem "(", imm);
	mips_register_to_io(io, snd_reg);
	uni_printf(io, ")\n");
}

// Вид инструкции:	instr	reg, imm
static void to_code_R_I(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg, const item_t imm)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, reg);
	uni_printf(io, ", %" PRIitem "\n", imm);
}

/**
 * Writes "val" field of rvalue structure to io
 *
 * @param	io					Universal i/o (?)
 * @param	rval				Rvalue whose value is to be printed
 */
static void rvalue_const_to_io(universal_io *const io, const rvalue *const rval)
{
	switch (rval->type)
	{
		case TYPE_BOOLEAN:
		case TYPE_CHARACTER:
		case TYPE_INTEGER:
			uni_printf(io, "%" PRIitem, rval->val.int_val);
			break;

		case TYPE_FLOATING:
			uni_printf(io, "%f", rval->val.float_val);
			break;

		default:
			system_error(node_unexpected);
			break;
	}
}

/**
 * Writes rvalue to io
 * 
 * @param	enc					Encoder
 * @param	rval				Rvalue to write
*/
static void rvalue_to_io(encoder *const enc, const rvalue *const rval)
{
	assert(rval->kind != RVALUE_KIND_VOID);

	if (rval->kind == RVALUE_KIND_CONST)
	{
		rvalue_const_to_io(enc->sx->io, rval);
	}
	else
	{
		mips_register_to_io(enc->sx->io, rval->val.reg_num);
	}
}

/**
 * Writes lvalue to io
 * 
 * @param	enc						Encoder
 * @param	lvalue					Lvalue
*/
static void lvalue_to_io(encoder *const enc, const lvalue *const value)
{
	if (value->kind == LVALUE_KIND_REGISTER)
	{
		mips_register_to_io(enc->sx->io, value->loc.reg_num);
	}
	else
	{
		uni_printf(enc->sx->io, "%" PRIitem "(", value->loc.displ);
		mips_register_to_io(enc->sx->io, value->base_reg);
		uni_printf(enc->sx->io, ")\n");
	}
}

/**
 * Add new identifier to displacements table
 * 
 * @param	enc						Encoder
 * @param	identifier				Identifier for adding to the table
 * @param	location				Location of identifier - register, or displacement on stack
 * @param	is_register				@c true, if identifier is register variable, and @c false otherwise
 * 
 * @return	Identifier lvalue
 */
static lvalue displacements_add(encoder *const enc, const size_t identifier
	, const item_t location, const bool is_register)
{
	const size_t displacement = enc->scope_displ;
	const bool is_local = ident_is_local(enc->sx, identifier);
	const mips_register_t base_reg = is_local ? R_FP : R_GP;
	const item_t type = ident_get_type(enc->sx, identifier);

	if ((!is_local) && (is_register)) // Запрет на глобальные регистровые переменные
	{
		// TODO: кидать соответствующую ошибку
		system_error(node_unexpected);
	}

	const size_t index = hash_add(&enc->displacements, identifier, 3);
	hash_set_by_index(&enc->displacements, index, 0, (is_register) ? 1 : 0);
	hash_set_by_index(&enc->displacements, index, 1, location);
	hash_set_by_index(&enc->displacements, index, 2, base_reg);

	if (is_local)
	{
		enc->scope_displ += mips_type_size(enc->sx, type);
		enc->max_displ = max(enc->scope_displ, enc->max_displ);
	}
	else
	{
		enc->global_displ += mips_type_size(enc->sx, type);
	}

	return (lvalue) { .kind = LVALUE_KIND_STACK, .base_reg = base_reg, .loc.displ = displacement, .type = type };
}

/**
 * Return lvalue for the given identifier
 * 
 * @param	enc						Encoder
 * @param	identifier				Identifier in the table
 * 
 * @return	Identifier lvalue
 */
static lvalue displacements_get(encoder *const enc, const size_t identifier)
{
	const bool is_register = (hash_get(&enc->displacements, identifier, 0) == 1);
	const size_t displacement = hash_get(&enc->displacements, identifier, 1);
	const mips_register_t base_reg = hash_get(&enc->displacements, identifier, 2);
	const item_t type = ident_get_type(enc->sx, identifier);

	const lvalue_kind_t kind = (is_register) ? LVALUE_KIND_REGISTER : LVALUE_KIND_STACK;

	return (lvalue) { .kind = kind, .base_reg = base_reg, .loc.displ = displacement, .type = type };
}

/**
 * Emit label
 *
 * @param	enc				Encoder
 * @param	label				Label for emitting
 */
static void emit_label(encoder *const enc, const label *const lbl)
{
	universal_io *const io = enc->sx->io;
	switch (lbl->kind)
	{
		case L_MAIN:
			uni_printf(io, "MAIN");
			break;
		case L_FUNC:
			uni_printf(io, "FUNC");
			break;
		case L_NEXT:
			uni_printf(io, "NEXT");
			break;
		case L_FUNCEND:
			uni_printf(io, "FUNCEND");
			break;
		case L_STRING:
			uni_printf(io, "STRING");
			break;
		case L_ELSE:
			uni_printf(io, "ELSE");
			break;
		case L_END:
			uni_printf(io, "END");
			break;
		case L_BEGIN_CYCLE:
			uni_printf(io, "BEGIN_CYCLE");
			break;
	}

	uni_printf(io, "%" PRIitem, lbl->num);
}

/**
 * Emit label declaration
 *
 * @param	enc				Encoder
 * @param	label			Declared label
 */
static void emit_label_declaration(encoder *const enc, const label *const lbl)
{
	emit_label(enc, lbl);
	uni_printf(enc->sx->io, ":\n");
}

/**
 * Emit unconditional branch
 *
 * @param	enc				Encoder
 * @param	label			Label for unconditional jump
 */
static void emit_unconditional_branch(encoder *const enc, const mips_instruction_t instruction, const label *const lbl)
{
	assert(instruction == IC_MIPS_J || instruction == IC_MIPS_JAL);

	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, instruction);
	uni_printf(enc->sx->io, " ");
	emit_label(enc, lbl);
	uni_printf(enc->sx->io, "\n");
}

/**
 * Emit conditional branch
 *
 * @param	enc				Encoder
 * @param	label			Label for conditional jump
 */
static void emit_conditional_branch(encoder *const enc, const mips_instruction_t instruction
	, const rvalue *const value, const label *const lbl)
{
	if (value->kind == RVALUE_KIND_CONST)
	{
		if (value->val.int_val == 0)
		{
			emit_unconditional_branch(enc, IC_MIPS_J, lbl);
		}
	}
	else
	{
		uni_printf(enc->sx->io, "\t");
		instruction_to_io(enc->sx->io, instruction);
		uni_printf(enc->sx->io, " ");
		rvalue_to_io(enc, value);
		uni_printf(enc->sx->io, ", ");
		if (instruction == IC_MIPS_BEQ || instruction == IC_MIPS_BNE)
		{
			mips_register_to_io(enc->sx->io, R_ZERO);
			uni_printf(enc->sx->io, ", ");
		}
		// иначе инструкции вида B..Z -- сравнение с нулём прямо в них
		emit_label(enc, lbl);
		uni_printf(enc->sx->io, "\n");
	}
}

/**
 * Emit branching with register
 * 
 * @param	enc				Encoder
 * @param	reg				Register			
*/
static void emit_register_branch(encoder *const enc, const mips_instruction_t instruction, const mips_register_t reg)
{
	assert(instruction == IC_MIPS_JR);

	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, instruction);
	uni_printf(enc->sx->io, " ");
	mips_register_to_io(enc->sx->io, reg);
	uni_printf(enc->sx->io, "\n");
}


/*
 *	 ______     __  __     ______   ______     ______     ______     ______     __     ______     __   __     ______
 *	/\  ___\   /\_\_\_\   /\  == \ /\  == \   /\  ___\   /\  ___\   /\  ___\   /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \  __\   \/_/\_\/_  \ \  _-/ \ \  __<   \ \  __\   \ \___  \  \ \___  \  \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \_____\   /\_\/\_\  \ \_\    \ \_\ \_\  \ \_____\  \/\_____\  \/\_____\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/_____/   \/_/\/_/   \/_/     \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 * Creates register kind rvalue and stores there constant kind rvalue
 * 
 * @param	enc				Encoder
 * @param	value			Rvalue of constant kind
 * 
 * @return	Created rvalue
*/
static rvalue emit_load_of_immediate(encoder *const enc, const rvalue *const value)
{
	assert(value->kind == RVALUE_KIND_CONST);

	const mips_register_t reg = (type_is_floating(value->type)) ? get_float_register(enc) : get_register(enc);
	const mips_instruction_t instruction = (type_is_floating(value->type)) ? IC_MIPS_LI_S : IC_MIPS_LI;

	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, instruction);
	uni_printf(enc->sx->io, " ");
	mips_register_to_io(enc->sx->io, reg);
	uni_printf(enc->sx->io, ", ");
	rvalue_to_io(enc, value);
	uni_printf(enc->sx->io, "\n");

	return (rvalue) {
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_REGISTER,
		.val.reg_num = reg,
		.type = value->type
	};
}

/**
 * Loads lvalue to register and forms rvalue
 *
 * @param	enc				Encoder
 * @param	lval			Lvalue to load
 *
 * @return	Formed rvalue
 */
static rvalue emit_load_of_lvalue(encoder *const enc, const lvalue *const lval)
{
	if (lval->kind == LVALUE_KIND_REGISTER)
	{
		return (rvalue) { 
			.kind = RVALUE_KIND_REGISTER, 
			.val.reg_num = lval->loc.reg_num, 
			.from_lvalue = FROM_LVALUE,
			.type = lval->type
		};
	}

	if (type_is_structure(enc->sx, lval->type))
	{
		// Грузим адрес первого элемента на регистр
		const rvalue addr_rvalue = { .kind = RVALUE_KIND_CONST, .val.int_val = lval->loc.displ, .type = TYPE_INTEGER };
		return emit_load_of_immediate(enc, &addr_rvalue);
	}

	const bool is_floating = type_is_floating(lval->type);
	const mips_register_t reg = is_floating ? get_float_register(enc) : get_register(enc);
	const mips_instruction_t instruction = is_floating ? IC_MIPS_L_S : IC_MIPS_LW;

	const rvalue result = { 
		.kind = RVALUE_KIND_REGISTER,
		.val.reg_num = reg,
		.from_lvalue = !FROM_LVALUE,
		.type = lval->type,
	};

	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, instruction);
	uni_printf(enc->sx->io, " ");
	rvalue_to_io(enc, &result);
	uni_printf(enc->sx->io, ", %" PRIitem "(", lval->loc.displ);
	mips_register_to_io(enc->sx->io, lval->base_reg);
	uni_printf(enc->sx->io, ")\n");

	// Для любых скалярных типов ничего не произойдёт,
	// а для остальных освобождается base_reg, в котором хранилось смещение
	free_register(enc, lval->base_reg);

	return result;
}

/**
 * Emit identifier lvalue
 *
 * @param	enc				Encoder
 * @param	nd  			Node in AST
 *
 * @return	Identifier lvalue
 */
static lvalue emit_identifier_lvalue(encoder *const enc, const node *const nd)
{
	return displacements_get(enc, expression_identifier_get_id(nd));
}

/**
 * Emit subscript lvalue
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Subscript lvalue
 */
static lvalue emit_subscript_lvalue(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);

	const node base = expression_subscript_get_base(nd);
	const rvalue base_value = emit_expression(enc, &base);

	const node index = expression_subscript_get_index(nd);
	const rvalue index_value = emit_expression(enc, &index);

	// base_value гарантированно имеет kind == RVALUE_KIND_REGISTER
	if (index_value.kind == RVALUE_KIND_CONST)
	{
		return (lvalue) {
			.kind = LVALUE_KIND_STACK,
			.base_reg = base_value.val.reg_num,
			.loc.displ = -(item_t)index_value.val.int_val * mips_type_size(enc->sx, type),
			.type = type
		};
	}
	const rvalue type_size_value = {
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_CONST,
		.val.int_val = mips_type_size(enc->sx, type),
		.type = TYPE_INTEGER
	};
	
	emit_binary_operation(enc, &index_value, &index_value, &type_size_value, BIN_MUL);
	emit_binary_operation(enc, &base_value, &base_value, &index_value, BIN_SUB);

	free_rvalue(enc, &index_value);

	return (lvalue) { .kind = LVALUE_KIND_STACK, .base_reg = base_value.val.reg_num, .loc.displ = 0, .type = type };
}

/**
 * Emit member lvalue
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Created lvalue
 */
static lvalue emit_member_lvalue(encoder *const enc, const node *const nd)
{
	const node base = expression_member_get_base(nd);
	const item_t base_type = expression_get_type(&base);

	const bool is_arrow = expression_member_is_arrow(nd);
	const item_t struct_type = is_arrow ? type_pointer_get_element_type(enc->sx, base_type) : base_type;

	size_t member_displ = 0;
	const size_t member_index = expression_member_get_member_index(nd);
	for (size_t i = 0; i < member_index; i++)
	{
		const item_t member_type = type_structure_get_member_type(enc->sx, struct_type, i);
		member_displ += mips_type_size(enc->sx, member_type);
	}

	const item_t type = expression_get_type(nd);

	if (is_arrow)
	{
		const rvalue struct_pointer = emit_expression(enc, &base);
		// FIXME: грузить константу на регистр в случае константных указателей
		return (lvalue) {
			.kind = LVALUE_KIND_STACK, 
			.base_reg = struct_pointer.val.reg_num,
			.loc.displ = member_displ, 
			.type = type
		};
	}
	else
	{
		const lvalue base_lvalue = emit_lvalue(enc, &base);
		const size_t displ = base_lvalue.loc.displ + member_displ;
		return (lvalue) { 
			.kind = LVALUE_KIND_STACK,
			.base_reg = base_lvalue.base_reg, 
			.loc.displ = displ, 
			.type = type 
		};
	}
}

/**
 * Emit indirection lvalue
 * 
 * @param	enc				Encoder
 * @param	nd				Node in AST
 * 
 * @return	Indirected lvalue
*/
static lvalue emit_indirection_lvalue(encoder *const enc, const node *const nd)
{
	assert(expression_unary_get_operator(nd) == UN_INDIRECTION);

	const node operand = expression_unary_get_operand(nd);
	const rvalue base = emit_expression(enc, &operand);
	// FIXME: грузить константу на регистр в случае константных указателей
	const item_t type = expression_get_type(nd);

	return (lvalue) {
		.kind = LVALUE_KIND_STACK,
		.base_reg = base.val.reg_num,
		.loc.displ = 0,
		.type = type
	};
}

/**
 * Emit lvalue expression
 *
 * @param	enc			encoder
 * @param	nd				Node in AST
 *
 * @return	Lvalue
 */
static lvalue emit_lvalue(encoder *const enc, const node *const nd)
{
	assert(expression_is_lvalue(nd));

	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			return emit_identifier_lvalue(enc, nd);

		case EXPR_SUBSCRIPT:
			return emit_subscript_lvalue(enc, nd);

		case EXPR_MEMBER:
			return emit_member_lvalue(enc, nd);

		case EXPR_UNARY: // Только UN_INDIRECTION
			return emit_indirection_lvalue(enc, nd);

		default:
			// Не может быть lvalue
			system_error(node_unexpected, nd);
			return (lvalue) { .loc.displ = ITEM_MAX };
	}
}

/**
 * Stores one rvalue of register kind to another
 * 
 * @param	enc				Encoder
 * @param	target			Target register
 * @param	value			Rvalue to store
*/
static void emit_move_rvalue_to_register(encoder *const enc
	, const mips_register_t target, const rvalue *const value)
{
	if (value->kind == RVALUE_KIND_CONST)
	{
		const mips_instruction_t instruction = !type_is_floating(value->type) ? IC_MIPS_LI : IC_MIPS_LI_S;
		uni_printf(enc->sx->io, "\t");
		instruction_to_io(enc->sx->io, instruction);
		uni_printf(enc->sx->io, " ");
		mips_register_to_io(enc->sx->io, target);
		uni_printf(enc->sx->io, ", ");
		rvalue_to_io(enc, value);
		uni_printf(enc->sx->io, "\n");
		return;
	}

	if (value->val.reg_num == target)
	{
		uni_printf(enc->sx->io, "\t# stays in register ");
		mips_register_to_io(enc->sx->io, target);
		uni_printf(enc->sx->io, ":\n");
	}
	else
	{
		const mips_instruction_t instruction = !type_is_floating(value->type) ? IC_MIPS_MOVE : IC_MIPS_MFC_1;
		uni_printf(enc->sx->io, "\t");
		instruction_to_io(enc->sx->io, instruction);
		uni_printf(enc->sx->io, " ");
		mips_register_to_io(enc->sx->io, target);
		uni_printf(enc->sx->io, ", ");
		rvalue_to_io(enc, value);
		uni_printf(enc->sx->io, "\n");
	}
}

/**
 * Stores rvalue to lvalue
 *
 * @param	enc				Encoder
 * @param	target			Target lvalue
 * @param	value			Rvalue to store
 */
static void emit_store_of_rvalue(encoder *const enc, const lvalue *const target, const rvalue *const value)
{
	assert(value->kind != RVALUE_KIND_VOID);

	const rvalue reg_value = (value->kind == RVALUE_KIND_CONST) ? emit_load_of_immediate(enc, value) : *value;

	if (target->kind == LVALUE_KIND_REGISTER)
	{
		if (value->val.reg_num != target->loc.reg_num)
		{
			const mips_instruction_t instruction = type_is_floating(value->type) ? IC_MIPS_MOV_S : IC_MIPS_MOVE;
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, instruction);
			uni_printf(enc->sx->io, " ");
			lvalue_to_io(enc, target);
			uni_printf(enc->sx->io, ", ");
			rvalue_to_io(enc, &reg_value);
			uni_printf(enc->sx->io, "\n");
		}
	}
	else
	{
		if ((!type_is_structure(enc->sx, target->type)) && (!type_is_array(enc->sx, target->type)))
		{
			const mips_instruction_t instruction = type_is_floating(value->type) ? IC_MIPS_S_S : IC_MIPS_SW;
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, instruction);
			uni_printf(enc->sx->io, " ");
			rvalue_to_io(enc, &reg_value);
			uni_printf(enc->sx->io, ", ");
			lvalue_to_io(enc, target);
			uni_printf(enc->sx->io, "\n");

			// Освобождаем регистр только в том случае, если он был занят на этом уровне. Выше не лезем.
			if (value->kind == RVALUE_KIND_CONST)
			{
				free_rvalue(enc, &reg_value);
			}

			free_register(enc, target->base_reg);
		}
		else
		{
			if (type_is_array(enc->sx, target->type))
			{
				// Загружаем указатель на массив
				uni_printf(enc->sx->io, "\t");
				instruction_to_io(enc->sx->io, IC_MIPS_SW);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, &reg_value);
				uni_printf(enc->sx->io, ", %" PRIitem "(", target->loc.displ);
				mips_register_to_io(enc->sx->io, target->base_reg);
				uni_printf(enc->sx->io, ")\n\n");
				return;
			}
			// else кусок должен быть не достижим
		}
	}
}

/**
 * Emit binary operation with two rvalues
 *
 * @param	enc				Encoder
 * @param	dest			Destination rvalue
 * @param	first_operand	First rvalue operand
 * @param	second_operand	Second rvalue operand
 * @param	operator		Operator
 */
static void emit_binary_operation(encoder *const enc, const rvalue *const dest
	, const rvalue *const first_operand, const rvalue *const second_operand, const binary_t operator)
{
	assert(operator != BIN_LOG_AND);
	assert(operator != BIN_LOG_OR);

	assert(dest->kind == RVALUE_KIND_REGISTER);
	assert(first_operand->kind != RVALUE_KIND_VOID);
	assert(second_operand->kind != RVALUE_KIND_VOID);

	if ((first_operand->kind == RVALUE_KIND_REGISTER) && (second_operand->kind == RVALUE_KIND_REGISTER))
	{
		switch (operator)
		{
			case BIN_LT:
			case BIN_GT:
			case BIN_LE:
			case BIN_GE:
			case BIN_EQ:
			case BIN_NE:
			{
				const item_t curr_label_num = enc->label_num++;
				const label label_else = { .kind = L_END, .num = curr_label_num };

				uni_printf(enc->sx->io, "\t");
				instruction_to_io(enc->sx->io, IC_MIPS_SUB);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, dest);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, first_operand);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, second_operand);
				uni_printf(enc->sx->io, "\n");

				const mips_instruction_t instruction = get_bin_instruction(operator, false);
				emit_conditional_branch(enc, instruction, dest, &label_else);

				uni_printf(enc->sx->io, "\t");
				instruction_to_io(enc->sx->io, IC_MIPS_LI);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, dest);
				uni_printf(enc->sx->io, ", 0\n");

				emit_label_declaration(enc, &label_else);

				uni_printf(enc->sx->io, "\n");
			}
			break;

			default:
			{
				uni_printf(enc->sx->io, "\t");
				instruction_to_io(
					enc->sx->io,
					get_bin_instruction(operator, /* Два регистра => 0 в get_bin_instruction() -> */ 0)
				);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, dest);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, first_operand);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, second_operand);
				uni_printf(enc->sx->io, "\n");
			}
			break;
		}
	}
	else
	{
		// Гарантируется, что будет ровно один оператор в регистре и один оператор в константе
		const rvalue *imm_rvalue = (second_operand->kind != RVALUE_KIND_CONST) ? first_operand : second_operand;
		const rvalue *const var_rvalue = (second_operand->kind == RVALUE_KIND_CONST) ? first_operand : second_operand;

		switch (operator)
		{
			case BIN_LT:
			case BIN_GT:
			case BIN_LE:
			case BIN_GE:
			case BIN_EQ:
			case BIN_NE:
			{
				const item_t curr_label_num = enc->label_num++;
				const label label_else = { .kind = L_ELSE, .num = curr_label_num };

				// Загружаем <значение из second_operand> на регистр
				const rvalue tmp = emit_load_of_immediate(enc, imm_rvalue);
				imm_rvalue = &tmp;

				// Записываем <значение из first_operand> - <значение из second_operand> в dest
				uni_printf(enc->sx->io, "\t");
				instruction_to_io(enc->sx->io, IC_MIPS_SUB);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, dest);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, var_rvalue);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, imm_rvalue);
				uni_printf(enc->sx->io, "\n");

				const mips_instruction_t instruction = get_bin_instruction(operator, false);
				emit_conditional_branch(enc, instruction, dest, &label_else);

				uni_printf(enc->sx->io, "\t");
				instruction_to_io(enc->sx->io, IC_MIPS_LI);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, dest);
				uni_printf(enc->sx->io, ", 0\n");

				emit_label_declaration(enc, &label_else);

				uni_printf(enc->sx->io, "\n");
				break;
			}

			default:
			{
				bool in_reg = false;
				// Предварительно загружаем константу из imm_rvalue в rvalue вида RVALUE_KIND_REGISTER
				if ((operator == BIN_SUB) || (operator == BIN_DIV) || (operator == BIN_MUL) ||
					(operator == BIN_REM))
				{
					// Нет команд вычитания из значения по регистру константы, так что умножаем на (-1)
					const rvalue tmp = emit_load_of_immediate(enc, imm_rvalue);
					imm_rvalue = &tmp;
					if (operator == BIN_SUB)
					{
						emit_binary_operation(enc, imm_rvalue, imm_rvalue, &RVALUE_NEGATIVE_ONE, BIN_MUL);
					}
					in_reg = true;
				}

				// Выписываем операцию, её результат будет записан в result
				uni_printf(enc->sx->io, "\t");
				instruction_to_io(
					enc->sx->io,
					get_bin_instruction(operator,
						/* Один регистр => true в get_bin_instruction() -> */ !in_reg)
				);
				uni_printf(enc->sx->io, " ");
				rvalue_to_io(enc, dest);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, var_rvalue);
				uni_printf(enc->sx->io, ", ");
				rvalue_to_io(enc, imm_rvalue);
				uni_printf(enc->sx->io, "\n");
			}
		}
	}
}

/**
 *	Emit literal expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of literal expression
 */
static rvalue emit_literal_expression(encoder *const enc, const node *const nd)
{
	const item_t type = expression_get_type(nd);
	switch (type_get_class(enc->sx, type))
	{
		case TYPE_BOOLEAN:
			return (rvalue) { 
				.kind = RVALUE_KIND_CONST,
				.val.int_val = (expression_literal_get_boolean(nd)) ? 1 : 0,
				.type = TYPE_INTEGER
			};

		case TYPE_CHARACTER:
			return (rvalue) { 
				.kind = RVALUE_KIND_CONST,
				.val.int_val = expression_literal_get_character(nd),
				.type = TYPE_INTEGER
			};

		case TYPE_INTEGER:
			return (rvalue) { 
				.kind = RVALUE_KIND_CONST,
				.val.int_val = expression_literal_get_integer(nd),
				.type = TYPE_INTEGER 
			};

		case TYPE_FLOATING:
			return (rvalue) { 
				.kind = RVALUE_KIND_CONST,
				.val.float_val = expression_literal_get_floating(nd),
				.type = TYPE_FLOATING
			};

		case TYPE_ARRAY:
			assert(type_is_string(enc->sx, type));
			return (rvalue) { 
				.kind = RVALUE_KIND_CONST,
				.val.str_index = expression_literal_get_string(nd),
				.type = type
			};

		default:
			return RVALUE_VOID;
	}
}

/**
 * Emit printf expression
 *
 * @param	enc					Encoder
 * @param	nd					AST node
 * @param	parameters_amount	Number of function parameters
 */
static rvalue emit_printf_expression(encoder *const enc, const node *const nd)
{
	const node string = expression_call_get_argument(nd, 0);
	const size_t index = expression_literal_get_string(&string);
	const size_t amount = strings_amount(enc->sx);
	const size_t parameters_amount = expression_call_get_arguments_amount(nd);

	for (size_t i = 1; i < parameters_amount; i++)
	{
		const node arg = expression_call_get_argument(nd, i);
		const rvalue val = emit_expression(enc, &arg);
		const rvalue arg_rvalue = (val.kind == RVALUE_KIND_CONST) ? emit_load_of_immediate(enc, &val) : val;
		const item_t arg_rvalue_type = arg_rvalue.type;

		// Всегда хотим сохранять $a0 и $a1
		to_code_2R_I(
			enc->sx->io, 
			IC_MIPS_ADDI, 
			R_SP, 
			R_SP,
			-(item_t)WORD_LENGTH * (!type_is_floating(arg_rvalue_type) ? /* $a0 и $a1 */ 1 : /* $a0, $a1 и $a2 */ 2)
		);
		uni_printf(enc->sx->io, "\n");

		const lvalue a0_lval = {
			.base_reg = R_SP,
			// по call convention: первый на WORD_LENGTH выше предыдущего положения $fp,
			// второй на 2*WORD_LENGTH и т.д.
			.loc.displ = 0,
			.kind = LVALUE_KIND_STACK,
			.type = arg_rvalue.type
		};
		const rvalue a0_rval = { 
			.kind = RVALUE_KIND_REGISTER,
			.val.reg_num = R_A0,
			.type = TYPE_INTEGER,
			.from_lvalue = !FROM_LVALUE
		};
		emit_store_of_rvalue(enc, &a0_lval, &a0_rval);

		const lvalue a1_lval = {
			.base_reg = R_SP,
			// по call convention: первый на WORD_LENGTH выше предыдущего положения $fp,
			// второй на 2*WORD_LENGTH и т.д.
			.loc.displ = WORD_LENGTH,
			.kind = LVALUE_KIND_STACK,
			.type = arg_rvalue.type
		};
		const rvalue a1_rval = {
			.kind = RVALUE_KIND_REGISTER,
			.val.reg_num = R_A1,
			.type = TYPE_INTEGER,
			.from_lvalue = !FROM_LVALUE 
		};
		emit_store_of_rvalue(enc, &a1_lval, &a1_rval);

		if (!type_is_floating(arg_rvalue.type))
		{
			uni_printf(enc->sx->io, "\n");
			emit_move_rvalue_to_register(enc, R_A1, &arg_rvalue);

			uni_printf(enc->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + (i - 1) * amount);
			uni_printf(enc->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + (i - 1) * amount);

			uni_printf(enc->sx->io, "\tjal printf\n");
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, IC_MIPS_NOP);
			uni_printf(enc->sx->io, "\n");

			free_rvalue(enc, &arg_rvalue);

			uni_printf(enc->sx->io, "\n\t# data restoring:\n");
		}
		else
		{
			const lvalue a2_lval = {
				.base_reg = R_SP,
				// по call convention: первый на WORD_LENGTH выше предыдущего положения
				// $fp, второй на 2*WORD_LENGTH и т.д.
				.loc.displ = 2 * WORD_LENGTH,
				.type = TYPE_INTEGER,
				.kind = LVALUE_KIND_STACK 
			};
			const rvalue a2_rval = {
				.kind = RVALUE_KIND_REGISTER,
				.val.reg_num = R_A2,
				.type = TYPE_INTEGER,
				.from_lvalue = !FROM_LVALUE
			};
			emit_store_of_rvalue(enc, &a2_lval, &a2_rval);
			uni_printf(enc->sx->io, "\n");

			// Конвертируем single to double
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, IC_MIPS_CVT_D_S);
			uni_printf(enc->sx->io, " ");
			rvalue_to_io(enc, &arg_rvalue);
			uni_printf(enc->sx->io, ", ");
			rvalue_to_io(enc, &arg_rvalue);
			uni_printf(enc->sx->io, "\n");

			// Следующие действия необходимы, т.к. аргументы в builtin-функции обязаны передаваться в $a0-$a3
			// Даже для floating point!
			// %lo из arg_rvalue в $a1
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, IC_MIPS_MFC_1);
			uni_printf(enc->sx->io, " ");
			mips_register_to_io(enc->sx->io, R_A1);
			uni_printf(enc->sx->io, ", ");
			rvalue_to_io(enc, &arg_rvalue);
			uni_printf(enc->sx->io, "\n");

			// %hi из arg_rvalue в $a2
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, IC_MIPS_MFHC_1);
			uni_printf(enc->sx->io, " ");
			mips_register_to_io(enc->sx->io, R_A2);
			uni_printf(enc->sx->io, ", ");
			rvalue_to_io(enc, &arg_rvalue);
			uni_printf(enc->sx->io, "\n");

			uni_printf(enc->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + (i - 1) * amount);
			uni_printf(enc->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + (i - 1) * amount);

			uni_printf(enc->sx->io, "\tjal printf\n\t");
			instruction_to_io(enc->sx->io, IC_MIPS_NOP);
			uni_printf(enc->sx->io, "\n");

			// Восстановление регистров-аргументов -- они могут понадобится в дальнейшем
			uni_printf(enc->sx->io, "\n\t# data restoring:\n");

			const rvalue a2_rval_to_copy = emit_load_of_lvalue(enc, &a2_lval);
			emit_move_rvalue_to_register(enc, R_A2, &a2_rval_to_copy);

			free_rvalue(enc, &a2_rval);
			free_rvalue(enc, &arg_rvalue);
			uni_printf(enc->sx->io, "\n");
		}

		const rvalue a0_rval_to_copy = emit_load_of_lvalue(enc, &a0_lval);
		emit_move_rvalue_to_register(enc, R_A0, &a0_rval_to_copy);

		free_rvalue(enc, &a0_rval_to_copy);
		uni_printf(enc->sx->io, "\n");

		const rvalue a1_rval_to_copy = emit_load_of_lvalue(enc, &a1_lval);
		emit_move_rvalue_to_register(enc, R_A1, &a1_rval_to_copy);

		free_rvalue(enc, &a1_rval_to_copy);
		uni_printf(enc->sx->io, "\n");

		to_code_2R_I(
			enc->sx->io,
			IC_MIPS_ADDI,
			R_SP,
			R_SP,
			(item_t)WORD_LENGTH * (!type_is_floating(arg_rvalue_type) ? /* $a0 и $a1 */ 1 : /* $a0, $a1 и $a2 */ 2)
		);
		uni_printf(enc->sx->io, "\n");
	}

	const lvalue a0_lval = { 
		.base_reg = R_SP,
		// по call convention: первый на WORD_LENGTH выше предыдущего положения $fp,
		// второй на 2*WORD_LENGTH и т.д.
		.loc.displ = 0,
		.kind = LVALUE_KIND_STACK,
		.type = TYPE_INTEGER
	};
	const rvalue a0_rval = {
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_REGISTER,
		.val.reg_num = R_A0,
		.type = TYPE_INTEGER 
	};
	emit_store_of_rvalue(enc, &a0_lval, &a0_rval);

	uni_printf(enc->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + (parameters_amount - 1) * amount);
	uni_printf(enc->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + (parameters_amount - 1) * amount);
	uni_printf(enc->sx->io, "\tjal printf\n");
	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, IC_MIPS_NOP);
	uni_printf(enc->sx->io, "\n");

	uni_printf(enc->sx->io, "\n\t# data restoring:\n");
	const rvalue a0_rval_to_copy = emit_load_of_lvalue(enc, &a0_lval);
	emit_move_rvalue_to_register(enc, R_A0, &a0_rval_to_copy);

	free_rvalue(enc, &a0_rval_to_copy);

	// FIXME: Возвращает число распечатанных символов (включая '\0'?)
	return RVALUE_VOID;
}

/**
 * Emit builtin function call
 * 
 * @param	enc					Encoder
 * @param	nd					Node in AST
 * 
 * @return	Rvalue of builtin function call expression
*/
static rvalue emit_builtin_call(encoder *const enc, const node *const nd)
{
	const node callee = expression_call_get_callee(nd);
	const size_t func_ref = expression_identifier_get_id(&callee);
	switch (func_ref)
	{
		case BI_PRINTF:
			return emit_printf_expression(enc, nd);
		
		default:
			return RVALUE_VOID;
	}
}

/**
 * Emit call expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of the result of call expression
 */
static rvalue emit_call_expression(encoder *const enc, const node *const nd)
{
	const node callee = expression_call_get_callee(nd);
	// Конвертируем в указатель на функцию
	// FIXME: хотим рассмотреть любой callee как указатель
	// на данный момент это не поддержано в билдере, когда будет сделано -- добавить в emit_expression()
	// и применяем функцию emit_identifier_expression (т.к. его категория в билдере будет проставлена как rvalue)
	const size_t func_ref = expression_identifier_get_id(&callee);
	const size_t params_amount = expression_call_get_arguments_amount(nd);

	const item_t return_type = type_function_get_return_type(enc->sx, expression_get_type(&callee));

	uni_printf(enc->sx->io, "\t# \"%s\" function call:\n", ident_get_spelling(enc->sx, func_ref));

	if (func_ref >= BEGIN_USER_FUNC)
	{
		size_t f_arg_count = 0;
		size_t arg_count = 0;
		size_t displ_for_parameters = (params_amount - 1) * WORD_LENGTH;
		const lvalue *prev_arg_displ[4 /* за $a0-$a3 */
							  + 4 / 2 /* за $fa0, $fa2 (т.к. single precision)*/];

		uni_printf(enc->sx->io, "\t# setting up $sp:\n");
		if (displ_for_parameters)
		{
			to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, -(item_t)(displ_for_parameters));
		}

		uni_printf(enc->sx->io, "\n\t# parameters passing:\n");

		// TODO: структуры/массивы в параметры
		size_t arg_reg_count = 0;
		for (size_t i = 0; i < params_amount; i++)
		{
			const node arg = expression_call_get_argument(nd, i);
			const rvalue tmp = emit_expression(enc, &arg);
			const rvalue arg_rvalue = (tmp.kind == RVALUE_KIND_CONST) ? emit_load_of_immediate(enc, &tmp) : tmp;

			if ((type_is_floating(arg_rvalue.type) ? f_arg_count : arg_count) < ARG_REG_AMOUNT)
			{
				uni_printf(enc->sx->io, "\t# saving ");
				mips_register_to_io(enc->sx->io, (type_is_floating(arg_rvalue.type) 
					? R_FA0 + f_arg_count 
					: R_A0 + arg_count));
				uni_printf(enc->sx->io, " value on stack:\n");
			}
			else
			{
				uni_printf(enc->sx->io, "\t# parameter on stack:\n");
			}

			const lvalue tmp_arg_lvalue = {
				.base_reg = R_SP,
				// по call convention: первый на WORD_LENGTH выше предыдущего положения $fp,
				// второй на 2*WORD_LENGTH и т.д.
				.loc.displ = i * WORD_LENGTH,
				.kind = LVALUE_KIND_STACK,
				.type = arg_rvalue.type
			};

			const rvalue arg_saved_rvalue = {
				.kind = RVALUE_KIND_REGISTER,
				.val.reg_num = (type_is_floating(arg_rvalue.type) 
					? R_FA0 + f_arg_count 
					: R_A0 + arg_count),
				.type = arg_rvalue.type,
				.from_lvalue = !FROM_LVALUE
			};
			// Сохранение текущего регистра-аргумента на стек либо передача аргументов на стек
			emit_store_of_rvalue(
				enc,
				&tmp_arg_lvalue,
				(type_is_floating(arg_rvalue.type) ? f_arg_count : arg_count) < ARG_REG_AMOUNT
					? &arg_saved_rvalue // Сохранение значения в регистре-аргументе
					: &arg_rvalue // Передача аргумента
			);

			// Если это передача параметров в регистры-аргументы
			if ((type_is_floating(arg_rvalue.type) ? f_arg_count : arg_count) < ARG_REG_AMOUNT)
			{
				// Аргументы рассматриваются в данном случае как регистровые переменные
				emit_move_rvalue_to_register(
					enc,
					type_is_floating(arg_rvalue.type)
						? (R_FA0 + f_arg_count)
						: (R_A0 + arg_count), 
					&arg_rvalue);

				// Запоминаем, куда положили текущее значение, лежавшее в регистре-аргументе
				prev_arg_displ[arg_reg_count++] = &tmp_arg_lvalue;
			}

			if (type_is_floating(arg_rvalue.type))
			{
				f_arg_count += 2;
			}
			else
			{
				arg_count += 1;
			}

			free_rvalue(enc, &arg_rvalue);
		}

		const label label_func = { .kind = L_FUNC, .num = func_ref };
		emit_unconditional_branch(enc, IC_MIPS_JAL, &label_func);

		// Восстановление регистров-аргументов -- они могут понадобится в дальнейшем
		uni_printf(enc->sx->io, "\n\t# data restoring:\n");

		size_t i = 0, j = 0; // Счётчик обычных и floating point регистров-аргументов соответственно
		while (i + j < arg_reg_count)
		{
			uni_printf(enc->sx->io, "\n");

			const rvalue tmp_rval = emit_load_of_lvalue(enc, prev_arg_displ[i + j]);
			emit_move_rvalue_to_register(
				enc,
				type_is_floating(prev_arg_displ[i + j]->type) ? (R_FA0 + 2 * j++) : (R_A0 + i++),
				&tmp_rval
			);

			free_rvalue(enc, &tmp_rval);
		}

		if (displ_for_parameters)
		{
			to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, (item_t)displ_for_parameters);
		}

		uni_printf(enc->sx->io, "\n");
	}
	else
	{
		return emit_builtin_call(enc, nd);
	}

	return (rvalue) {
		.kind = RVALUE_KIND_REGISTER,
		.type = return_type,
		.val.reg_num = type_is_floating(return_type) ? R_FV0 : R_V0,
		.from_lvalue = !FROM_LVALUE
	};
}

/**
 * Emit member expression
 * 
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of member expression
*/
static rvalue emit_member_expression(encoder *const enc, const node *const nd)
{
	(void)enc;
	(void)nd;
	// FIXME: возврат структуры из функции. Указателя тут оказаться не может
	return RVALUE_VOID;
}

/**
 * Emit cast expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of cast expression
 */
static rvalue emit_cast_expression(encoder *const enc, const node *const nd)
{
	const node operand = expression_cast_get_operand(nd);
	const rvalue value = emit_expression(enc, &operand);

	const item_t target_type = expression_get_type(nd);
	const item_t source_type = expression_get_type(&operand);

	if (type_is_integer(enc->sx, source_type) && type_is_floating(target_type))
	{
		// int -> float
		const rvalue result = { 
			.kind = RVALUE_KIND_REGISTER,
			.from_lvalue = !FROM_LVALUE,
			.val.reg_num = get_float_register(enc),
			.type = target_type 
		};

		// FIXME: избавится от to_code функций
		to_code_2R(enc->sx->io, IC_MIPS_MFC_1, value.val.reg_num, result.val.reg_num);
		to_code_2R(enc->sx->io, IC_MIPS_CVT_S_W, result.val.reg_num, result.val.reg_num);

		free_rvalue(enc, &value);
		return result;
	}
	else
	{
		// char -> int пока не поддержано в билдере
		return (rvalue) {
			.from_lvalue = value.from_lvalue,
			.kind = value.kind,
			.val.int_val = value.val.int_val,
			.type = TYPE_INTEGER
		};
	}
}

/**
 *	Emit increment or decrement expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return Rvalue of the result of increment or decrement expression
 */
static rvalue emit_increment_expression(encoder *const enc, const node *const nd)
{
	const node operand = expression_unary_get_operand(nd);
	const lvalue operand_lvalue = emit_lvalue(enc, &operand);
	const rvalue operand_rvalue = emit_load_of_lvalue(enc, &operand_lvalue);

	const unary_t operator = expression_unary_get_operator(nd);
	const bool is_prefix = (operator == UN_PREDEC) || (operator == UN_PREINC);
	const rvalue imm_rvalue = {
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_CONST,
		.val.int_val = ((operator == UN_PREINC) || (operator == UN_POSTINC)) ? 1 : -1,
		.type = TYPE_INTEGER
	};

	if (is_prefix)
	{
		emit_binary_operation(enc, &operand_rvalue, &operand_rvalue, &imm_rvalue, BIN_ADD);
		emit_store_of_rvalue(enc, &operand_lvalue, &operand_rvalue);
	}
	else
	{
		const rvalue post_result_rvalue = {
			.from_lvalue = !FROM_LVALUE,
			.kind = RVALUE_KIND_REGISTER, 
			.val.reg_num = get_register(enc), 
			.type = operand_lvalue.type 
		};

		emit_binary_operation(enc, &post_result_rvalue, &operand_rvalue, &imm_rvalue, BIN_ADD);
		emit_store_of_rvalue(enc, &operand_lvalue, &post_result_rvalue);
		free_rvalue(enc, &post_result_rvalue);
	}

	return operand_rvalue;
}

/**
 *	Emit unary expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of the result of unary expression
 */
static rvalue emit_unary_expression(encoder *const enc, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd);
	assert(operator != UN_INDIRECTION);

	switch (operator)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			return emit_increment_expression(enc, nd);

		case UN_MINUS:
		case UN_NOT:
		{
			// вынести в отдельные функции
			const node operand = expression_unary_get_operand(nd);
			const rvalue operand_rvalue = emit_expression(enc, &operand);

			if (operator == UN_MINUS)
			{
				emit_binary_operation(enc, &operand_rvalue, &RVALUE_ZERO, &operand_rvalue, BIN_SUB);
				return operand_rvalue;
			}
			else
			{
				emit_binary_operation(enc, &operand_rvalue, &operand_rvalue, &RVALUE_NEGATIVE_ONE, BIN_XOR);
				return operand_rvalue;
			}
		}

		case UN_LOGNOT:
		{
			const node operand = expression_unary_get_operand(nd);
			const rvalue value = emit_expression(enc, &operand);

			to_code_2R_I(enc->sx->io, IC_MIPS_SLTIU, value.val.reg_num, value.val.reg_num, 1);
			return value;
		}

		case UN_ABS:
		{
			const node operand = expression_unary_get_operand(nd);
			const rvalue operand_rvalue = emit_expression(enc, &operand);
			const mips_instruction_t instruction = type_is_floating(operand_rvalue.type) ? IC_MIPS_ABS_S : IC_MIPS_ABS;

			to_code_2R(enc->sx->io, instruction, operand_rvalue.val.reg_num, operand_rvalue.val.reg_num);
			return operand_rvalue;
		}

		case UN_ADDRESS:
		{
			const node operand = expression_unary_get_operand(nd);
			const lvalue operand_lvalue = emit_lvalue(enc, &operand);

			assert(operand_lvalue.kind != LVALUE_KIND_REGISTER);

			const rvalue result_rvalue = {
				.from_lvalue = !FROM_LVALUE,
				.kind = RVALUE_KIND_REGISTER,
				.val.reg_num = get_register(enc),
				.type = TYPE_INTEGER
			};

			to_code_2R_I(
				enc->sx->io, 
				IC_MIPS_ADDI, 
				result_rvalue.val.reg_num, 
				operand_lvalue.base_reg, 
				operand_lvalue.loc.displ
			);

			return result_rvalue;
		}

		case UN_UPB:
		{
			const node operand = expression_unary_get_operand(nd);
			const rvalue arr_displ_rvalue = emit_expression(enc, &operand);
			const rvalue word_size_rvalue = {
				.from_lvalue = !FROM_LVALUE,
				.kind = RVALUE_KIND_CONST,
				.val.int_val = WORD_LENGTH,
				.type = TYPE_INTEGER
			};
			emit_binary_operation(enc, &arr_displ_rvalue, &arr_displ_rvalue, &word_size_rvalue, BIN_ADD);
			const lvalue size_lvalue = { 
				.base_reg = arr_displ_rvalue.val.reg_num,
				.kind = LVALUE_KIND_STACK,
				.loc.displ = 0,
				.type = TYPE_INTEGER
			};
			return emit_load_of_lvalue(enc, &size_lvalue);
		}

		default:
			system_error(node_unexpected);
			return RVALUE_VOID;
	}
}

/**
 * Emit binary expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of the result of binary expression
 */
static rvalue emit_binary_expression(encoder *const enc, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	const node LHS = expression_binary_get_LHS(nd);
	const node RHS = expression_binary_get_RHS(nd);

	switch (operator)
	{
		case BIN_COMMA:
		{
			emit_void_expression(enc, &LHS);
			return emit_expression(enc, &RHS);
		}

		case BIN_LOG_OR:
		case BIN_LOG_AND:
		{
			const rvalue lhs_rvalue = emit_expression(enc, &LHS);
			// Случай константы нужно сделать в билдере, на данный момент отсутствует

			const item_t curr_label_num = enc->label_num++;
			const label label_end = { .kind = L_END, .num = curr_label_num };

			const mips_instruction_t instruction = (operator == BIN_LOG_OR) ? IC_MIPS_BNE : IC_MIPS_BEQ;
			emit_conditional_branch(enc, instruction, &lhs_rvalue, &label_end);

			free_rvalue(enc, &lhs_rvalue);

			const rvalue rhs_rvalue = emit_expression(enc, &RHS);
			assert(lhs_rvalue.val.reg_num == rhs_rvalue.val.reg_num);

			emit_label_declaration(enc, &label_end);
			return rhs_rvalue;
		}

		default:
		{
			const rvalue lhs_rvalue = emit_expression(enc, &LHS);
			const rvalue rhs_rvalue = emit_expression(enc, &RHS);

			emit_binary_operation(enc, &lhs_rvalue, &lhs_rvalue, &rhs_rvalue, operator);

			free_rvalue(enc, &rhs_rvalue);
			return lhs_rvalue;
		}
	}
}

/**
 * Emit ternary expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of the result of ternary expression
 */
static rvalue emit_ternary_expression(encoder *const enc, const node *const nd)
{
	const node condition = expression_ternary_get_condition(nd);
	const rvalue value = emit_expression(enc, &condition);

	const size_t label_num = enc->label_num++;
	const label label_else = { .kind = L_ELSE, .num = label_num };

	const mips_instruction_t instruction = IC_MIPS_BEQ;
	emit_conditional_branch(enc, instruction, &value, &label_else);
	free_rvalue(enc, &value);

	const node LHS = expression_ternary_get_LHS(nd);
	const rvalue LHS_rvalue = emit_expression(enc, &LHS);
	free_rvalue(enc, &LHS_rvalue);

	const label label_end = { .kind = L_END, .num = label_num };
	emit_unconditional_branch(enc, IC_MIPS_J, &label_end);
	emit_label_declaration(enc, &label_else);

	const node RHS = expression_ternary_get_RHS(nd);
	const rvalue RHS_rvalue = emit_expression(enc, &RHS);

	emit_label_declaration(enc, &label_end);
	assert(LHS_rvalue.val.reg_num == RHS_rvalue.val.reg_num);

	return RHS_rvalue;
}

/**
 * Emit assignment expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of the result of assignment expression
 */
static rvalue emit_assignment_expression(encoder *const enc, const node *const nd)
{
	const node LHS = expression_assignment_get_LHS(nd);
	const lvalue target = emit_lvalue(enc, &LHS);

	const node RHS = expression_assignment_get_RHS(nd);
	const item_t RHS_type = expression_get_type(&RHS);
	if (type_is_structure(enc->sx, RHS_type))
	{
		// FIXME: возврат структуры из функции
		// Грузим адрес первого элемента RHS на регистр
		const size_t RHS_identifier = expression_identifier_get_id(&RHS);
		const size_t displ = hash_get(&enc->displacements, RHS_identifier, 1);
		const rvalue tmp = {
			.from_lvalue = !FROM_LVALUE,
			.kind = RVALUE_KIND_CONST,
			.val.int_val = displ,
			.type = TYPE_INTEGER
		};
		const rvalue RHS_addr_rvalue = emit_load_of_immediate(enc, &tmp);

		// Подсчёт размеров структуры
		const size_t struct_size = mips_type_size(enc->sx, RHS_type);
		const mips_register_t reg = get_register(enc);

		// Копирование всех данных из RHS 
		for (size_t i = 0; i < struct_size; i += WORD_LENGTH)
		{
			// Грузим данные из RHS
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, IC_MIPS_LW);
			uni_printf(enc->sx->io, " ");
			mips_register_to_io(enc->sx->io, reg);
			uni_printf(enc->sx->io, ", %zu(", i);
			rvalue_to_io(enc, &RHS_addr_rvalue);
			uni_printf(enc->sx->io, ")\n");

			// Отправляем их в target
			uni_printf(enc->sx->io, "\t");
			instruction_to_io(enc->sx->io, IC_MIPS_SW);
			uni_printf(enc->sx->io, " ");
			mips_register_to_io(enc->sx->io, reg);
			uni_printf(enc->sx->io, ", %" PRIitem "(", target.loc.displ + i);
			mips_register_to_io(enc->sx->io, target.base_reg);
			uni_printf(enc->sx->io, ")\n\n");
		}

		return emit_load_of_lvalue(enc, &target);
	}

	const rvalue value = emit_expression(enc, &RHS);

	const binary_t operator = expression_assignment_get_operator(nd);
	if (operator == BIN_ASSIGN)
	{
		emit_store_of_rvalue(enc, &target, &value);
		return value;
	}

	// это "+=", "-=" и т.п.
	const rvalue prev_value = emit_load_of_lvalue(enc, &target);
	binary_t correct_operation;
	switch (operator)
	{
		case BIN_ADD_ASSIGN:
			correct_operation = BIN_ADD;
			break;

		case BIN_SUB_ASSIGN:
			correct_operation = BIN_SUB;
			break;

		case BIN_MUL_ASSIGN:
			correct_operation = BIN_MUL;
			break;

		case BIN_DIV_ASSIGN:
			correct_operation = BIN_DIV;
			break;

		case BIN_SHL_ASSIGN:
			correct_operation = BIN_SHL;
			break;

		case BIN_SHR_ASSIGN:
			correct_operation = BIN_SHR;
			break;

		case BIN_AND_ASSIGN:
			correct_operation = BIN_AND;
			break;

		case BIN_XOR_ASSIGN:
			correct_operation = BIN_XOR;
			break;

		case BIN_OR_ASSIGN:
			correct_operation = BIN_OR;
			break;

		default:
			system_error(node_unexpected);
			return RVALUE_VOID;
	}

	emit_binary_operation(enc, &value, &prev_value, &value, correct_operation);
	emit_store_of_rvalue(enc, &target, &value);

	return value;
}

/**
 * Emit inline expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of inline expression
 */
/*
// TODO: текущая ветка от feature, а туда inline_expression'ы пока не влили
static rvalue emit_inline_expression(encoder *const enc, const node *const nd)
{
	// FIXME: inline expression cannot return value at the moment
	const size_t amount = expression_inline_get_size(nd);

	for (size_t i = 0; i < amount; i++)
	{
		const node substmt = expression_inline_get_substmt(nd, i);
		emit_statement(enc, &substmt);
	}

	return RVALUE_VOID;
}
*/

/**
 * Emit expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of the expression
 */
static rvalue emit_expression(encoder *const enc, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		const lvalue lval = emit_lvalue(enc, nd);
		return emit_load_of_lvalue(enc, &lval);
	}

	// Иначе rvalue:
	switch (expression_get_class(nd))
	{
		case EXPR_LITERAL:
			return emit_literal_expression(enc, nd);

		case EXPR_CALL:
			return emit_call_expression(enc, nd);

		case EXPR_MEMBER:
			return emit_member_expression(enc, nd);

		case EXPR_CAST:
			return emit_cast_expression(enc, nd);

		case EXPR_UNARY:
			return emit_unary_expression(enc, nd);

		case EXPR_BINARY:
			return emit_binary_expression(enc, nd);

		case EXPR_ASSIGNMENT:
			return emit_assignment_expression(enc, nd);

		case EXPR_TERNARY:
			return emit_ternary_expression(enc, nd);

		/*
		// TODO: текущая ветка от feature, а туда inline_expression'ы пока не влили
		case EXPR_INLINE:
			return emit_inline_expression(enc, nd);
		*/

		case EXPR_INITIALIZER:
			system_error(node_unexpected);
			return RVALUE_VOID;

		default:
			system_error(node_unexpected);
			return RVALUE_VOID;
	}
}

/**
 * Emit expression which will be evaluated as a void expression
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 *
 * @return	Rvalue of void type
 */
static rvalue emit_void_expression(encoder *const enc, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		emit_lvalue(enc, nd); // Либо регистровая переменная, либо на стеке => ничего освобождать не надо
	}
	else
	{
		const rvalue result = emit_expression(enc, nd);
		free_rvalue(enc, &result);
	}
	return RVALUE_VOID;
}


/*
 *	 _____     ______     ______     __         ______     ______     ______     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\   /\ \       /\  __ \   /\  == \   /\  __ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \ \____  \ \ \____  \ \  __ \  \ \  __<   \ \  __ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/_/ /_/   \/_/\/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


static void emit_array_init(encoder *const enc, const node *const nd, const size_t dimension
	, const node *const init, const rvalue *const addr)
{
	const size_t amount = expression_initializer_get_size(init);

	// Проверка на соответствие размеров массива и инициализатора
	uni_printf(enc->sx->io, "\n\t# Check for array and initializer sizes equality:\n");

	const node bound = declaration_variable_get_bound(nd, dimension);
	const rvalue tmp = emit_expression(enc, &bound);
	const rvalue bound_rvalue = (tmp.kind == RVALUE_KIND_REGISTER) ? tmp : emit_load_of_immediate(enc, &tmp);

	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, IC_MIPS_ADDI);
	uni_printf(enc->sx->io, " ");
	rvalue_to_io(enc, &bound_rvalue);
	uni_printf(enc->sx->io, ", ");
	rvalue_to_io(enc, &bound_rvalue);
	uni_printf(enc->sx->io, ", %" PRIitem "\n", (-1)*amount);

	uni_printf(enc->sx->io, "\t");
	instruction_to_io(enc->sx->io, IC_MIPS_BNE);
	uni_printf(enc->sx->io, " ");
	rvalue_to_io(enc, &bound_rvalue);
	uni_printf(enc->sx->io, ", ");
	mips_register_to_io(enc->sx->io, R_ZERO);
	uni_printf(enc->sx->io, ", error\n");

	free_rvalue(enc, &bound_rvalue);

	for (size_t i = 0; i < amount; i++)
	{
		const node subexpr = expression_initializer_get_subexpr(init, i);
		uni_printf(enc->sx->io, "\n");
		if (expression_get_class(&subexpr) == EXPR_INITIALIZER)
		{
			// Сдвиг адреса на размер массива + 1 (за размер следующего измерения)
			const mips_register_t reg = get_register(enc);
			to_code_R_I_R(enc->sx->io, IC_MIPS_LW, reg, 0, addr->val.reg_num); // адрес следующего измерения

			const rvalue next_addr = {
				.from_lvalue = !FROM_LVALUE,
				.kind = RVALUE_KIND_REGISTER,
				.val.reg_num = reg,
				.type = TYPE_INTEGER
			};

			emit_array_init(enc, nd, dimension + 1, &subexpr, &next_addr);

			// Сдвиг адреса
			to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, addr->val.reg_num, addr->val.reg_num, -(item_t)WORD_LENGTH);
			uni_printf(enc->sx->io, "\n");
			free_register(enc, reg);
		}
		else
		{
			const rvalue subexpr_value = emit_expression(enc, &subexpr); // rvalue элемента
			const lvalue array_index_value = {
				.base_reg = addr->val.reg_num,
				.loc.displ = 0,
				.kind = LVALUE_KIND_STACK,
				.type = TYPE_INTEGER
			};
			emit_store_of_rvalue(enc, &array_index_value, &subexpr_value);
			lock_register(enc, addr->val.reg_num);
			if (i != amount-1)
			{
				to_code_2R_I(
					enc->sx->io,
					IC_MIPS_ADDI,
					addr->val.reg_num,
					addr->val.reg_num,
					-4
				);
			}
			free_rvalue(enc, &subexpr_value);
		}
	}
}

/**
 * Emit bound expression in array declaration
 * 
 * @param	enc				Encoder
 * @param	bound			Bound node
 * @param	nd				Declaration node
 * 
 * @return	Bound expression
*/
static rvalue emit_bound(encoder *const enc, const node *const bound, const node *const nd)
{
	// Если границы у массива не проставлены -- смотрим на инициализатор, чтобы их узнать
	node dim_size = *bound;
	const size_t dim = declaration_variable_get_bounds_amount(nd);
	if (expression_get_class(bound) == EXPR_EMPTY_BOUND)
	{
		const node init = declaration_variable_get_initializer(nd);
		dim_size = expression_initializer_get_subexpr(&init, dim-1);
		const size_t size = expression_initializer_get_size(&init);
		const rvalue result = {
			.from_lvalue = !FROM_LVALUE,
			.kind = RVALUE_KIND_CONST,
			.val.int_val = (item_t)size,
			.type = TYPE_INTEGER
		};
		return emit_load_of_immediate(enc, &result);
	}
	return emit_expression(enc, &dim_size);
}

/**
 * Emit array declaration
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_array_declaration(encoder *const enc, const node *const nd)
{
	const size_t identifier = declaration_variable_get_id(nd);
	const bool has_init = declaration_variable_has_initializer(nd);

	// Сдвигаем, чтобы размер первого измерения был перед массивом
	to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, -4);
	const lvalue variable = displacements_add(enc, identifier, enc->scope_displ, false);
	const rvalue value = {
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_REGISTER,
		.val.reg_num = get_register(enc),
		.type = TYPE_INTEGER
	};
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, value.val.reg_num, R_SP);
	emit_store_of_rvalue(enc, &variable, &value);
	free_rvalue(enc, &value);

	// FIXME: Переделать регистры-аргументы
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_S0, R_A0);
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_S1, R_A1);
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_S2, R_A2);
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_S3, R_A3);

	// Загрузка адреса в $a0
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A0, R_SP);

	// Загрузка размера массива в $a1
	const node dim_size = declaration_variable_get_bound(nd, 0);
	const rvalue tmp = emit_expression(enc, &dim_size);
	const rvalue second_arg_rvalue = (tmp.kind == RVALUE_KIND_CONST) ? emit_load_of_immediate(enc, &tmp) : tmp;
	emit_move_rvalue_to_register(enc, R_A1, &second_arg_rvalue);
	free_rvalue(enc, &second_arg_rvalue);

	const size_t dim = declaration_variable_get_bounds_amount(nd);
	if (dim >= 2)
	{
		// Предварительно загрузим в $a2 и $a3 адрес первого элемента и размер соответственно
		to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A2, R_A0);
		to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A3, R_A1);
	}

	uni_printf(enc->sx->io, "\tjal DEFARR1\n");

	for (size_t j = 1; j < dim; j++)
	{
		// Загрузка адреса в $a0
		to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A0, R_V0);
		// Загрузка размера массива в $a1
		const node try_dim_size = declaration_variable_get_bound(nd, j);
		const rvalue bound = emit_bound(enc, &try_dim_size, nd);
		emit_move_rvalue_to_register(enc, R_A1, &bound);
		free_rvalue(enc, &bound);

		to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_S5, R_A0);
		to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_S6, R_A1);

		uni_printf(enc->sx->io, "\tjal DEFARR2\n");

		if (j != dim - 1)
		{
			// Предварительно загрузим в $a2 и $a3 адрес первого элемента и размер соответственно
			to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A2, R_T5);
			to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A3, R_T6);
		}
	}

	if (has_init)
	{
		uni_printf(enc->sx->io, "\n");

		const rvalue variable_value = emit_load_of_lvalue(enc, &variable);
		const node init = declaration_variable_get_initializer(nd);
		emit_array_init(enc, nd, 0, &init, &variable_value);

		free_rvalue(enc, &variable_value);
	}

	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_SP, R_V0);

	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A0, R_S0);
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A1, R_S1);
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A2, R_S2);
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_A3, R_S3);
}

/**
 * Emit struct initialization
 * 
 * @param	enc					Encoder
 * @param	nd					Node in AST
*/
static void emit_structure_init(encoder *const enc, const lvalue *const variable
	, const node *const initializer, size_t* const displ)
{
	const size_t members_amount = type_structure_get_member_amount(enc->sx, expression_get_type(initializer));
	for (size_t i = 0; i < members_amount; i++)
	{
		const node member = expression_initializer_get_subexpr(initializer, i);
		if (expression_get_class(&member) == EXPR_INITIALIZER)
		{
			emit_structure_init(enc, variable, &member, displ);
			continue;
		}

		const rvalue member_rvalue = emit_expression(enc, &member);

		const lvalue correct_target_lvalue = {
			.base_reg = variable->base_reg,
			.kind = variable->kind,
			.loc.displ = *displ,
			.type = member_rvalue.type
		};
		emit_store_of_rvalue(enc, &correct_target_lvalue, &member_rvalue);

		*displ += mips_type_size(enc->sx, member_rvalue.type);
	}
}

/**
 * Emit variable declaration
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_variable_declaration(encoder *const enc, const node *const nd)
{
	const size_t identifier = declaration_variable_get_id(nd);
	uni_printf(enc->sx->io, "\t# \"%s\" variable declaration:\n", ident_get_spelling(enc->sx, identifier));

	const item_t type = ident_get_type(enc->sx, identifier);
	if (type_is_array(enc->sx, type))
	{
		emit_array_declaration(enc, nd);
	}
	else
	{
		const bool is_register = false; // Тут должно быть что-то типо ident_is_register (если не откажемся)
		const lvalue variable = displacements_add(
			enc,
			identifier,
			// что-то такое, как будет поддержано: (is_register) ? <получение регистра> : info->scope_displ,
			enc->scope_displ,
			is_register
		);
		if (declaration_variable_has_initializer(nd))
		{
			const node initializer = declaration_variable_get_initializer(nd);

			if (type_is_structure(enc->sx, type))
			{
				// Инициализация списком
				if (expression_get_class(&initializer) == EXPR_INITIALIZER)
				{
					size_t var_displ = variable.loc.displ;
					emit_structure_init(enc, &variable, &initializer, &var_displ);
				}
				else
				{
					// Грузим адрес первого элемента на регистр
					// FIXME: возврат структуры из функции
					const size_t RHS_identifier = expression_identifier_get_id(&initializer);
					const size_t displ = hash_get(&enc->displacements, RHS_identifier, 1);
					const rvalue tmp = {
						.from_lvalue = !FROM_LVALUE,
						.kind = RVALUE_KIND_CONST,
						.val.int_val = displ,
						.type = TYPE_INTEGER
					};
					const rvalue RHS_addr_rvalue = emit_load_of_immediate(enc, &tmp);

					// Подсчёт размеров структуры
					const size_t struct_size = mips_type_size(enc->sx, type);
					const mips_register_t reg = get_register(enc);

					// Копирование всех данных из RHS 
					for (size_t i = 0; i < struct_size; i += WORD_LENGTH)
					{
						// Грузим данные из RHS
						uni_printf(enc->sx->io, "\t");
						instruction_to_io(enc->sx->io, IC_MIPS_LW);
						uni_printf(enc->sx->io, " ");
						mips_register_to_io(enc->sx->io, reg);
						uni_printf(enc->sx->io, ", %zu(", i);
						rvalue_to_io(enc, &RHS_addr_rvalue);
						uni_printf(enc->sx->io, ")\n");

						// Отправляем их в variable
						uni_printf(enc->sx->io, "\t");
						instruction_to_io(enc->sx->io, IC_MIPS_SW);
						uni_printf(enc->sx->io, " ");
						mips_register_to_io(enc->sx->io, reg);
						uni_printf(enc->sx->io, ", %" PRIitem "(", variable.loc.displ + i);
						mips_register_to_io(enc->sx->io, variable.base_reg);
						uni_printf(enc->sx->io, ")\n\n");
					}
					free_rvalue(enc, &RHS_addr_rvalue);
				}
			}
			else
			{
				const rvalue value = emit_expression(enc, &initializer);

				emit_store_of_rvalue(enc, &variable, &value);
				free_rvalue(enc, &value);
			}
		}
	}
}

/**
 * Emit function definition
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_function_definition(encoder *const enc, const node *const nd)
{
	const size_t ref_ident = declaration_function_get_id(nd);
	const label func_label = { .kind = L_FUNC, .num = ref_ident };
	emit_label_declaration(enc, &func_label);

	const item_t func_type = ident_get_type(enc->sx, ref_ident);
	const size_t parameters = type_function_get_parameter_amount(enc->sx, func_type);

	if (ref_ident == enc->sx->ref_main)
	{
		// FIXME: пока тут будут две метки для функции main
		uni_printf(enc->sx->io, "MAIN:\n");
	}

	uni_printf(enc->sx->io, "\t# \"%s\" function:\n", ident_get_spelling(enc->sx, ref_ident));

	enc->curr_function_ident = ref_ident;
	enc->max_displ = 0;
	enc->scope_displ = 0;

	// Сохранение оберегаемых регистров перед началом работы функции
	// FIXME: избавиться от функций to_code
	uni_printf(enc->sx->io, "\n\t# preserved registers:\n");
	to_code_R_I_R(enc->sx->io, IC_MIPS_SW, R_RA, -(item_t)RA_SIZE, R_SP);
	to_code_R_I_R(enc->sx->io, IC_MIPS_SW, R_FP, -(item_t)(RA_SIZE + SP_SIZE), R_SP);

	// Сохранение s0-s7
	for (size_t i = 0; i < PRESERVED_REG_AMOUNT; i++)
	{
		to_code_R_I_R(enc->sx->io, IC_MIPS_SW, R_S0 + i, -(item_t)(RA_SIZE + SP_SIZE + (i + 1) * WORD_LENGTH), R_SP);
	}

	uni_printf(enc->sx->io, "\n");

	// Сохранение fs0-fs10 (в цикле 5, т.к. операции одинарной точности => нужны только четные регистры)
	for (size_t i = 0; i < PRESERVED_FP_REG_AMOUNT/2; i++)
	{
		to_code_R_I_R(enc->sx->io, IC_MIPS_S_S, R_FS0 + 2 * i
			, -(item_t)(RA_SIZE + SP_SIZE + (i + 1) * WORD_LENGTH + PRESERVED_REG_AMOUNT*WORD_LENGTH /* за $s0-$s7 */)
			, R_SP);
	}

	// Сохранение $a0-$a3:
	for (size_t i = 0; i < ARG_REG_AMOUNT; i++)
	{
		to_code_R_I_R(enc->sx->io
			, IC_MIPS_SW, R_A0 + i
			, -(item_t)(RA_SIZE + SP_SIZE + (i + 1) * WORD_LENGTH + PRESERVED_REG_AMOUNT * WORD_LENGTH /* за $s0-$s7 */
				+ PRESERVED_FP_REG_AMOUNT/2 * WORD_LENGTH /* за $fs0-$fs10 */)
			, R_SP);
	}

	// Выравнивание смещения на 8
	if (enc->max_displ % 8)
	{
		const size_t padding = 8 - (enc->max_displ % 8);
		enc->max_displ += padding;
		if (padding)
		{
			uni_printf(enc->sx->io, "\n\t# padding -- max displacement == %zu\n", enc->max_displ);
		}
	}

	// Создание буфера для тела функции
	universal_io *const old_io = enc->sx->io;
	universal_io new_io = io_create();
	out_set_buffer(&new_io, BUFFER_SIZE);
	enc->sx->io = &new_io;

	uni_printf(enc->sx->io, "\n\t# function parameters:\n");

	size_t gpr_count = 0;
	size_t fp_count = 0;

	for (size_t i = 0; i < parameters; i++)
	{
		const size_t id = declaration_function_get_parameter(nd, i);
		uni_printf(enc->sx->io, "\t# parameter \"%s\" ", ident_get_spelling(enc->sx, id));

		if (!type_is_floating(ident_get_type(enc->sx, id)))
		{
			if (i < ARG_REG_AMOUNT)
			{
				// Рассматриваем их как регистровые переменные
				const mips_register_t curr_reg = R_A0 + gpr_count++;
				uni_printf(enc->sx->io, "is in register ");
				mips_register_to_io(enc->sx->io, curr_reg);
				uni_printf(enc->sx->io, "\n");

				// Вносим переменную в таблицу символов
				displacements_add(enc, id, curr_reg, true);
			}
			else
			{
				// TODO:
				assert(false);
			}
		}
		else
		{
			if (i < ARG_REG_AMOUNT/2)
			{
				// Рассматриваем их как регистровые переменные
				const mips_register_t curr_reg = R_FA0 + 2*fp_count++;
				uni_printf(enc->sx->io, "is in register ");
				mips_register_to_io(enc->sx->io, curr_reg);
				uni_printf(enc->sx->io, "\n");

				displacements_add(enc, id, curr_reg, true);
			}
			else
			{
				// TODO:
				assert(false);
			}
		}
	}

	uni_printf(enc->sx->io, "\n\t# function body:\n");
	node body = declaration_function_get_body(nd);
	emit_statement(enc, &body);

	// Извлечение буфера с телом функции в старый io
	char *buffer = out_extract_buffer(enc->sx->io);
	enc->sx->io = old_io;

	uni_printf(enc->sx->io, "\n\t# setting up $sp:\n");
	// $fp указывает на конец динамики (которое в данный момент равно концу статики)
	to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, -(item_t)(enc->max_displ + FUNC_DISPL_PRESEREVED + WORD_LENGTH));

	uni_printf(enc->sx->io, "\n\t# setting up $fp:\n");
	// $sp указывает на конец статики (которое в данный момент равно концу динамики)
	to_code_2R(enc->sx->io, IC_MIPS_MOVE, R_FP, R_SP);

	// Смещаем $fp ниже конца статики (чтобы он не совпадал с $sp)
	to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, -(item_t)WORD_LENGTH);

	uni_printf(enc->sx->io, "%s", buffer);
	free(buffer);

	const label end_label = { .kind = L_FUNCEND, .num = ref_ident };
	emit_label_declaration(enc, &end_label);

	// Восстановление стека после работы функции
	uni_printf(enc->sx->io, "\n\t# data restoring:\n");

	// Ставим $fp на его положение в предыдущей функции
	to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_FP,
				 (item_t)(enc->max_displ + FUNC_DISPL_PRESEREVED + WORD_LENGTH));

	uni_printf(enc->sx->io, "\n");

	// Восстановление $s0-$s7
	for (size_t i = 0; i < PRESERVED_REG_AMOUNT; i++)
	{
		to_code_R_I_R(enc->sx->io, IC_MIPS_LW, R_S0 + i, -(item_t)(RA_SIZE + SP_SIZE + (i + 1) * WORD_LENGTH), R_SP);
	}

	uni_printf(enc->sx->io, "\n");

	// Восстановление $fs0-$fs7
	for (size_t i = 0; i < PRESERVED_FP_REG_AMOUNT/2; i++)
	{
		to_code_R_I_R(enc->sx->io, IC_MIPS_L_S, R_FS0 + 2 * i
			, -(item_t)(RA_SIZE + SP_SIZE + (i + 1) * WORD_LENGTH + /* за s0-s7 */ 8 * WORD_LENGTH), R_SP);
	}

	// Восстановление $a0-$a3
	for (size_t i = 0; i < ARG_REG_AMOUNT; i++)
	{
		to_code_R_I_R(enc->sx->io, IC_MIPS_LW, R_A0 + i
			, -(item_t)(RA_SIZE + SP_SIZE + (i + 1) * WORD_LENGTH + 8 * WORD_LENGTH /* за s0-s7 */
				+ 5 * WORD_LENGTH /* за $fs0-$fs10*/)
			, R_SP);
	}

	uni_printf(enc->sx->io, "\n");

	// Возвращаем $sp его положение в предыдущей функции
	to_code_R_I_R(enc->sx->io, IC_MIPS_LW, R_FP, -(item_t)(RA_SIZE + SP_SIZE), R_SP);

	to_code_R_I_R(enc->sx->io, IC_MIPS_LW, R_RA, -(item_t)(RA_SIZE), R_SP);

	// Прыгаем далее
	emit_register_branch(enc, IC_MIPS_JR, R_RA);
}

static void emit_declaration(encoder *const enc, const node *const nd)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			emit_variable_declaration(enc, nd);
			break;

		case DECL_FUNC:
			emit_function_definition(enc, nd);
			break;

		default:
			// С объявлением типа ничего делать не нужно
			return;
	}

	uni_printf(enc->sx->io, "\n");
}


/*
 *	 ______     ______   ______     ______   ______     __    __     ______     __   __     ______   ______
 *	/\  ___\   /\__  _\ /\  __ \   /\__  _\ /\  ___\   /\ "-./  \   /\  ___\   /\ "-.\ \   /\__  _\ /\  ___\
 *	\ \___  \  \/_/\ \/ \ \  __ \  \/_/\ \/ \ \  __\   \ \ \-./\ \  \ \  __\   \ \ \-.  \  \/_/\ \/ \ \___  \
 *	 \/\_____\    \ \_\  \ \_\ \_\    \ \_\  \ \_____\  \ \_\ \ \_\  \ \_____\  \ \_\\"\_\    \ \_\  \/\_____\
 *	  \/_____/     \/_/   \/_/\/_/     \/_/   \/_____/   \/_/  \/_/   \/_____/   \/_/ \/_/     \/_/   \/_____/
 */


/**
 *	Emit declaration statement
 *
 *	@param	enc		encoder
 *	@param	nd			Node in AST
 */
static void emit_declaration_statement(encoder *const enc, const node *const nd)
{
	const size_t size = statement_declaration_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = statement_declaration_get_declarator(nd, i);
		emit_declaration(enc, &decl);
	}
}

/**
 * Emit compound statement
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_compound_statement(encoder *const enc, const node *const nd)
{
	const size_t scope_displacement = enc->scope_displ;

	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node substmt = statement_compound_get_substmt(nd, i);
		emit_statement(enc, &substmt);
	}

	enc->max_displ = max(enc->scope_displ, enc->max_displ);
	enc->scope_displ = scope_displacement;
}

/**
 *	Emit if statement
 *
 *	@param	enc				Encoder
 *	@param	nd				Node in AST
 */
static void emit_if_statement(encoder *const enc, const node *const nd)
{
	const node condition = statement_if_get_condition(nd);
	const rvalue value = emit_expression(enc, &condition);

	const size_t label_num = enc->label_num++;
	const label label_else = { .kind = L_ELSE, .num = label_num };
	const label label_end = { .kind = L_END, .num = label_num };

	const bool has_else = statement_if_has_else_substmt(nd);
	const mips_instruction_t instruction = IC_MIPS_BEQ;
	emit_conditional_branch(enc, instruction, &value, has_else ? &label_else : &label_end);
	free_rvalue(enc, &value);

	const node then_substmt = statement_if_get_then_substmt(nd);
	emit_statement(enc, &then_substmt);

	if (has_else)
	{
		emit_unconditional_branch(enc, IC_MIPS_J, &label_end);
		emit_label_declaration(enc, &label_else);

		const node else_substmt = statement_if_get_else_substmt(nd);
		emit_statement(enc, &else_substmt);
	}

	emit_label_declaration(enc, &label_end);
}

/**
 * Emit while statement
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_while_statement(encoder *const enc, const node *const nd)
{
	const size_t label_num = enc->label_num++;
	const label label_begin = { .kind = L_BEGIN_CYCLE, .num = label_num };
	const label label_end = { .kind = L_END, .num = label_num };

	const label old_continue = enc->label_continue;
	const label old_break = enc->label_break;

	enc->label_continue = label_begin;
	enc->label_break = label_end;

	emit_label_declaration(enc, &label_begin);

	const node condition = statement_while_get_condition(nd);
	const rvalue value = emit_expression(enc, &condition);

	const mips_instruction_t instruction = IC_MIPS_BEQ;
	emit_conditional_branch(enc, instruction, &value, &label_end);
	free_rvalue(enc, &value);

	const node body = statement_while_get_body(nd);
	emit_statement(enc, &body);

	emit_unconditional_branch(enc, IC_MIPS_J, &label_begin);
	emit_label_declaration(enc, &label_end);

	enc->label_continue = old_continue;
	enc->label_break = old_break;
}

/**
 * Emit do statement
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_do_statement(encoder *const enc, const node *const nd)
{
	const size_t label_num = enc->label_num++;
	const label label_begin = { .kind = L_BEGIN_CYCLE, .num = label_num };
	emit_label_declaration(enc, &label_begin);

	const label label_condition = { .kind = L_NEXT, .num = label_num };
	const label label_end = { .kind = L_END, .num = label_num };

	const label old_continue = enc->label_continue;
	const label old_break = enc->label_break;
	enc->label_continue = label_condition;
	enc->label_break = label_end;

	const node body = statement_do_get_body(nd);
	emit_statement(enc, &body);
	emit_label_declaration(enc, &label_condition);

	const node condition = statement_do_get_condition(nd);
	const rvalue value = emit_expression(enc, &condition);

	const mips_instruction_t instruction = IC_MIPS_BNE;
	emit_conditional_branch(enc, instruction, &value, &label_begin);
	emit_label_declaration(enc, &label_end);
	free_rvalue(enc, &value);

	enc->label_continue = old_continue;
	enc->label_break = old_break;
}

/**
 * Emit for statement
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_for_statement(encoder *const enc, const node *const nd)
{
	const size_t scope_displacement = enc->scope_displ;

	if (statement_for_has_inition(nd))
	{
		const node inition = statement_for_get_inition(nd);
		emit_statement(enc, &inition);
	}

	const size_t label_num = enc->label_num++;
	const label label_begin = { .kind = L_BEGIN_CYCLE, .num = label_num };
	const label label_end = { .kind = L_END, .num = label_num };

	const label old_continue = enc->label_continue;
	const label old_break = enc->label_break;
	enc->label_continue = label_begin;
	enc->label_break = label_end;

	emit_label_declaration(enc, &label_begin);
	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		const rvalue value = emit_expression(enc, &condition);
		const mips_instruction_t instruction = IC_MIPS_BEQ;
		emit_conditional_branch(enc, instruction, &value, &label_end);
		free_rvalue(enc, &value);
	}

	const node body = statement_for_get_body(nd);
	emit_statement(enc, &body);

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		emit_void_expression(enc, &increment);
	}

	emit_unconditional_branch(enc, IC_MIPS_J, &label_begin);
	emit_label_declaration(enc, &label_end);

	enc->label_continue = old_continue;
	enc->label_break = old_break;

	enc->scope_displ = scope_displacement;
}

/**
 *	Emit continue statement
 *
 *	@param	enc		encoder
 */
static void emit_continue_statement(encoder *const enc)
{
	emit_unconditional_branch(enc, IC_MIPS_J, &enc->label_continue);
}

/**
 *	Emit break statement
 *
 *	@param	enc		encoder
 */
static void emit_break_statement(encoder *const enc)
{
	emit_unconditional_branch(enc, IC_MIPS_J, &enc->label_break);
}

/**
 * Emit return statement
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_return_statement(encoder *const enc, const node *const nd)
{
	if (statement_return_has_expression(nd))
	{
		const node expression = statement_return_get_expression(nd);
		const rvalue value = emit_expression(enc, &expression);

		const item_t type = expression_get_type(nd);
		const lvalue return_lval = { .kind = LVALUE_KIND_REGISTER, .loc.reg_num = R_V0, .type = type };

		emit_store_of_rvalue(enc, &return_lval, &value);
		free_rvalue(enc, &value);
	}

	const label label_end = { .kind = L_FUNCEND, .num = enc->curr_function_ident };
	emit_unconditional_branch(enc, IC_MIPS_J, &label_end);
}

/**
 * Emit statement
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static void emit_statement(encoder *const enc, const node *const nd)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			emit_declaration_statement(enc, nd);
			break;

		case STMT_CASE:
			// emit_case_statement(enc, nd);
			break;

		case STMT_DEFAULT:
			// emit_default_statement(enc, nd);
			break;

		case STMT_COMPOUND:
			emit_compound_statement(enc, nd);
			break;

		case STMT_EXPR:
			emit_void_expression(enc, nd);
			break;

		case STMT_NULL:
			break;

		case STMT_IF:
			emit_if_statement(enc, nd);
			break;

		case STMT_SWITCH:
			// emit_switch_statement(enc, nd);
			break;

		case STMT_WHILE:
			emit_while_statement(enc, nd);
			break;

		case STMT_DO:
			emit_do_statement(enc, nd);
			break;

		case STMT_FOR:
			emit_for_statement(enc, nd);
			break;

		case STMT_CONTINUE:
			emit_continue_statement(enc);
			break;

		case STMT_BREAK:
			emit_break_statement(enc);
			break;

		case STMT_RETURN:
			emit_return_statement(enc, nd);
			break;

		default:
			break;
	}

	uni_printf(enc->sx->io, "\n");
}

/**
 * Emit translation unit
 *
 * @param	enc				Encoder
 * @param	nd				Node in AST
 */
static int emit_translation_unit(encoder *const enc, const node *const nd)
{
	const size_t size = translation_unit_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = translation_unit_get_declaration(nd, i);
		emit_declaration(enc, &decl);
	}

	return enc->sx->rprt.errors != 0;
}

// В дальнейшем при необходимости сюда можно передавать флаги вывода директив
// TODO: подписать, что значит каждая директива и команда
static void pregen(syntax *const sx)
{
	// Подпись "GNU As:" для директив GNU
	// Подпись "MIPS Assembler:" для директив ассемблера MIPS

	uni_printf(sx->io, "\t.section .mdebug.abi32\n"); // ?
	uni_printf(sx->io, "\t.previous\n"); // следующая инструкция будет перенесена в секцию, описанную выше
	uni_printf(sx->io, "\t.nan\tlegacy\n");		  // ?
	uni_printf(sx->io, "\t.module fp=xx\n");	  // ?
	uni_printf(sx->io, "\t.module nooddspreg\n"); // ?
	uni_printf(sx->io, "\t.abicalls\n");		  // ?
	uni_printf(sx->io, "\t.option pic0\n"); // как если бы при компиляции была включена опция "-fpic" (что означает?)
	uni_printf(sx->io, "\t.text\n"); // последующий код будет перенесён в текстовый сегмент памяти
	// выравнивание последующих данных/команд по границе, кратной 2^n байт (в данном случае 2^2 = 4)
	uni_printf(sx->io, "\t.align 2\n");

	// делает метку main глобальной -- её можно вызывать извне кода (например, используется при линковке)
	uni_printf(sx->io, "\n\t.globl\tmain\n");
	uni_printf(sx->io, "\t.ent\tmain\n");			  // начало процедуры main
	uni_printf(sx->io, "\t.type\tmain, @function\n"); // тип "main" -- функция
	uni_printf(sx->io, "main:\n");

	// инициализация gp
	// "__gnu_local_gp" -- локация в памяти, где лежит Global Pointer
	uni_printf(sx->io, "\tlui $gp, %%hi(__gnu_local_gp)\n");
	uni_printf(sx->io, "\taddiu $gp, $gp, %%lo(__gnu_local_gp)\n");

	to_code_2R(sx->io, IC_MIPS_MOVE, R_FP, R_SP);
	to_code_2R_I(sx->io, IC_MIPS_ADDI, R_SP, R_SP, -4);
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_RA, 0, R_SP);
	to_code_R_I(sx->io, IC_MIPS_LI, R_T0, LOW_DYN_BORDER);
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_T0, -(item_t)HEAP_DISPL - 60, R_GP);
	uni_printf(sx->io, "\n");
}

// создаём метки всех строк в программе
static void strings_declaration(encoder *const enc)
{
	uni_printf(enc->sx->io, "\t.rdata\n");
	uni_printf(enc->sx->io, "\t.align 2\n");

	const size_t amount = strings_amount(enc->sx);
	for (size_t i = 0; i < amount; i++)
	{
		item_t args_for_printf = 0;
		const label string_label = { .kind = L_STRING, .num = i };
		emit_label_declaration(enc, &string_label);
		uni_printf(enc->sx->io, "\t.ascii \"");

		const char *string = string_get(enc->sx, i);
		for (size_t j = 0; string[j] != '\0'; j++)
		{
			const char ch = string[j];
			if (ch == '\n')
			{
				uni_printf(enc->sx->io, "\\n");
			}
			else if (ch == '%')
			{
				args_for_printf++;
				j++;

				uni_printf(enc->sx->io, "%c", ch);
				uni_printf(enc->sx->io, "%c", string[j]);

				uni_printf(enc->sx->io, "\\0\"\n");
				const label another_str_label = { .kind = L_STRING, .num = i + args_for_printf*amount };
				emit_label_declaration(enc, &another_str_label);
				uni_printf(enc->sx->io, "\t.ascii \"");
			}
			else
			{
				uni_printf(enc->sx->io, "%c", ch);
			}
		}

		uni_printf(enc->sx->io, "\\0\"\n");
	}
	uni_printf(enc->sx->io, "\t.text\n");
	uni_printf(enc->sx->io, "\t.align 2\n\n");

	// Прыжок на главную метку
	uni_printf(enc->sx->io, "\tjal MAIN\n");
	
	// Выход из программы в конце работы
	to_code_R_I_R(enc->sx->io, IC_MIPS_LW, R_RA, 0, R_SP);
	emit_register_branch(enc, IC_MIPS_JR, R_RA);
}

static void postgen(encoder *const enc)
{
	// вставляем runtime.s в конец файла
	/*
	uni_printf(enc->sx->io, "\n\n# runtime\n");
	char *runtime = "../runtimeMIPS/runtime.s";
	FILE *file = fopen(runtime, "r+");
	if (runtime != NULL)
	{
		char string[1024];
		while (fgets(string, sizeof(string), file) != NULL)
		{
			uni_printf(enc->sx->io, "%s", string);
		}
	}
	fclose(file);
	uni_printf(enc->sx->io, "# runtime end\n\n");
	*/

	uni_printf(enc->sx->io, "\n\n# defarr\n");
	char *defarr = "../runtimeMIPS/defarr.s";
	FILE *file = fopen(defarr, "r+");
	if (defarr != NULL)
	{
		char string[1024];
		while (fgets(string, sizeof(string), file) != NULL)
		{
			uni_printf(enc->sx->io, "%s", string);
		}
	}
	fclose(file);

	uni_printf(enc->sx->io, "\n\n\t.end\tmain\n");
	uni_printf(enc->sx->io, "\t.size\tmain, .-main\n");
}


/*
 *	 __	    __   __	    ______   ______	    ______	   ______   ______	   ______	  ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\	  \ \_\  \ \_____\  \ \_\ \_\  \ \_\	\ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/	   \/_/   \/_____/   \/_/ /_/   \/_/	 \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_mips(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}

	encoder enc;
	enc.sx = sx;
	enc.next_register = R_T0;
	enc.next_float_register = R_FT0;
	enc.label_num = 1;

	enc.scope_displ = 0;
	enc.global_displ = 0;

	enc.displacements = hash_create(HASH_TABLE_SIZE);

	for (size_t i = 0; i < TEMP_REG_AMOUNT + TEMP_FP_REG_AMOUNT; i++)
	{
		enc.registers[i] = false;
	}

	pregen(sx);
	strings_declaration(&enc);
	// TODO: нормальное получение корня
	const node root = node_get_root(&enc.sx->tree);
	const int ret = emit_translation_unit(&enc, &root);
	postgen(&enc);

	hash_clear(&enc.displacements);
	return ret;
}