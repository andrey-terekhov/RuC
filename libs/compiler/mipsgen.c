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


static const size_t BUFFER_SIZE = 65536;			/**< Размер буфера для тела функции */
static const size_t HASH_TABLE_SIZE = 1024;			/**< Размер хеш-таблицы для смещений и регистров */
static const bool IS_ON_STACK = true;				/**< Хранится ли переменная на стеке */

static const size_t WORD_LENGTH = 4;				/**< Длина слова данных */
static const size_t HALF_WORD_LENGTH = 2;			/**< Длина половины слова данных */

static const size_t LOW_DYN_BORDER = 0x10010000;	/**< Нижняя граница динамической памяти */
static const size_t HEAP_DISPL = 8000;				/**< Смещение кучи относительно глобальной памяти */

static const size_t SP_SIZE = 4;					/**< Размер регистра $sp для его сохранения */
static const size_t RA_SIZE = 4;					/**< Размер регистра $ra для его сохранения */

static const size_t TEMP_FP_REG_AMOUNT = 12;		/**< Количество временных регистров для чисел с плавающей точкой */
static const size_t TEMP_REG_AMOUNT = 10;			/**< Количество обычных временных регистров */
static const size_t ARG_REG_AMOUNT = 4;				/**< Количество регистров-аргументов для функций */

static const size_t PRESERVED_REG_AMOUNT = 8;		/**< Количество сохраняемых регистров общего назначения */
static const size_t PRESERVED_FP_REG_AMOUNT = 10;	/**< Количество сохраняемых регистров с плавающей точкой */

static const bool FROM_LVALUE = 1;					/**< Получен ли rvalue из lvalue */

/**< Смещение в стеке для сохранения оберегаемых регистров, без учёта оптимизаций */
static const size_t FUNC_DISPL_PRESEREVED = /* за $sp */ 4 + /* за $ra */ 4 +
											/* fs0-fs10 (одинарная точность): */ 5 * 4 + /* s0-s7: */ 8 * 4;


// Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition
typedef enum MIPS_REGISTER
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
							calling position independent functions $25 (R_T9) must contain
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
} mips_register_t;

// Назначение команд взято из документации MIPS® Architecture for Programmers
// Volume II-A: The MIPS32® Instruction
// Set Manual 2016
typedef enum INSTRUCTION
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

	MIPS_IC_BLEZ,		/**< Branch on Less Than or Equal to Zero.
							To test a GPR then do a PC-relative conditional branch */
	MIPS_IC_BLTZ,		/**< Branch on Less Than Zero.
							To test a GPR then do a PC-relative conditional branch */
	MIPS_IC_BGEZ,		/**< Branch on Greater Than or Equal to Zero.
							To test a GPR then do a PC-relative conditional branch */
	MIPS_IC_BGTZ,		/**< Branch on Greater Than Zero.
							To test a GPR then do a PC-relative conditional branch */
	MIPS_IC_BEQ,		/**< Branch on Equal.
							To compare GPRs then do a PC-relative conditional branch */
	MIPS_IC_BNE,		/**< Branch on Not Equal.
							To compare GPRs then do a PC-relative conditional branch */

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
} mips_instruction_t;


typedef enum LABEL
{
	L_MAIN,				/**< Тип метки -- главная функция */
	L_FUNC,				/**< Тип метки -- вход в функцию */
	L_NEXT,				/**< Тип метки -- следующая функция */
	L_FUNCEND,			/**< Тип метки -- выход из функции */
	L_STRING,			/**< Тип метки -- строка */
	L_ELSE,				/**< Тип метки -- переход по else */
	L_END,				/**< Тип метки -- переход в конец конструкции */
	L_BEGIN_CYCLE,		/**< Тип метки -- переход в начало цикла */
	L_CASE,				/**< Тип метки -- переход по case */
} mips_label_t;

typedef struct label
{
	mips_label_t kind;
	size_t num;
} label;


/** Kinds of lvalue */
typedef enum LVALUE_KIND
{
	LVALUE_KIND_STACK,
	LVALUE_KIND_REGISTER,
} lvalue_kind_t;

typedef struct lvalue
{
	lvalue_kind_t kind;				/**< Value kind */
	mips_register_t base_reg;			/**< Base register */
	union								/**< Value location */
	{
		item_t reg_num;						/**< Register where the value is stored */
		item_t displ;						/**< Stack displacement where the value is stored */
	} loc;
	item_t type;						/**< Value type */
} lvalue;


/** Kinds of rvalue */
typedef enum RVALUE_KIND
{
	RVALUE_KIND_CONST,						// Значит, запомнили константу и потом обработали её
	RVALUE_KIND_REGISTER,
	RVALUE_KIND_VOID,
} rvalue_kind_t;

typedef struct rvalue
{
	const rvalue_kind_t kind;				/**< Value kind */
	const item_t type;						/**< Value type */
	const bool from_lvalue;					/**< Was the rvalue instance formed from lvalue */
	const union
	{
		item_t reg_num;						/**< Where the value is stored */
		item_t int_val;						/**< Value of integer (character, boolean) literal */
		double float_val;					/**< Value of floating literal */
		item_t str_index;					/**< Index of pre-declared string */
	} val;
} rvalue;


typedef struct encoder
{
	syntax *sx;								/**< Структура syntax с таблицами */

	size_t max_displ;						/**< Максимальное смещение от $sp */
	size_t global_displ;					/**< Смещение от $gp */

	hash displacements;						/**< Хеш таблица с информацией о расположении идентификаторов:
												@c key		- ссылка на таблицу идентификаторов
												@c value[0]	- флаг, лежит ли переменная на стеке или в регистре
												@c value[1]	- смещение или номер регистра */

	mips_register_t next_register;			/**< Следующий обычный регистр для выделения */
	mips_register_t next_float_register;	/**< Следующий регистр с плавающей точкой для выделения */

	size_t label_num;						/**< Номер метки */
	size_t case_label_num;					/**< Номер метки-перехода по case */
	label label_else;						/**< Метка перехода на else */
	label label_continue;					/**< Метка continue */
	label label_break;						/**< Метка break */
	size_t curr_function_ident;				/**< Идентификатор текущей функций */

	bool registers[22];						/**< Информация о занятых регистрах */

	size_t scope_displ;						/**< Смещение */
} encoder;


static const rvalue RVALUE_ONE = { .kind = RVALUE_KIND_CONST, .type = TYPE_INTEGER, .val.int_val = 1 };
static const rvalue RVALUE_NEGATIVE_ONE = { .kind = RVALUE_KIND_CONST, .type = TYPE_INTEGER, .val.int_val = -1 };
static const rvalue RVALUE_ZERO = { .kind = RVALUE_KIND_CONST, .type = TYPE_INTEGER, .val.int_val = 0 };
static const rvalue RVALUE_VOID = { .kind = RVALUE_KIND_CONST };



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
 *	Locks certain register
 *
 *	@param	enc					Encoder
 *	@param	reg					Register to lock
 */
static void lock_register(encoder *const enc, const mips_registeMIPS_R_t reg)
{
	switch (reg)
	{
		case MIPS_R_T0:
		case MIPS_R_T1:
		case MIPS_R_T2:
		case MIPS_R_T3:
		case MIPS_R_T4:
		case MIPS_R_T5:
		case MIPS_R_T6:
		case MIPS_R_T7:
			if (!enc->registers[reg - MIPS_R_T0])
			{
				// Регистр занят => освобождаем
				enc->registers[reg - MIPS_R_T0] = true;
			}
			return;

		case MIPS_R_T8:
		case MIPS_R_T9:
			if (!enc->registers[reg - MIPS_R_T8 + /* индекс MIPS_R_T8 в enc->registers */ 8])
			{
				enc->registers[reg - MIPS_R_T8 + 8] = true;
			}
			return;

		case MIPS_R_FT0:
		case MIPS_R_FT1:
		case MIPS_R_FT2:
		case MIPS_R_FT3:
		case MIPS_R_FT4:
		case MIPS_R_FT5:
		case MIPS_R_FT6:
		case MIPS_R_FT7:
		case MIPS_R_FT8:
		case MIPS_R_FT9:
		case MIPS_R_FT10:
		case MIPS_R_FT11:
			if (!enc->registers[reg - MIPS_R_FT0 + /* индекс MIPS_R_FT0 в enc->registers */ TEMP_REG_AMOUNT])
			{
				enc->registers[reg - MIPS_R_FT0 + TEMP_REG_AMOUNT] = true;
			}
			return;

		default:	// Не временный регистр и пришли сюда => и так захвачен
			return;
	}
}

/**
 *	Takes the first free register
 *
 *	@param	enc					Encoder
 *
 *	@return	General purpose register
 */
static mips_register_t get_register(encoder *const enc)
{
	// Ищем первый свободный регистр
	size_t i = 0;
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
 *	Takes the first free floating point register
 *
 *	@param	enc					Encoder
 *
 *	@return	Register			Floating point register
 */
static mips_register_t get_float_register(encoder *const enc)
{
	// Ищем первый свободный регистр
	size_t i = TEMP_REG_AMOUNT;
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
 *	Free register
 *
 *	@param	enc					Encoder
 *	@param	reg					Register to set as free
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

		default:	// Не временный регистр => освобождать не надо
			return;
	}
}

/**
 *	Free register occupied by rvalue
 *
 *	@param	enc					Encoder
 *	@param	rval				Rvalue to be freed
 */
static void free_rvalue(encoder *const enc, const rvalue *const rval)
{
	if ((rval->kind == RVALUE_KIND_REGISTER) && (!rval->from_lvalue))
	{
		free_register(enc, rval->val.reg_num);
	}
}

/**	Get MIPS assembler binary instruction from binary_t type
 *
 *	@param	operation_type		Type of operation in AST
 *	@param	is_imm				@c True if the instruction is immediate, @c False otherwise
 *
 *	@return	MIPS binary instruction
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
		case MIPS_R_ZERO:
			uni_printf(io, "$0");
			break;
		case MIPS_R_AT:
			uni_printf(io, "$at");
			break;

		case MIPS_R_V0:
			uni_printf(io, "$v0");
			break;
		case MIPS_R_V1:
			uni_printf(io, "$v1");
			break;

		case MIPS_R_A0:
			uni_printf(io, "$a0");
			break;
		case MIPS_R_A1:
			uni_printf(io, "$a1");
			break;
		case MIPS_R_A2:
			uni_printf(io, "$a2");
			break;
		case MIPS_R_A3:
			uni_printf(io, "$a3");
			break;

		case MIPS_R_T0:
			uni_printf(io, "$t0");
			break;
		case MIPS_R_T1:
			uni_printf(io, "$t1");
			break;
		case MIPS_R_T2:
			uni_printf(io, "$t2");
			break;
		case MIPS_R_T3:
			uni_printf(io, "$t3");
			break;
		case MIPS_R_T4:
			uni_printf(io, "$t4");
			break;
		case MIPS_R_T5:
			uni_printf(io, "$t5");
			break;
		case MIPS_R_T6:
			uni_printf(io, "$t6");
			break;
		case MIPS_R_T7:
			uni_printf(io, "$t7");
			break;

		case MIPS_R_S0:
			uni_printf(io, "$s0");
			break;
		case MIPS_R_S1:
			uni_printf(io, "$s1");
			break;
		case MIPS_R_S2:
			uni_printf(io, "$s2");
			break;
		case MIPS_R_S3:
			uni_printf(io, "$s3");
			break;
		case MIPS_R_S4:
			uni_printf(io, "$s4");
			break;
		case MIPS_R_S5:
			uni_printf(io, "$s5");
			break;
		case MIPS_R_S6:
			uni_printf(io, "$s6");
			break;
		case MIPS_R_S7:
			uni_printf(io, "$s7");
			break;

		case MIPS_R_T8:
			uni_printf(io, "$t8");
			break;
		case MIPS_R_T9:
			uni_printf(io, "$t9");
			break;

		case MIPS_R_K0:
			uni_printf(io, "$k0");
			break;
		case MIPS_R_K1:
			uni_printf(io, "$k1");
			break;

		case MIPS_R_GP:
			uni_printf(io, "$gp");
			break;
		case MIPS_R_SP:
			uni_printf(io, "$sp");
			break;
		case MIPS_R_FP:
			uni_printf(io, "$fp");
			break;
		case MIPS_R_RA:
			uni_printf(io, "$ra");
			break;

		case MIPS_R_FV0:
			uni_printf(io, "$f0");
			break;
		case MIPS_R_FV1:
			uni_printf(io, "$f1");
			break;
		case MIPS_R_FV2:
			uni_printf(io, "$f2");
			break;
		case MIPS_R_FV3:
			uni_printf(io, "$f3");
			break;

		case MIPS_R_FT0:
			uni_printf(io, "$f4");
			break;
		case MIPS_R_FT1:
			uni_printf(io, "$f5");
			break;
		case MIPS_R_FT2:
			uni_printf(io, "$f6");
			break;
		case MIPS_R_FT3:
			uni_printf(io, "$f7");
			break;
		case MIPS_R_FT4:
			uni_printf(io, "$f8");
			break;
		case MIPS_R_FT5:
			uni_printf(io, "$f9");
			break;
		case MIPS_R_FT6:
			uni_printf(io, "$f10");
			break;
		case MIPS_R_FT7:
			uni_printf(io, "$f11");
			break;
		case MIPS_R_FT8:
			uni_printf(io, "$f16");
			break;
		case MIPS_R_FT9:
			uni_printf(io, "$f17");
			break;
		case MIPS_R_FT10:
			uni_printf(io, "$f18");
			break;
		case MIPS_R_FT11:
			uni_printf(io, "$f19");
			break;

		case MIPS_R_FA0:
			uni_printf(io, "$f12");
			break;
		case MIPS_R_FA1:
			uni_printf(io, "$f13");
			break;
		case MIPS_R_FA2:
			uni_printf(io, "$f14");
			break;
		case MIPS_R_FA3:
			uni_printf(io, "$f15");
			break;

		case MIPS_R_FS0:
			uni_printf(io, "$f20");
			break;
		case MIPS_R_FS1:
			uni_printf(io, "$f21");
			break;
		case MIPS_R_FS2:
			uni_printf(io, "$f22");
			break;
		case MIPS_R_FS3:
			uni_printf(io, "$f23");
			break;
		case MIPS_R_FS4:
			uni_printf(io, "$f24");
			break;
		case MIPS_R_FS5:
			uni_printf(io, "$f25");
			break;
		case MIPS_R_FS6:
			uni_printf(io, "$f26");
			break;
		case MIPS_R_FS7:
			uni_printf(io, "$f27");
			break;
		case MIPS_R_FS8:
			uni_printf(io, "$f28");
			break;
		case MIPS_R_FS9:
			uni_printf(io, "$f29");
			break;
		case MIPS_R_FS10:
			uni_printf(io, "$f30");
			break;
		case MIPS_R_FS11:
			uni_printf(io, "$f31");
			break;
	}
}

static void mips_instr_to_io(universal_io *const io, const mips_instr instr)
{
	switch (instr)
	{
		case MIPS_IC_MOVE:
			uni_printf(io, "move");
			break;
		case MIPS_IC_LI:
			uni_printf(io, "li");
			break;
		case MIPS_IC_LA:
			uni_printf(io, "la");
			break;
		case MIPS_IC_NOT:
			uni_printf(io, "not");
			break;

		case MIPS_IC_ADDI:
			uni_printf(io, "addi");
			break;
		case MIPS_IC_SLL:
			uni_printf(io, "sll");
			break;
		case MIPS_IC_SRA:
			uni_printf(io, "sra");
			break;
		case MIPS_IC_ANDI:
			uni_printf(io, "andi");
			break;
		case MIPS_IC_XORI:
			uni_printf(io, "xori");
			break;
		case MIPS_IC_ORI:
			uni_printf(io, "ori");
			break;

		case MIPS_IC_ADD:
			uni_printf(io, "add");
			break;
		case MIPS_IC_SUB:
			uni_printf(io, "sub");
			break;
		case MIPS_IC_MUL:
			uni_printf(io, "mul");
			break;
		case MIPS_IC_DIV:
			uni_printf(io, "div");
			break;
		case MIPS_IC_MOD:
			uni_printf(io, "mod");
			break;
		case MIPS_IC_SLLV:
			uni_printf(io, "sllv");
			break;
		case MIPS_IC_SRAV:
			uni_printf(io, "srav");
			break;
		case MIPS_IC_AND:
			uni_printf(io, "and");
			break;
		case MIPS_IC_XOR:
			uni_printf(io, "xor");
			break;
		case MIPS_IC_OR:
			uni_printf(io, "or");
			break;

		case MIPS_IC_SW:
			uni_printf(io, "sw");
			break;
		case MIPS_IC_LW:
			uni_printf(io, "lw");
			break;

		case MIPS_IC_JR:
			uni_printf(io, "jr");
			break;
		case MIPS_IC_JAL:
			uni_printf(io, "jal");
			break;
		case MIPS_IC_J:
			uni_printf(io, "j");
			break;

		case MIPS_IC_BLEZ:
			uni_printf(io, "blez");
			break;
		case MIPS_IC_BLTZ:
			uni_printf(io, "bltz");
			break;
		case MIPS_IC_BGEZ:
			uni_printf(io, "bgez");
			break;
		case MIPS_IC_BGTZ:
			uni_printf(io, "bgtz");
			break;
		case MIPS_IC_BEQ:
			uni_printf(io, "beq");
			break;
		case MIPS_IC_BNE:
			uni_printf(io, "bne");
			break;

		case MIPS_IC_SLTIU:
			uni_printf(io, "sltiu");
			break;

		case MIPS_IC_NOP:
			uni_printf(io, "nop");
			break;

		case MIPS_IC_ADD_S:
			uni_printf(io, "add.s");
			break;
		case MIPS_IC_SUB_S:
			uni_printf(io, "sub.s");
			break;
		case MIPS_IC_MUL_S:
			uni_printf(io, "mul.s");
			break;
		case MIPS_IC_DIV_S:
			uni_printf(io, "div.s");
			break;

		case MIPS_IC_ABS_S:
			uni_printf(io, "abs.s");
			break;
		case MIPS_IC_ABS:
			uni_printf(io, "abs");
			break;

		case MIPS_IC_S_S:
			uni_printf(io, "s.s");
			break;
		case MIPS_IC_L_S:
			uni_printf(io, "l.s");
			break;

		case MIPS_IC_LI_S:
			uni_printf(io, "li.s");
			break;

		case MIPS_IC_MOV_S:
			uni_printf(io, "mov.s");
			break;

		case MIPS_IC_MFC_1:
			uni_printf(io, "mfc1");
			break;
		case MIPS_IC_MFHC_1:
			uni_printf(io, "mfhc1");
			break;

		case MIPS_IC_CVT_D_S:
			uni_printf(io, "cvt.d.s");
			break;
		case MIPS_IC_CVT_S_W:
			uni_printf(io, "cvt.s.w");
			break;
		case MIPS_IC_CVT_W_S:
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
 *	Writes @c val field of rvalue structure to io
 *
 *	@param	io					Universal io structure
 *	@param	rval				Rvalue whose value is to be printed
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
 *	Writes rvalue to io
 *
 *	@param	enc					Encoder
 *	@param	rval				Rvalue to write
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
 *	Writes lvalue to io
 *
 *	@param	enc					Encoder
 *	@param	lvalue				Lvalue
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
 *	Add new identifier to displacements table
 *
 *	@param	enc					Encoder
 *	@param	identifier			Identifier for adding to the table
 *	@param	location			Location of identifier - register, or displacement on stack
 *	@param	is_register			@c true, if identifier is register variable, and @c false otherwise
 *
 *	@return	Identifier lvalue
 */
static lvalue displacements_add(encoder *const enc, const size_t identifier
	, const item_t location, const bool is_register)
{
	const size_t displacement = enc->scope_displ;
	const bool is_local = ident_is_local(enc->sx, identifier);
	const mips_register_t base_reg = is_local ? R_FP : R_GP;
	const item_t type = ident_get_type(enc->sx, identifier);

	if ((!is_local) && (is_register))	// Запрет на глобальные регистровые переменные
	{
		// TODO: кидать соответствующую ошибку
		system_error(node_unexpected);
	}

	const size_t index = hash_add(&enc->displacements, identifier, 3);
	hash_set_by_index(&enc->displacements, index, 0, (is_register) ? 1 : 0);
	hash_set_by_index(&enc->displacements, index, 1, location);
	hash_set_by_index(&enc->displacements, index, 2, base_reg);

	if (!is_local)
	{
		enc->global_displ += mips_type_size(enc->sx, type);
	}
	else if (!is_register)
	{
		enc->scope_displ += mips_type_size(enc->sx, type);
		enc->max_displ = max(enc->scope_displ, enc->max_displ);
	}

	return (lvalue) { .kind = LVALUE_KIND_STACK, .base_reg = base_reg, .loc.displ = displacement, .type = type };
}

/**
 *	Return lvalue for the given identifier
 *
 *	@param	enc					Encoder
 *	@param	identifier			Identifier in the table
 *
 *	@return	Identifier lvalue
 */
static lvalue displacements_get(encoder *const enc, const size_t identifier)
{
	const bool is_register = (hash_get(&enc->displacements, identifier, 0) == 1);
	const size_t displacement = (size_t)hash_get(&enc->displacements, identifier, 1);
	const mips_register_t base_reg = hash_get(&enc->displacements, identifier, 2);
	const item_t type = ident_get_type(enc->sx, identifier);

	const lvalue_kind_t kind = (is_register) ? LVALUE_KIND_REGISTER : LVALUE_KIND_STACK;

	return (lvalue) { .kind = kind, .base_reg = base_reg, .loc.displ = displacement, .type = type };
}

/**
 *	Emit label
 *
 *	@param	enc					Encoder
 *	@param	label				Label for emitting
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
		case L_CASE:
			uni_printf(io, "CASE");
			break;
	}

	uni_printf(io, "%zu", lbl->num);
}

/**
 *	Emit label declaration
 *
 *	@param	enc					Encoder
 *	@param	label				Declared label
 */
static void emit_label_declaration(encoder *const enc, const label *const lbl)
{
	emit_label(enc, lbl);
	uni_printf(enc->sx->io, ":\n");
}

/**
 *	Emit unconditional branch
 *
 *	@param	enc					Encoder
 *	@param	label				Label for unconditional jump
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
 *	Emit conditional branch
 *
 *	@param	enc					Encoder
 *	@param	label				Label for conditional jump
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
 *	Emit branching with register
 *
 *	@param	enc					Encoder
 *	@param	reg					Register
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
 *	Creates register kind rvalue and stores there constant kind rvalue
 *
 *	@param	enc					Encoder
 *	@param	value				Rvalue of constant kind
 *
 *	@return	Created rvalue
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
 *	Loads lvalue to register and forms rvalue. If lvalue kind is @c LVALUE_KIND_REGISTER,
 *	returns rvalue on the same register
 *
 *	@param	enc					Encoder
 *	@param	lval				Lvalue to load
 *
 *	@return	Formed rvalue
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
 *	Emit identifier lvalue
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Identifier lvalue
 */
static lvalue emit_identifier_lvalue(encoder *const enc, const node *const nd)
{
	return displacements_get(enc, expression_identifier_get_id(nd));
}

/**
 *	Emit subscript lvalue
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Subscript lvalue
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

	const rvalue type_size_value = {	// Можно было бы сделать отдельным конструктором
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_CONST,
		.val.int_val = mips_type_size(enc->sx, type),
		.type = TYPE_INTEGER
	};
	const rvalue offset = {
		.from_lvalue = !FROM_LVALUE,
		.kind = RVALUE_KIND_REGISTER,
		.val.reg_num = get_register(enc),
		.type = TYPE_INTEGER
	};

	emit_binary_operation(enc, &offset, &index_value, &type_size_value, BIN_MUL);
	free_rvalue(enc, &index_value);

	emit_binary_operation(enc, &base_value, &base_value, &offset, BIN_SUB);
	free_rvalue(enc, &offset);

	return (lvalue) { .kind = LVALUE_KIND_STACK, .base_reg = base_value.val.reg_num, .loc.displ = 0, .type = type };
}

/**
 *	Emit member lvalue
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Created lvalue
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
		const size_t displ = (size_t)(base_lvalue.loc.displ + member_displ);
		return (lvalue) {
			.kind = LVALUE_KIND_STACK,
			.base_reg = base_lvalue.base_reg,
			.loc.displ = displ,
			.type = type
		};
	}
}

/**
 *	Emit indirection lvalue
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Indirected lvalue
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
 *	Emit lvalue expression
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Lvalue
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

		case EXPR_UNARY:	// Только UN_INDIRECTION
			return emit_indirection_lvalue(enc, nd);

		default:
			// Не может быть lvalue
			system_error(node_unexpected, nd);
			return (lvalue) { .loc.displ = ITEM_MAX };
	}
}

/**
 *	Stores one rvalue of register kind to another
 *
 *	@param	enc					Encoder
 *	@param	target				Target register
 *	@param	value				Rvalue to store
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
 *	Stores rvalue to lvalue. Frees value parameter register for a pointer
 *
 *	@param	enc					Encoder
 *	@param	target				Target lvalue
 *	@param	value				Rvalue to store
 */
static void emit_store_of_rvalue(encoder *const enc, const lvalue *const target, const rvalue *const value)
{
	assert(value->kind != RVALUE_KIND_VOID);
	assert(value->type == target->type);

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
 *	Emit binary operation with two rvalues
 *
 *	@param	enc					Encoder
 *	@param	dest				Destination rvalue
 *	@param	first_operand		First rvalue operand
 *	@param	second_operand		Second rvalue operand
 *	@param	operator			Operator
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
				const label label_else = { .kind = L_END, .num = (size_t)curr_label_num };

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
				uni_printf(enc->sx->io, ", 1\n");

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
				const label label_else = { .kind = L_ELSE, .num = (size_t)curr_label_num };

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
				uni_printf(enc->sx->io, ", 1\n");

				emit_label_declaration(enc, &label_else);

				uni_printf(enc->sx->io, "\n");
				break;
			}

			default:
			{
				bool in_reg = false;
				// Предварительно загружаем константу из imm_rvalue в rvalue вида RVALUE_KIND_REGISTER
				if ((operator == BIN_SUB) || (operator == BIN_DIV) || (operator == BIN_MUL) || (operator == BIN_REM))
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
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Rvalue of literal expression
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
 *	Emit printf expression
 *
 *	@param	enc					Encoder
 *	@param	nd					AST node
 *	@param	parameters_amount	Number of function parameters
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
 *	Emit builtin function call
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Rvalue of builtin function call expression
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
 *	Emit call expression
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Rvalue of the result of call expression
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
		lvalue prev_arg_displ[4 /* за $a0-$a3 */
									+ 4 / 2 /* за $fa0, $fa2 (т.к. single precision)*/];

		uni_printf(enc->sx->io, "\t# setting up $sp:\n");
		if (displ_for_parameters)
		{
			to_code_2R_I(enc->sx->io, IC_MIPS_ADDI, R_SP, R_SP, -(item_t)(displ_for_parameters));
		}

		uni_printf(enc->sx->io, "\n\t# parameters passing:\n");

		// TODO: структуры / массивы в параметры
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
					? &arg_saved_rvalue	// Сохранение значения в регистре-аргументе
					: &arg_rvalue		// Передача аргумента
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
				prev_arg_displ[arg_reg_count++] = tmp_arg_lvalue;
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

		size_t i = 0, j = 0;	// Счётчик обычных и floating point регистров-аргументов соответственно
		while (i + j < arg_reg_count)
		{
			uni_printf(enc->sx->io, "\n");

			const rvalue tmp_rval = emit_load_of_lvalue(enc, &prev_arg_displ[i + j]);
			emit_move_rvalue_to_register(
				enc,
				type_is_floating(prev_arg_displ[i + j].type) ? (R_FA0 + 2 * j++) : (R_A0 + i++),
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
 *	Emit inline expression
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Rvalue of inline expression
 */
// TODO: текущая ветка от feature, а туда inline_expression'ы пока не влили
/* static rvalue emit_inline_expression(encoder *const enc, const node *const nd)
{
	// FIXME: inline expression cannot return value at the moment
	const size_t amount = expression_inline_get_size(nd);

	for (size_t i = 0; i < amount; i++)
	{
		const node substmt = expression_inline_get_substmt(nd, i);
		emit_statement(enc, &substmt);
	}

	return RVALUE_VOID;
} */

/**
 *	Emit expression
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
 *
 *	@return	Rvalue of the expression
 */

/**
 *	Emit translation unit
 *
 *	@param	enc					Encoder
 *	@param	nd					Node in AST
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

static void mips_printf(mips_context *const ctx)
{

}

static void mips_emit_extern(mips_context *const ctx, const char *const name, void* type)
{
  mips_printf(ctx, ".extern %s", name);
}

// Генерация rvalue
// Генерация констант.
static void mips_emit_const_int(mips_context *const ctx, const int value)
{
  mips_printf(ctx, "%i", value);
}
static void mips_emit_const_float(mips_context *const ctx, const float value)
{
  mips_printf(ctx, "%i", value);
}
static void mips_emit_const_string(mips_context *const ctx, const char *const value)
{
  mips_printf(ctx, "%s", value);
}
static void mips_emit_const(mips_context *const ctx, const )
{

}
// Г
static void mips_emit_rvalue(mips_context *const ctx, const rvalue *const value)
{
  switch(value->kind)
  {
    case RVALUE_KIND_TEMP:
      mips_printf();
    case RVALUE_KIND_CONST:
    	mips_emit_const(ctx, value);
  }
}

static void mips_emit_lvalue(mips_context *const ctx, const lvalue *const value)
{
	switch(value->kind);
	{
	case LVALUE_KIND_GLOBAL:

	case LVALUE_KIND_REGISTER:

	case LVALUE_KIND_STACK:

	}
}

static void mips_emit_param(mips_context *const ctx, const rvalue *const value)
{
  if (ctx->param_count < 4)
  {
    mips_register param_register
    ctx->param_count
  }
  else
  {

  }
}
static void mips_emit_load(mips_context *const ctx, const rvalue *const dest, const lvalue *const src)
{
  if (src->kind == LVALUE_KIND_REGISTER)
  {
    mips_build_instr()
  }
}
static void mips_emit_call(mips_context *const ctx, item_t callee_id)
{
	mips_
}
static void mips_emit_ibin(mips_context *const ctx, MIPS_IC ic, MIPS_IC ic_imm, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
  if (rhs->type == RVALUE_KIND_CONST)
    mips_build_instr(ctx, ic_imm);
  else 
    mips_build_instr(ctx, ic);
  mips_emit_rvalue(ctx, res);
  mips_emit_rvalue(ctx, lhs);
  mips_emit_rvalue(ctx, rhs);
}
static void mips_emit_add(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
  mips_emit_bin(ctx, MIPS_IC_ADD, MIPS_IC_ADDI, lhs, rhs, res);
}
static void mips_emit_sub(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
  mips_emit_bin(ctx, MIPS_IC_SUB, MIPS_IC_SUBI, lhs, rhs, res);
}
static void mips_emit_mul(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
  mips_emit_bin(ctx, MIPS_IC_MUL, MIPS_IC_ADDI, lhs, rhs, res);
}
static void mips_emit_div(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
  mips_emit_bin(ctx, MIPS_IC_DIV, MIPS_IC_ADDI, lhs, rhs, res);
}

static void mips_emit_fadd(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
	
}
static void mips_emit_fsub(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
	
}
static void mips_emit_fmul(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
	
}
static void mips_emit_fdiv(mips_context *const ctx, const rvalue *const lhs, const rvalue *const rhs, const rvalue *const res)
{
	
}


static void mips_emit_ret(mips_context *const ctx, const rvalue *const value)
{
	if (value != NULL) 
	{
		if (value.kind == RVALUE_KIND_CONST)
			mips_build_move(ctx, MIPS_R_V0, );
		else
			mips_build_move(ctx, MIPS_R_V0, )
	}
	mips_build_jump_reg(ctx, MIPS_R_RA);
}

// В дальнейшем при необходимости сюда можно передавать флаги вывода директив
// TODO: подписать, что значит каждая директива и команда
static void mips_emit_pre(syntax *const sx)
{
	// Подпись "GNU As:" для директив GNU
	// Подпись "MIPS Assembler:" для директив ассемблера MIPS

	uni_printf(sx->io, "\t.section .mdebug.abi32\n");	// ?
	uni_printf(sx->io, "\t.previous\n");				// следующая инструкция будет перенесена в секцию, описанную выше
	uni_printf(sx->io, "\t.nan\tlegacy\n");				// ?
	uni_printf(sx->io, "\t.module fp=xx\n");			// ?
	uni_printf(sx->io, "\t.module nooddspreg\n");		// ?
	uni_printf(sx->io, "\t.abicalls\n");				// ?
	uni_printf(sx->io, "\t.option pic0\n");				// как если бы при компиляции была включена опция "-fpic" (что означает?)
	uni_printf(sx->io, "\t.text\n");					// последующий код будет перенесён в текстовый сегмент памяти
	// выравнивание последующих данных / команд по границе, кратной 2^n байт (в данном случае 2^2 = 4)
	uni_printf(sx->io, "\t.align 2\n");

	// делает метку main глобальной -- её можно вызывать извне кода (например, используется при линковке)
	uni_printf(sx->io, "\n\t.globl\tmain\n");
	uni_printf(sx->io, "\t.ent\tmain\n");				// начало процедуры main
	uni_printf(sx->io, "\t.type\tmain, @function\n");	// тип "main" -- функция
	uni_printf(sx->io, "main:\n");

	// инициализация gp
	// "__gnu_local_gp" -- локация в памяти, где лежит Global Pointer
	uni_printf(sx->io, "\tlui $gp, %%hi(__gnu_local_gp)\n");
	uni_printf(sx->io, "\taddiu $gp, $gp, %%lo(__gnu_local_gp)\n");

	// FIXME: сделать для $ra, $sp и $fp отдельные глобальные rvalue
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
}

static void mips_emit_post_functions(mips_context *const ctx)
{

}
static void mips_emit_post_strings(mips_context *const ctx)
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
				const label another_str_label = { .kind = L_STRING, .num = (size_t)(i + args_for_printf * amount) };
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

	// Выход из программы в конце работы
	to_code_R_I_R(enc->sx->io, IC_MIPS_LW, R_RA, 0, R_SP);
	emit_register_branch(enc, IC_MIPS_JR, R_RA);
}
static void mips_emit_post_main()
{
	uni_printf(enc->sx->io, "\tjal MAIN\n");
}

static void mips_emit_post(encoder *const enc)
{
	// FIXME: целиком runtime.s не вставить, т.к. не понятно, что делать с modetab
	// По этой причине вставляю только defarr
	uni_printf(enc->sx->io, "\n\n# defarr\n\
# объявление одномерного массива\n\
# $a0 -- адрес первого элемента\n\
# $a1 -- размер измерения\n\
DEFARR1:\n\
	sw $a1, 4($a0)			# Сохранение границы\n\
	li $v0, 4				# Загрузка размера слова\n\
	mul $v0, $v0, $a1		# Подсчёт размера первого измерения массива в байтах\n\
	sub $v0, $a0, $v0		# Считаем адрес после конца массива, т.е. $v0 -- на слово ниже последнего элемента\n\
	addi $v0, $v0, -4\n\
	jr $ra\n\
\n\
# объявление многомерного массива, но сначала обязана вызываться процедура DEFARR1\n\
# $a0 -- адрес первого элемента\n\
# $a1 -- размер измерения\n\
# $a2 -- адрес первого элемента предыдущего измерения\n\
# $a3 -- размер предыдущего измерения\n\
DEFARR2:\n\
	sw $a0, 0($a2)			# Сохраняем адрес в элементе предыдущего измерения\n\
	move $t0, $ra			# Запоминаем $ra, чтобы он не затёрся\n\
	jal DEFARR1				# Выделение памяти под массив\n\
	move $ra, $t0			# Восстанавливаем $ra\n\
	addi $a2, $a2, -4		# В $a2 следующий элемент в предыдущем измерении\n\
	addi $a0, $v0, -4		# В $a0 первый элемент массива в текущем измерении, плюс выделяется место под размеры\n\
	addi $a3, $a3, -1		# Уменьшаем счётчик\n\
	bne $a3, $0, DEFARR2	# Прыгаем, если ещё не всё выделили\n\
	jr $ra\n");

	uni_printf(enc->sx->io, "\n\n\t.end\tmain\n");
	uni_printf(enc->sx->io, "\t.size\tmain, .-main\n");
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_mips(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}

	ir_evals evals;

	evals.int_size = MIPS_INT_SIZE;
	evals.float_size = MIPS_FLOAT_SIZE;
	evals.
	evals.pointer_size = MIPS_POINTER_SIZE;

	evals.emit_pre = mips_emit_pre;
	evals.emit_post = mips_emit_post;

	evals.emit_iadd = mips_emit_iadd;
	evals.emit_isub = mips_emit_isub;
	evals.emit_imul = mips_emit_imul;
	evals.emit_idiv = mips_emit_idiv;

	evals.emit_fadd = mips_emit_fadd;
	evals.emit_fsub = mips_emit_fsub;
	evals.emit_fmul = mips_emit_fmul;
	evals.emit_fdiv = mips_emit_fdiv;

	encoder enc;
	enc.sx = sx;
	enc.next_register = R_T0;
	enc.next_float_register = R_FT0;
	enc.label_num = 1;
	enc.case_label_num = 1;

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
