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
#include "uniprinter.h"


static const size_t BUFFER_SIZE = 65536;				/**< Размер буфера для тела функции */
static const size_t HASH_TABLE_SIZE = 1024;				/**< Размер хеш-таблицы для смещений и регистров */
static const bool IS_ON_STACK = true;					/**< Хранится ли переменная на стеке */

static const size_t WORD_LENGTH = 4;					/**< Длина слова данных */
static const size_t HALF_WORD_LENGTH = 2;				/**< Длина половины слова данных */

static const size_t LOW_DYN_BORDER = 0x10010000;		/**< Нижняя граница динамической памяти */
static const size_t HEAP_DISPL = 8000;					/**< Смещение кучи относительно глобальной памяти */

static const size_t SP_SIZE = 4;						/**< Размер регистра $sp для его сохранения */
static const size_t RA_SIZE = 4;						/**< Размер регистра $ra для его сохранения */

static const size_t TEMP_FP_REG_AMOUNT = 12;			/**< Количество временных регистров для чисел с плавающей точкой */
static const size_t TEMP_REG_AMOUNT = 8;				/**< Количество обычных временных регистров */
static const size_t ARG_REG_AMOUNT = 4;					/**< Количество регистров-аргументов для функций */

static const bool FROM_LVALUE = 1;						/**< Освобожден ли регистр, в котором хранится значение rvalue */

/**< Смещение в стеке для сохранения оберегаемых регистров,
	 без учёта оптимизаций */
static const size_t FUNC_DISPL_PRESEREVED = /* за $sp */ 4 + /* за $ra */ 4
	+ /* fs0-fs10 (одинарная точность): */ 5*4
	+ /* s0-s7: */ 8*4;						

// Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition 
typedef enum REGISTER
{
	R_ZERO,							/**< Always has the value 0 */	
	R_AT,							/**< Temporary, generally used by assembler */

	R_V0,
	R_V1,							/**< Used for expression evaluations and to hold the integer
										and pointer type function return values */

	R_A0,
	R_A1,
	R_A2,
	R_A3,							/**< Used for passing arguments to functions; values are not
										preserved across function calls */

	R_T0,
	R_T1,
	R_T2,
	R_T3,
	R_T4,
	R_T5,
	R_T6,
	R_T7,							/**< Temporary registers used for expression evaluation; 
										values are not preserved across function calls */

	R_S0,
	R_S1,
	R_S2,
	R_S3,
	R_S4,
	R_S5,
	R_S6,
	R_S7,							/**< Saved registers; values are preserved across function calls */

	R_T8,
	R_T9,							/**< Temporary registers used for expression evaluations;
										values are not preserved across function calls.  When
										calling position independent functions $25 (R_T9) must contain
										the address of the called function */

	R_K0,
	R_K1,							/**< Used only by the operating system */

	R_GP,							/**< Global pointer and context pointer */
	R_SP,							/**< Stack pointer */
	R_FP,							/**< Saved register (like s0-s7) or frame pointer */
	R_RA,							/**< Return address. The return address is the location to
										which a function should return control */

	// Регистры для работы с числами с плавающей точкой
	// Для чисел с двойной точностью используется пара регистров:
	// - регистр с чётным номером содержит младшие 32 бита числа;
	// - регистр с нечётным номером содержит старшие 32 бита числа.
	R_FV0,
	R_FV1,							
	R_FV2,
	R_FV3,							/**< used to hold floating-point type function results; 
										single-precision uses $f0 and double-precision uses 
										the register pair $f0..$f1 */ 

	R_FA0,
	R_FA1,
	R_FA2,
	R_FA3,							/**< Used for passing arguments to functions */

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
	R_FT11,						    /**< Temporary registers */

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
	R_FS11							/**< Saved registers; their values are preserved across function calls */
} mips_register_t;


// Назначение команд взято из документации MIPS® Architecture for Programmers 
// Volume II-A: The MIPS32® Instruction 
// Set Manual 2016
typedef enum INSTRUCTION
{
	IC_MIPS_MOVE,					/**< MIPS Pseudo-Instruction. Move the contents of one register to another */
	IC_MIPS_LI,						/**< MIPS Pseudo-Instruction. Load a constant into a register */
	IC_MIPS_NOT,					/**< MIPS Pseudo-Instruction. Flips the bits of the source register and 
										stores them in the destination register (не из вышеуказанной книги) */

	IC_MIPS_ADDI,					/**< To add a constant to a 32-bit integer. If overflow occurs, then trap */
	IC_MIPS_SLL,					/**< To left-shift a word by a fixed number of bits */
	IC_MIPS_SRA,					/**< To execute an arithmetic right-shift of a word by a fixed number of bits */
	IC_MIPS_ANDI,					/**< To do a bitwise logical AND with a constant */
	IC_MIPS_XORI,					/**< To do a bitwise logical Exclusive OR with a constant */
	IC_MIPS_ORI,					/**< To do a bitwise logical OR with a constant */

	IC_MIPS_ADD,					/**< To add 32-bit integers. If an overflow occurs, then trap */
	IC_MIPS_SUB,					/**< To subtract 32-bit integers. If overflow occurs, then trap */
	IC_MIPS_MUL,					/**< To multiply two words and write the result to a GPR */
	IC_MIPS_DIV,					/**< DIV performs a signed 32-bit integer division, and places
										 the 32-bit quotient result in the destination register */
	IC_MIPS_MOD,					/**< MOD performs a signed 32-bit integer division, and places
										 the 32-bit remainder result in the destination register.
										 The remainder result has the same sign as the dividend */
	IC_MIPS_SLLV,					/**< To left-shift a word by a variable number of bits */
	IC_MIPS_SRAV,					/**< To execute an arithmetic right-shift of a word by a variable number of bits */
	IC_MIPS_AND,					/**< To do a bitwise logical AND */
	IC_MIPS_XOR,					/**< To do a bitwise logical Exclusive OR */
	IC_MIPS_OR,						/**< To do a bitwise logical OR */

	IC_MIPS_SW,						/**< To store a word to memory */
	IC_MIPS_LW,						/**< To load a word from memory as a signed value */

	IC_MIPS_JR,						/**< To execute a branch to an instruction address in a register */
	IC_MIPS_JAL,					/**< To execute a procedure call within the current 256MB-aligned region */
	IC_MIPS_J,						/**< To branch within the current 256 MB-aligned region */

	IC_MIPS_BLEZ,					/**< Branch on Less Than or Equal to Zero.
										To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BLTZ,					/**< Branch on Less Than Zero.
										To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BGEZ,					/**< Branch on Greater Than or Equal to Zero.
										To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BGTZ,					/**< Branch on Greater Than Zero.
										To test a GPR then do a PC-relative conditional branch */
	IC_MIPS_BEQ,					/**< Branch on Equal.
										To compare GPRs then do a PC-relative conditional branch */
	IC_MIPS_BNE,					/**< Branch on Not Equal.
										To compare GPRs then do a PC-relative conditional branch */

	IC_MIPS_LA,						/**< Load the address of a named memory 
										location into a register (не из вышеуказанной книги)*/

	IC_MIPS_NOP,					/**<To perform no operation */

	/** Floating point operations. Mostly single precision. */
	IC_MIPS_ADD_S, 					/**< To add FP values. */
	IC_MIPS_SUB_S, 					/**< To subtract FP values. */
	IC_MIPS_MUL_S, 					/**< To multiply FP values. */
	IC_MIPS_DIV_S, 					/**< To divide FP values. */ 

	IC_MIPS_S_S,					/**< MIPS Pseudo instruction. To store a doubleword from an FPR to memory. */
	IC_MIPS_L_S,					/**< MIPS Pseudo instruction. To load a doubleword from memory to an FPR. */

	IC_MIPS_LI_S,					/**< MIPS Pseudo-Instruction. Load a FP constant into a FPR. */

	IC_MIPS_MOV_S,					/**< The value in first FPR is placed into second FPR. */

	IC_MIPS_CVT_D_S,				/**< To convert an FP value to double FP. */
	IC_MIPS_CVT_S_D,				/**< To convert a double FP to single FP. */
	IC_MIPS_CVT_S_W,				/**< To convert a fixed point value to single FP. */
	IC_MIPS_CVT_W_S,				/**< To convert an FP value to fixed point. */

	IC_MIPS_MFC_1,					/**< To copy a word from an FPU (CP1) general register to a GPR. */
	IC_MIPS_MFHC_1,					/**< To copy a word from the high half of an FPU (CP1) 
										general register to a GPR. */
	IC_MIPS_SQRT_S,					/**< To compute the square root of an FP value. */
	IC_MIPS_ROUND_W_S,				/**< To convert an FP value to 32-bit fixed point, rounding to nearest. */
} mips_instruction_t;


typedef enum LABEL
{
	L_FUNC,							/**< Тип метки -- вход в функцию */
	L_NEXT,							/**< Тип метки -- следующая функция */
	L_FUNCEND,						/**< Тип метки -- выход из функции */
	L_STRING,						/**< Тип метки -- строка */
	L_ELSE,							/**< Тип метки -- переход по else */
	L_END,							/**< Тип метки -- переход в конец конструкции */
	L_BEGIN_CYCLE,					/**< Тип метки -- переход в начало цикла */
	L_USER_LABEL,					/**< Тип метки -- метка, заданная пользователем */
} mips_label_t;


typedef struct information
{
	syntax *sx;								/**< Структура syntax с таблицами */

	item_t main_label;						/**< Метка функции main */

	size_t max_displ;						/**< Максимальное смещение */

	hash displacements;						/**< Хеш таблица с информацией о расположении идентификаторов:
												@c key		 - ссылка на таблицу идентификаторов
												@c value[0]	 - флаг, лежит ли переменная на стеке или в регистре 
												@c value[1]  - смещение или номер регистра */

	mips_register_t next_register;			/**< Следующий обычный регистр для выделения */
	mips_register_t next_float_register;	/**< Следующий регистр с плавающей точкой для выделения */

	item_t label_num;						/**< Номер метки */
	item_t label_else;						/**< Метка перехода на else */

	size_t curr_function_ident;				/**< Идентификатор текущей функций */

	bool registers[24];						/**< Информация о занятых регистрах */
} information;

/** Kinds of lvalue */
typedef enum LVALUE_KIND
{
	STACK,
	REG,
} lvalue_kind_t;

typedef struct LVALUE
{ 
	lvalue_kind_t kind;				/**< Value kind */
	mips_register_t base_reg;		/**< Base register */
	union							/**< Value location */
	{
		item_t reg_num;				/**< Register where the value is stored */
		item_t displ;				/**< Stack displacement where the value is stored */
	} loc;
	item_t type;					/**< Value type */
} lvalue;  

/** Kinds of rvalue */
typedef enum RVALUE_KIND
{
	CONST,							// Значит, запомнили константу и потом обработали её
	REGISTER,
	VOID,
} rvalue_kind_t;
 
typedef struct RVALUE
{
	rvalue_kind_t kind;				/**< Value kind */
	item_t type;					/**< Value type */
	bool from_lvalue;				/**< Was the rvalue instance formed from lvalue */
	union
	{
		item_t reg_num;				/**< Where the value is stored */
		item_t int_val;				/**< Value of integer literal */
		double float_val;			/**< Value of floating literal */
		// TODO: остальные типы (включая сложные: массивы/структуры)
	} val; 
} rvalue;

static const rvalue rvalue_one = { .kind = CONST, .type = TYPE_INTEGER, .val.int_val = 1 };
static const rvalue rvalue_negative_one = { .kind = CONST, .type = TYPE_INTEGER, .val.int_val = -1 };


static lvalue emit_lvalue(information *info, const node *const nd);
static rvalue emit_expression(information *const info, const node *const nd);
static void emit_statement(information *const info, const node *const nd);
static lvalue emit_store_of_rvalue(information *const info, const rvalue rval, const lvalue lval);
static rvalue emit_load_of_lvalue(information *const info, const lvalue lval);
static void emit_store_rvalue_to_rvalue(information *const info, const rvalue destination, const rvalue source);


// TODO: это есть в кодогенераторе llvm, не хотелось бы копипастить
static item_t array_get_type(information *const info, const item_t array_type)
{
	item_t type = array_type;
	while (type_is_array(info->sx, type))
	{
		type = type_array_get_element_type(info->sx, type);
	}

	return type;
}

static size_t array_get_dim(information *const info, const item_t array_type)
{
	size_t i = 0;
	item_t type = array_type;
	while (type_is_array(info->sx, type))
	{
		type = type_array_get_element_type(info->sx, type);
		i++;
	}

	return i;
}


/**
 * Takes the first free register
 * 
 * @param	info				Codegen info (?)
 * 
 * @return	Register
*/
static mips_register_t get_register(information *const info)
{
	// Ищем первый свободный регистр
	mips_register_t i = 0;
	for (; i < TEMP_REG_AMOUNT; i++)
	{
		if (!info->registers[i])
		{
			break;
		}
	}

	assert(i != TEMP_REG_AMOUNT);

	// Занимаем его
	info->registers[i] = true;

	return i + R_T0;
}

/**
 * Takes the first free floating point register
 * 
 * @param	info				Codegen info (?)
 * 
 * @return	Register
*/
static mips_register_t get_float_register(information *const info)
{
	// Ищем первый свободный регистр
	mips_register_t i = 8;
	for (; i < TEMP_FP_REG_AMOUNT + TEMP_REG_AMOUNT; i += 2 /* т.к. операции с одинарной точностью */)
	{
		if (!info->registers[i])
		{
			break;
		}
	}

	assert(i != TEMP_FP_REG_AMOUNT + TEMP_REG_AMOUNT);

	// Занимаем его
	info->registers[i] = true;

	return i + R_FT0 - /* за индекс R_FT0 в info->registers */ 10;
}

/**
 * Free register occupied by rvalue
 * 
 * @param	info				Codegen info (?)
 * @param	rval				Rvalue to be freed 
*/
static void free_rvalue(information *const info, const rvalue rval)
{
	if (rval.kind == REGISTER)
	{
		switch (rval.val.reg_num)
		{
			case R_T0:
			case R_T1:
			case R_T2:
			case R_T3:
			case R_T4:
			case R_T5:
			case R_T6:
			case R_T7:
				if (info->registers[rval.val.reg_num - R_T0])
				{
					// Регистр занят => освобождаем
					info->registers[rval.val.reg_num - R_T0] = false;
				}
				return;

			case R_T8:
			case R_T9:
				if (info->registers[rval.val.reg_num - R_T8 + /* индекс R_T8 в info->registers */ 8])
				{
					// Регистр занят => освобождаем
					info->registers[rval.val.reg_num - R_T8 + 8] = false;
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
				if (info->registers[rval.val.reg_num - R_FT0 + /* индекс R_FT0 в info->registers */ 10])
				{
					// Регистр занят => освобождаем
					info->registers[rval.val.reg_num - R_FT0 + 10] = false;
				}
				break;

			default: // Не временный регистр => освобождать не надо
				return;
		}
	}
}


/**
 * Reverse binary logic operation
 * 
 * @param	operation			Operation to reverse
 * 
 * @return	Reversed binary operation
*/
static binary_t reverse_logic_command(const binary_t operation)
{
	assert(operation >= 7);
	assert(operation <= 12);

	switch (operation)
	{
		case BIN_LT:
			return BIN_GE;
		case BIN_GT:
			return BIN_LE;
		case BIN_LE:
			return BIN_GT;
		case BIN_GE:
			return BIN_LT;
		case BIN_EQ:
			return BIN_NE;
		case BIN_NE:
			return BIN_EQ;
		
		default:
			return 0;
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

		case IC_MIPS_CVT_D_S:
			uni_printf(io, "cvt.d.s");
			break;

		case IC_MIPS_CVT_S_D:
			uni_printf(io, "cvt.s.d");
			break;

		case IC_MIPS_CVT_W_S:
			uni_printf(io, "cvt.w.s");
			break;

		case IC_MIPS_CVT_S_W:
			uni_printf(io, "cvt.s.w");
			break;

		case IC_MIPS_MFC_1:
			uni_printf(io, "mfc1");
			break;

		case IC_MIPS_MFHC_1:
			uni_printf(io, "mfhc1");
			break;
		
		case IC_MIPS_SQRT_S:
			uni_printf(io, "sqrt.s");
			break;
		
		case IC_MIPS_ROUND_W_S:
			uni_printf(io, "round.w.s");
			break;
	}
}

static void mips_label_to_io(universal_io *const io, const mips_label_t label)
{
	switch (label)
	{
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
		case L_USER_LABEL:
			uni_printf(io, "USER_LABEL");
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

// Вид инструкции:	instr	reg
static void to_code_R(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, reg);
	uni_printf(io, "\n");
}

// Вид инструкции:	instr	label
static void to_code_L(universal_io *const io, const mips_instruction_t instruction
	, const mips_label_t label, const item_t label_num)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_label_to_io(io, label);
	uni_printf(io, "%" PRIitem "\n", label_num);
}

// Вид инструкции:	label:
static void to_code_label(universal_io *const io, const mips_label_t label, const item_t label_num)
{
	mips_label_to_io(io, label);
	uni_printf(io, "%" PRIitem ":\n", label_num);
}


/**
 * Writes "val" field of rvalue structure to io
 * 
 * @param	io			Universal i/o (?)
 * @param	rval		Rvalue whose value is to be printed
*/
static void rvalue_const_to_io(universal_io *const io, const rvalue rval)
{
	// TODO: Оставшиеся типы
	switch (rval.type)
	{
		case TYPE_INTEGER:
			uni_printf(io, "%" PRIitem, rval.val.int_val);
			break;

		case TYPE_FLOATING:
			uni_printf(io, "%f", rval.val.float_val);
		
		default:
			break;
	}
}

/**
 * Writes rvalue to io
 * 
 * @param	info			Codegen info (?)
 * @param	rval			Rvalue to write
*/
static void rvalue_to_io(information *const info, const rvalue rval)
{
	assert(rval.kind != VOID);

	if (rval.kind == CONST)
	{
		rvalue_const_to_io(info->sx->io, rval);
	}
	else
	{
		mips_register_to_io(info->sx->io, rval.val.reg_num);
	}
}


/*
 *	 ______     __  __     ______   ______     ______     ______     ______     __     ______     __   __     ______
 *	/\  ___\   /\_\_\_\   /\  == \ /\  == \   /\  ___\   /\  ___\   /\  ___\   /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \  __\   \/_/\_\/_  \ \  _-/ \ \  __<   \ \  __\   \ \___  \  \ \___  \  \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \_____\   /\_\/\_\  \ \_\    \ \_\ \_\  \ \_____\  \/\_____\  \/\_____\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/_____/   \/_/\/_/   \/_/     \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */
 

/**
 * Emit identifier expression
 *
 * @param	info			Codegen info (?)
 * @param	nd  			Node in AST
 *  
 * @return	Created lvalue
 */
static lvalue emit_identifier_lvalue(information *const info, const node *const nd)
{
	const size_t identifier = expression_identifier_get_id(nd);
	const item_t type = ident_get_type(info->sx, identifier);
	const item_t on_stack = hash_get(&info->displacements, identifier, 0);
	const item_t loc = hash_get(&info->displacements, identifier, 1);

	if (on_stack)
	{
		return (lvalue) { .type = type
			, .kind = STACK
			, .loc.displ = loc
			, .base_reg = R_SP /* FIXME: глобальные переменные */ };
	}
	return (lvalue) { .type = type, .kind = REG, .loc.reg_num = loc };
}

/**
 * Emit lvalue expression
 * 
 * @param	info			Information
 * @param	nd				Node in AST
 * 
 * @return	Lvalue
 */
static lvalue emit_lvalue(information *info, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			return emit_identifier_lvalue(info, nd); 

		case EXPR_SUBSCRIPT:
			//return emit_subscript_expression(info, nd);

		case EXPR_MEMBER:
			//return emit_member_expression(info, nd);

		case EXPR_UNARY: // Только UN_INDIRECTION
		{ 
			
		}

		default:
			// Не может быть lvalue
			system_error(node_unexpected, nd);
			return (lvalue){ .loc.displ = ITEM_MAX };
	}

}


/**
 * Stores rvalue to lvalue
 * 
 * @param	info			Codegen info (?)
 * @param	rval			Rvalue to store
 * @param	lval			Lvalue to store rvalue to
 * 
 * @return	Formed lvalue
*/
static lvalue emit_store_of_rvalue(information *const info, rvalue rval, const lvalue lval)
{
	assert(rval.kind != VOID);

	if (rval.kind == CONST)
	{
		// Предварительно загружаем константу в rvalue вида REGISTER
		const rvalue tmp_rval = rval;
		rval = (rvalue) { .kind = REGISTER
			, .val.reg_num = type_is_floating(tmp_rval.type) ? get_float_register(info) : get_register(info)
			, .type = tmp_rval.type
			, .from_lvalue = !FROM_LVALUE };
		emit_store_rvalue_to_rvalue(info, rval, tmp_rval);
	}

	uni_printf(info->sx->io, "\t");

	if (lval.kind == REG)
	{
		instruction_to_io(info->sx->io, type_is_floating(rval.type) ? IC_MIPS_MOV_S : IC_MIPS_MOVE);
		uni_printf(info->sx->io, " ");
		rvalue_to_io(info, rval);
		uni_printf(info->sx->io, ", ");
		rvalue_to_io(info, emit_load_of_lvalue(info, lval));
		uni_printf(info->sx->io, "\n");
	}
	else
	{
		instruction_to_io(info->sx->io, type_is_floating(rval.type) ? IC_MIPS_S_S : IC_MIPS_SW);
		uni_printf(info->sx->io, " ");
		rvalue_to_io(info, rval);
		uni_printf(info->sx->io, ", %" PRIitem "(", lval.loc.displ);
		mips_register_to_io(info->sx->io, lval.base_reg);
		uni_printf(info->sx->io, ")\n");
	}

	free_rvalue(info, rval);

	return lval;
}

/**
 * Stores rvalue into another register-kind rvalue
 * 
 * @param info				Codegen info (?)
 * @param destination		Rvalue where "source" parameter will be stored
 * @param source			Rvalue to store
*/
static void emit_store_rvalue_to_rvalue(information *const info
	, const rvalue destination, const rvalue source)
{
	assert(source.kind != VOID);
	assert(destination.kind == REGISTER);

	if (source.kind == CONST)
	{
		uni_printf(info->sx->io, "\t");
		(type_is_floating(source.type))
			? instruction_to_io(info->sx->io, IC_MIPS_LI_S)
			: instruction_to_io(info->sx->io, IC_MIPS_LI);
		uni_printf(info->sx->io, " ");
		rvalue_to_io(info, destination);
		uni_printf(info->sx->io, ", ");
		rvalue_to_io(info, source);
		uni_printf(info->sx->io, "\n");
	}
	else 
	{
		if (destination.val.reg_num == source.val.reg_num)
		{
			uni_printf(info->sx->io, "\t# stays in register");
			mips_register_to_io(info->sx->io, destination.val.reg_num);
			uni_printf(info->sx->io, "\n");
		}
		else
		{
			// TODO: Оставшиеся типы
			switch (type_get_class(info->sx, source.type))
			{
				case TYPE_INTEGER:
					uni_printf(info->sx->io, "\t");
					instruction_to_io(info->sx->io
						, !type_is_floating(destination.type) ? IC_MIPS_MOVE : IC_MIPS_CVT_S_W);
					uni_printf(info->sx->io, " ");
					rvalue_to_io(info, destination);
					uni_printf(info->sx->io, ", ");
					rvalue_to_io(info, source);
					uni_printf(info->sx->io, "\n");
					break;

				case TYPE_FLOATING:
					uni_printf(info->sx->io, "\t");
					instruction_to_io(info->sx->io, IC_MIPS_MOV_S);
					uni_printf(info->sx->io, " ");
					rvalue_to_io(info, destination);
					uni_printf(info->sx->io, ", ");
					rvalue_to_io(info, source);
					uni_printf(info->sx->io, "\n");
					break;

				default:
					break;
			}
		}
	}
}

/**
 * Loads lvalue to register and forms rvalue 
 * 
 * @param	info			Codegen info (?)
 * @param	lval			Lvalue to load
 * 
 * @return	Formed rvalue
*/
static rvalue emit_load_of_lvalue(information *const info, const lvalue lval)
{
	if (lval.kind == REG)
	{
		return (rvalue) { .kind = REGISTER
			, .type = lval.type
			, .val.reg_num = lval.loc.reg_num
			, .from_lvalue = FROM_LVALUE };
	}

	const mips_register_t reg_to_load = (type_is_floating(lval.type)) 
		? get_float_register(info) : get_register(info);
	const rvalue rval = { .kind = REGISTER
		, .type = lval.type
		, .val.reg_num = reg_to_load
		, .from_lvalue = !FROM_LVALUE };

	uni_printf(info->sx->io, "\t");

	switch (lval.type)
	{
		case TYPE_INTEGER:
			instruction_to_io(info->sx->io, IC_MIPS_LW);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, rval);
			uni_printf(info->sx->io, ", %" PRIitem "(", lval.loc.displ);
			mips_register_to_io(info->sx->io, lval.base_reg);
			uni_printf(info->sx->io, ")\n");
			break;

		case TYPE_FLOATING:
			instruction_to_io(info->sx->io, IC_MIPS_L_S);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, rval);
			uni_printf(info->sx->io, ", %" PRIitem "(", lval.loc.displ);
			mips_register_to_io(info->sx->io, lval.base_reg);
			uni_printf(info->sx->io, ")\n");
			break;

		case TYPE_STRUCTURE:
			break;
	
		default:
			break;
	}

	return rval;
}

/**
 * Apply operation to two rvalue values
 * 
 * @param	info			Codegen info (?)
 * @param	first_rval1		First rvalue
 * @param	second_rval2	Second rvalue
 * @param	operation		Operation
 * 
 * @return	Result rvalue
*/
static rvalue apply_bin_operation_rvalue(information *const info, rvalue rval1, rvalue rval2, const binary_t operation)
{
	assert(operation != BIN_LOG_AND);
	assert(operation != BIN_LOG_OR);

	assert(rval1.kind != VOID);
	assert(rval2.kind != VOID);

	mips_register_t result;
	rvalue result_rvalue;
	rvalue freeing_rvalue = (rvalue) { .kind = VOID };

	uni_printf(info->sx->io, "\t");

	if ((rval1.kind == REGISTER) && (rval2.kind == REGISTER))
	{
		if (!rval1.from_lvalue && !rval2.from_lvalue) // Оба rvalue -- не регистровые переменные
		{
			// Возьмём тогда для результата минимальный регистр, а другой впоследствии будет отброшен
			if (rval1.val.reg_num > rval2.val.reg_num)
			{
				result = rval2.val.reg_num;
				freeing_rvalue = rval1;
			}
			else
			{
				result = rval1.val.reg_num;
				freeing_rvalue = rval2;
			}
		} // В противном случае никакой регистр освобождать не требуется, т.к. в нём будет записан результат
		else if (rval1.from_lvalue)
		{
			result = rval2.val.reg_num;
		}
		else if (rval2.from_lvalue) 
		{
			result = rval1.val.reg_num;
		}
		else
		{
			result = type_is_floating(rval1.type) ? get_float_register(info) : get_register(info);
		}

		result_rvalue = (rvalue) { .kind = REGISTER
			, .val.reg_num = result
			, .type = rval1.type
			, .from_lvalue = !FROM_LVALUE };

		switch (operation)
		{
			case BIN_LT:
			case BIN_GT:
			case BIN_LE:
			case BIN_GE:
			case BIN_EQ:
			case BIN_NE:
				instruction_to_io(info->sx->io, IC_MIPS_SUB);
				uni_printf(info->sx->io, " ");
				rvalue_to_io(info, result_rvalue);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval1);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval2);
				uni_printf(info->sx->io, "\n");
				break;

			default:
				instruction_to_io(info->sx->io
					, get_bin_instruction(operation, /* Два регистра => 0 в get_bin_instruction() -> */ 0));
				uni_printf(info->sx->io, " ");
				rvalue_to_io(info, result_rvalue);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval1);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval2);
				uni_printf(info->sx->io, "\n");
		}
	}
	else
	{
		// Гарантируется, что будет ровно один оператор в регистре и один оператор в константе

		// Сделаем так, чтобы константа гарантированно лежала в rval2
		if (rval2.kind != CONST)
		{
			const rvalue tmp_rval = rval1;
			rval1 = rval2;
			rval2 = tmp_rval;
		}

		if (rval1.from_lvalue)
		{
			result = type_is_floating(rval1.type) ? get_float_register(info) : get_register(info);
		}
		else
		{
			result = rval1.val.reg_num;
		}

		result_rvalue = (rvalue) { .kind = REGISTER
		, .val.reg_num = result
		, .type = rval1.type
		, .from_lvalue = !FROM_LVALUE };

		switch (operation)
		{
			case BIN_LT:
			case BIN_GT:
			case BIN_LE:
			case BIN_GE:
			case BIN_EQ:
			case BIN_NE:
				// TODO: Оптимизации с умножением на (-1)
				// Загружаем <значение из rval2> на регистр
				instruction_to_io(info->sx->io
					, (type_is_floating(rval2.type)) ? IC_MIPS_LI_S : IC_MIPS_LI);
				const mips_register_t tmp_reg = (type_is_floating(rval2.type)) 
					? get_float_register(info) : get_register(info);
				mips_register_to_io(info->sx->io, tmp_reg);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval2);
				uni_printf(info->sx->io, "\n");

				// Записываем <значение из rval1> - <значение из rval2> в result
				uni_printf(info->sx->io, "\t");
				instruction_to_io(info->sx->io, IC_MIPS_SUB);
				uni_printf(info->sx->io, " ");
				rvalue_to_io(info, result_rvalue);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval1);
				uni_printf(info->sx->io, ", ");
				mips_register_to_io(info->sx->io, tmp_reg);
				uni_printf(info->sx->io, "\n");
				break;

			default:
				// Выписываем операцию, её результат будет записан в result
				instruction_to_io(info->sx->io 
					, get_bin_instruction(operation
						, /* Один регистр => 1 в get_bin_instruction() -> */ 1));
				uni_printf(info->sx->io, " ");
				rvalue_to_io(info, result_rvalue);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval1);
				uni_printf(info->sx->io, ", ");
				rvalue_to_io(info, rval2);
				uni_printf(info->sx->io, "\n");
		}
	}

	free_rvalue(info, freeing_rvalue);

	return result_rvalue;
}

/**
 *	Emit literal expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of literal expression
 */
static rvalue emit_literal_expression(const node *const nd)
{
	// Константа => хотим просто запомнить её значение

	const item_t type = expression_get_type(nd);

	// TODO: оставшиеся типы 
	switch (type)
	{
		case TYPE_INTEGER:
			return (rvalue) { .kind = CONST
				, .type = type
				, .val.int_val = expression_literal_get_integer(nd)
				, .from_lvalue = !FROM_LVALUE };

		case TYPE_FLOATING:
			return (rvalue) { .kind = CONST
				, .type = type
				, .val.float_val = expression_literal_get_floating(nd)
				, .from_lvalue = !FROM_LVALUE };

		default:
			return (rvalue) { .kind = VOID };
	}
}

/**
 * Emit printf expression
 * 
 * @param	info			Codegen info (?)
 * @param	
*/
static void emit_printf_expression(information *const info, const node *const nd, const size_t parameters_amount)
{
	const node string = expression_call_get_argument(nd, 0);
	const size_t index = expression_literal_get_string(&string);
	const size_t amount = strings_amount(info->sx);

	for (size_t i = 1; i < parameters_amount; i++)
	{
		// TODO: хорошо бы определённый регистр тоже через функцию выделять

		const node arg = expression_call_get_argument(nd, i);

		const rvalue arg_rvalue = emit_expression(info, &arg);

		uni_printf(info->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + (i-1) * amount);
		uni_printf(info->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + (i-1) * amount);

		if (!type_is_floating(arg_rvalue.type))
		{
			const rvalue tmp = { .type = arg_rvalue.type
				, .val.reg_num = (type_is_floating(arg_rvalue.type)) ? R_FA0 : R_A1
				, .kind = REGISTER
				, .from_lvalue = FROM_LVALUE };
			emit_store_rvalue_to_rvalue(info, tmp, arg_rvalue);
		}
		else
		{
			// Конвертируем single to double
			uni_printf(info->sx->io, "\t");
			instruction_to_io(info->sx->io, IC_MIPS_CVT_D_S);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, arg_rvalue);
			uni_printf(info->sx->io, ", ");
			rvalue_to_io(info, arg_rvalue);
			uni_printf(info->sx->io, "\n");

			// Следующие действия необходимы, т.к. аргументы в builtin-функции обязаны передаваться в $a0-$a3
			// Даже для floating point!
			// %lo из arg_rvalue в $a1
			uni_printf(info->sx->io, "\t");
			instruction_to_io(info->sx->io, IC_MIPS_MFC_1);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, (rvalue) { .from_lvalue = !FROM_LVALUE
				, .kind = REGISTER
				, .type = arg_rvalue.type
				, .val.reg_num = R_A1});
			uni_printf(info->sx->io, ", ");
			rvalue_to_io(info, arg_rvalue);
			uni_printf(info->sx->io, "\n");
			
			// %hi из arg_rvalue в $a2
			uni_printf(info->sx->io, "\t");
			instruction_to_io(info->sx->io, IC_MIPS_MFHC_1);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, (rvalue) { .from_lvalue = !FROM_LVALUE
				, .kind = REGISTER
				, .type = arg_rvalue.type
				, .val.reg_num = R_A2});
			uni_printf(info->sx->io, ", ");
			rvalue_to_io(info, arg_rvalue);
			uni_printf(info->sx->io, "\n");
		}

		uni_printf(info->sx->io, "\tjal printf\n\tnop\n");
	}

	uni_printf(info->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + (parameters_amount-1) * amount);
	uni_printf(info->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + (parameters_amount-1) * amount);
	uni_printf(info->sx->io, "\tjal printf\n\tnop\n");
}

static void bi_func(universal_io *const io, const builtin_t func)
{
	switch (func)
	{
		// Math:
		case BI_SQRT:
			to_code_2R(io, IC_MIPS_SQRT_S, R_FV0, R_FA0);
			break;
		case BI_ROUND:
			to_code_2R(io, IC_MIPS_ROUND_W_S, R_V0, R_FA0);
			break;

		// TODO:
		/*
		case BI_EXP:					return "exp";
		case BI_SIN:					return "sin";
		case BI_COS:					return "cos";
		case BI_LOG:					return "log";
		case BI_LOG10:					return "log10";
		case BI_ASIN:					return "asin";
		case BI_RAND:					return "rand";
		*/

		// TODO: оставшиеся builtin-функции
		default:						break;
	}
}

/**
 * Emit call expression
 *
 * @param	info			Codegen info (?)
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of the result of call expression
 */
static rvalue emit_call_expression(information *const info, const node *const nd)
{
	const node callee = expression_call_get_callee(nd);
	size_t func_ref = expression_identifier_get_id(&callee);
	const size_t params_amount = expression_call_get_arguments_amount(nd);

	const item_t return_type = type_function_get_return_type(info->sx, expression_get_type(&callee));

	uni_printf(info->sx->io, "\t# \"%s\" function call:\n", ident_get_spelling(info->sx, func_ref));

	// TODO: Перед вызовом функций также сохранять регистры-аргументы 

	if (func_ref == BI_PRINTF)
	{
		emit_printf_expression(info, nd, params_amount);
	}
	else
	{
		size_t f_arg_count = 0;
		size_t arg_count = 0;

		uni_printf(info->sx->io, "\t# parameters passing:\n");

		// TODO: структуры

		size_t displ_for_parameters = 0;
		for (size_t i = 0; i < params_amount; i++)
		{
			const node arg = expression_call_get_argument(nd, i);
			rvalue arg_rvalue = emit_expression(info, &arg);

			// FIXME: Сохранять только в случае, если arg_rvalue.from_lvalue == True

			// Сохранение текущего регистра-аргумента на стек либо передача аргументов на стек
			emit_store_of_rvalue(info
				, (type_is_floating(arg_rvalue.type) ? f_arg_count : arg_count) < ARG_REG_AMOUNT 
					? (rvalue) { .kind = REGISTER
						, .val.reg_num = (type_is_floating(arg_rvalue.type) ? R_FA0 + f_arg_count : R_A0 + arg_count)
						, .type = arg_rvalue.type
						, .from_lvalue = !FROM_LVALUE } // Сохранение значения в регистре-аргументе
					: arg_rvalue // Передача аргумента
				, (lvalue) { .loc.displ = 0, .base_reg = R_FP, .kind = STACK, .type = arg_rvalue.type });

			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_FP, -(item_t)WORD_LENGTH);
			displ_for_parameters += WORD_LENGTH;

			// в регистры-аргументы
			if ((type_is_floating(arg_rvalue.type) ? f_arg_count : arg_count) < ARG_REG_AMOUNT)
			{
				// Аргументы рассматриваются в данном случае как регистровые переменные
				const rvalue tmp = { .kind = REGISTER
					, .val.reg_num = type_is_floating(arg_rvalue.type) ? (R_FA0 + f_arg_count) : (R_A0 + arg_count)
					, .type = arg_rvalue.type
					, .from_lvalue = !FROM_LVALUE };
				emit_store_rvalue_to_rvalue(info, tmp, arg_rvalue);
			}

			if (type_is_floating(arg_rvalue.type))
			{
				f_arg_count += 2;
			}
			else
			{
				arg_count += 1;
			}

			free_rvalue(info, arg_rvalue);
		}

		if (func_ref >= BEGIN_USER_FUNC)
		{
			to_code_L(info->sx->io, IC_MIPS_JAL, L_FUNC, func_ref);
		}
		else
		{
			bi_func(info->sx->io, func_ref);
			instruction_to_io(info->sx->io, IC_MIPS_NOP);
		}

		uni_printf(info->sx->io, "\n\t# data restoring:\n");
		if (displ_for_parameters)
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_FP, (item_t)(displ_for_parameters));
		}
/*
		for (size_t i = 0; i < arg_count; i++)
		{
			const rvalue tmp = emit_load_of_lvalue(info
				, (lvalue) { .base_reg = R_FP
					, .kind = !FROM_LVALUE
					, .loc.displ = -(item_t)WORD_LENGTH*i
					// FIXME: Дальнейшее некорректно для любых типов, кроме INTEGER
					, .type = TYPE_INTEGER });

			emit_store_rvalue_to_rvalue(info
				, (rvalue) { .from_lvalue = !FROM_LVALUE
					, .kind = REGISTER
					, .val.reg_num = R_A0 + i }
				, tmp);

			free_rvalue(info, tmp);
		}

		for (size_t i = 0; i < f_arg_count; i += 2)
		{
			const rvalue tmp = emit_load_of_lvalue(info
				, (lvalue) { .base_reg = R_FP
					, .kind = STACK
					, .loc.displ = -(item_t)(WORD_LENGTH*(i/2) + WORD_LENGTH*arg_count)
					, .type = TYPE_FLOATING });

			emit_store_rvalue_to_rvalue(info
				, (rvalue) { .from_lvalue = !FROM_LVALUE
					, .kind = REGISTER
					, .val.reg_num = R_FA0 + i }
				, tmp);

			free_rvalue(info, tmp);
		}

		printf("%zu <- arg_count\n %zu <- f_arg_count\n", arg_count, f_arg_count);
*/
		uni_printf(info->sx->io, "\n");
	}

	return (rvalue){ .kind = REGISTER
		, .type = return_type
		, .val.reg_num = type_is_floating(return_type) ? R_FV0 : R_V0
		, .from_lvalue = !FROM_LVALUE };
}

/**
 *	Emit increment/decrement expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return Rvalue of the result of increment/decrement expression
 */
static rvalue emit_inc_dec_expression(information *const info, const node *const nd)
{
	const unary_t operation = expression_unary_get_operator(nd);

	const node identifier = expression_unary_get_operand(nd);

	// TODO: вещественные числа
	lvalue identifier_lvalue = emit_lvalue(info, &identifier);
	const rvalue identifier_rvalue = emit_load_of_lvalue(info, identifier_lvalue);

	const mips_register_t post_result_reg = get_register(info);
	const rvalue post_result_rvalue = { .kind = REGISTER
		, .val.reg_num = post_result_reg
		, .type = identifier_lvalue.type
		, .from_lvalue = FROM_LVALUE };

	if (operation == UN_POSTDEC || operation == UN_POSTINC)
	{
		emit_store_rvalue_to_rvalue(info, post_result_rvalue, identifier_rvalue);
	}

	switch (operation)
	{
		case UN_PREDEC:
		case UN_POSTDEC:
			apply_bin_operation_rvalue(info, identifier_rvalue, rvalue_negative_one, BIN_ADD);
			break;
		case UN_PREINC:
		case UN_POSTINC:
			apply_bin_operation_rvalue(info, identifier_rvalue, rvalue_one, BIN_ADD);
			break;

		default:
			break;
	}

	if (identifier_lvalue.kind == STACK)
	{
		emit_store_of_rvalue(info, identifier_rvalue, identifier_lvalue);
	}
	// Иначе результат и так будет в identifier_rvalue

	if (operation == UN_POSTDEC || operation == UN_POSTINC)
	{
		return post_result_rvalue;
	}
	return identifier_rvalue;
} 

/**
 *	Emit unary expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of the result of unary expression
 */
static rvalue emit_unary_expression_rvalue(information *const info, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd); 

	switch (operator)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			return emit_inc_dec_expression(info, nd);

		case UN_MINUS:
		case UN_NOT:
		{
			const node operand = expression_unary_get_operand(nd);
			rvalue operand_rvalue = emit_expression(info, &operand);

			if (operator == UN_MINUS)
			{
				return apply_bin_operation_rvalue(info
					, operand_rvalue
					, (rvalue) { .kind = REGISTER
						, .val.reg_num = R_ZERO
						, .type = TYPE_INTEGER
						, .from_lvalue = !FROM_LVALUE }
					, BIN_SUB);
			}
			else
			{
				return apply_bin_operation_rvalue(info
					, operand_rvalue
					, rvalue_negative_one
					, BIN_XOR);
			}
		}

		case UN_LOGNOT:
		{
			const node operand = expression_unary_get_operand(nd);
			rvalue operand_rvalue = emit_expression(info, &operand);

			uni_printf(info->sx->io, "\t");
			instruction_to_io(info->sx->io, IC_MIPS_NOT);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, operand_rvalue);
			uni_printf(info->sx->io, ", ");
			rvalue_to_io(info, operand_rvalue);
			uni_printf(info->sx->io, "\n");

			// TODO:
			uni_printf(info->sx->io, "\t");
			instruction_to_io(info->sx->io, IC_MIPS_BLTZ);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, operand_rvalue);
			uni_printf(info->sx->io, ", ");
			mips_label_to_io(info->sx->io, L_ELSE);
			uni_printf(info->sx->io, "%" PRIitem "\n", info->label_else);

			return operand_rvalue;
		}

		case UN_ADDRESS:
		case UN_INDIRECTION:
		case UN_ABS:
			return (rvalue){ .kind = VOID };

		default:
			// TODO: оставшиеся унарные операторы
			return (rvalue){ .kind = VOID };
	}
}

/**
 *	Emit logic binary expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of the result of logic expression
 */ 
static rvalue emit_logic_expression(information *const info, const node *const nd)
{
	binary_t operation = expression_binary_get_operator(nd);
	
	const node LHS = expression_binary_get_LHS(nd); 
	const rvalue lhs_rvalue = emit_expression(info, &LHS);

	operation = reverse_logic_command(operation);

	const node RHS = expression_binary_get_RHS(nd);
	const rvalue rhs_rvalue = emit_expression(info, &RHS);

	return apply_bin_operation_rvalue(info, lhs_rvalue, rhs_rvalue, operation);
} 

/**
 *	Emit non-assignment binary expression
 *
 *	@param	info			Encoder
 *	@param	nd				Node in AST
 */ 
static rvalue emit_integral_expression_rvalue(information *const info, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);

	const node LHS = expression_binary_get_LHS(nd);
	rvalue lhs_rvalue = emit_expression(info, &LHS);	

	const node RHS = expression_binary_get_RHS(nd); 
	rvalue rhs_rvalue = emit_expression(info, &RHS);

	const rvalue res = apply_bin_operation_rvalue(info, lhs_rvalue, rhs_rvalue, operator);

	return res;
} 

/**
 * Emit assignment expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of the result of assignment expression
 */
static rvalue emit_assignment_expression(information *const info, const node *const nd)
{
	const binary_t operation = expression_assignment_get_operator(nd);

	// LHS -- точно lvalue
	const node LHS = expression_assignment_get_LHS(nd);
	const lvalue lhs_lvalue = emit_lvalue(info, &LHS);

	const node RHS = expression_assignment_get_RHS(nd); 
	rvalue rhs_rvalue = emit_expression(info, &RHS);

	if (operation != BIN_ASSIGN) // это "+=", "-=" и т.п. 
	{   
		const rvalue lhs_rvalue = emit_load_of_lvalue(info, lhs_lvalue);
		binary_t correct_operation;
		switch (operation)
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

			default: // BIN_OR_ASSIGN
				correct_operation = BIN_OR;
		}
		rhs_rvalue = apply_bin_operation_rvalue(info, lhs_rvalue, rhs_rvalue, correct_operation);

		free_rvalue(info, lhs_rvalue);
	} 

	if (lhs_lvalue.kind == STACK)
	{
		emit_store_of_rvalue(info, rhs_rvalue, lhs_lvalue);
	}
	// Иначе всё и так будет в rhs_rvalue
	
	return rhs_rvalue; 
}

/**
 * Emit binary expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of the result of binary expression
 */ 
static rvalue emit_binary_expression(information *const info, const node *const nd)
{
	const binary_t operation = expression_binary_get_operator(nd);

	switch (operation)
	{
		case BIN_MUL:
		case BIN_DIV:
		case BIN_REM:
		case BIN_ADD:
		case BIN_SUB:
		case BIN_SHL:
		case BIN_SHR:
		case BIN_AND:
		case BIN_XOR:
		case BIN_OR:
			return emit_integral_expression_rvalue(info, nd);

		case BIN_LT:
		case BIN_GT:
		case BIN_LE:
		case BIN_GE:
		case BIN_EQ:
		case BIN_NE:
			return emit_logic_expression(info, nd);

		case BIN_LOG_OR:
		case BIN_LOG_AND:
		{
			// TODO: А если это объявление переменной типа bool?

			const item_t label_then = info->label_num;
			const item_t old_label_else = info->label_else;

			const node LHS = expression_binary_get_LHS(nd);
			rvalue lhs_rvalue = emit_expression(info, &LHS);

			if (lhs_rvalue.kind == CONST)
			{
				// Предварительно загружаем константу в rvalue вида REGISTER
				const rvalue tmp_rval = lhs_rvalue;
				lhs_rvalue = (rvalue) { .kind = REGISTER
					, .val.reg_num = type_is_floating(tmp_rval.type) ? get_float_register(info) : get_register(info)
					, .type = tmp_rval.type
					, .from_lvalue = !FROM_LVALUE };
				emit_store_rvalue_to_rvalue(info, lhs_rvalue, tmp_rval);
			}

			uni_printf(info->sx->io, "\t");
			if (operation == BIN_LOG_OR)
			{
				instruction_to_io(info->sx->io, IC_MIPS_BGTZ);
				uni_printf(info->sx->io, " ");
				rvalue_to_io(info, lhs_rvalue);
				uni_printf(info->sx->io, ", ");
				mips_label_to_io(info->sx->io, L_ELSE);
				uni_printf(info->sx->io, "%" PRIitem "\n", label_then);
			}
			else
			{
				instruction_to_io(info->sx->io, IC_MIPS_BLTZ);
				uni_printf(info->sx->io, " ");
				rvalue_to_io(info, lhs_rvalue);
				uni_printf(info->sx->io, ", ");
				mips_label_to_io(info->sx->io, L_ELSE);
				uni_printf(info->sx->io, "%" PRIitem "\n", old_label_else);
			}

			info->label_else = old_label_else;

			const node RHS = expression_binary_get_RHS(nd); 
			rvalue rhs_rvalue = emit_expression(info, &RHS);

			if (rhs_rvalue.kind == CONST)
			{
				// Предварительно загружаем константу в rvalue вида REGISTER
				const rvalue tmp_rval = rhs_rvalue;
				rhs_rvalue = (rvalue) { .kind = REGISTER
					, .val.reg_num = type_is_floating(tmp_rval.type) ? get_float_register(info) : get_register(info)
					, .type = tmp_rval.type
					, .from_lvalue = !FROM_LVALUE };
				emit_store_rvalue_to_rvalue(info, rhs_rvalue, tmp_rval);
			}

			instruction_to_io(info->sx->io, IC_MIPS_BLTZ);
			uni_printf(info->sx->io, " ");
			rvalue_to_io(info, rhs_rvalue);
			uni_printf(info->sx->io, ", ");
			mips_label_to_io(info->sx->io, L_ELSE);
			uni_printf(info->sx->io, "%" PRIitem "\n", old_label_else);

			to_code_label(info->sx->io, L_ELSE, label_then);

			free_rvalue(info, rhs_rvalue);
			free_rvalue(info, lhs_rvalue);

			return (rvalue) { .kind = VOID };
		}

		default:
			// TODO: оставшиеся бинарные операторы
			return (rvalue) { .kind = VOID };
	}
}

/**
 * Emit expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of the expression
 */
static rvalue emit_expression(information *const info, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		lvalue lval = emit_lvalue(info, nd);
		return emit_load_of_lvalue(info, lval); 
	}

	// Иначе rvalue: 
	switch (expression_get_class(nd))
	{
		/*
		case EXPR_CAST:
			return emit_cast_expression(info, nd);
		*/
		case EXPR_LITERAL:
			return emit_literal_expression(nd); 
		/*
		case EXPR_SUBSCRIPT:
			return emit_subscript_expression(info, nd);
		*/ 
		case EXPR_CALL:
			return emit_call_expression(info, nd);  
		/*
		case EXPR_MEMBER:
			return emit_member_expression(info, nd);
		*/

		case EXPR_UNARY:
			return emit_unary_expression_rvalue(info, nd); 

		case EXPR_BINARY:
			return emit_binary_expression(info, nd);  

		case EXPR_ASSIGNMENT: 
			return emit_assignment_expression(info, nd);  
		/*
		case EXPR_TERNARY:
			return emit_ternary_expression(info, nd);
		*/
		/*
		case EXPR_INLINE:
			emit_inline_expression(info, nd);
			return;
		*/
		/*
		case EXPR_INITIALIZER:
			//return emit_initializer_expression(info, nd); 
		*/

		default: // EXPR_INVALID
			// TODO: генерация оставшихся выражений
			assert(expression_get_class(nd) == EXPR_INVALID);
			return (rvalue) { .kind = VOID };
	}  
} 

/**
 * Emit expression which will be evaluated as a void expression
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 * 
 * @return	Rvalue of void type
 */
static rvalue emit_void_expression(information *const info, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		emit_lvalue(info, nd); // Либо регистровая переменная, либо на стеке => ничего освобождать не надо
	}
	else
	{
		const rvalue result = emit_expression(info, nd);

		free_rvalue(info, result);
	}
	return (rvalue) { .kind = VOID };
}


/*
 *	 _____     ______     ______     __         ______     ______     ______     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\   /\ \       /\  __ \   /\  == \   /\  __ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \ \____  \ \ \____  \ \  __ \  \ \  __<   \ \  __ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/_/ /_/   \/_/\/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 * Emit variable declaration
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_variable_declaration(information *const info, const node *const nd)
{
	const size_t id = declaration_variable_get_id(nd);
	const bool has_init = declaration_variable_has_initializer(nd);
	const item_t type = ident_get_type(info->sx, id);

	uni_printf(info->sx->io, "\t# \"%s\" variable declaration:\n", ident_get_spelling(info->sx, id));

	info->max_displ += WORD_LENGTH;
	const size_t value_displ = info->max_displ;

	// TODO: в глобальных переменных регистр gp

	const mips_register_t value_reg = R_SP;

	const size_t index = hash_add(&info->displacements, id, 2);
	hash_set_by_index(&info->displacements, index, 0, IS_ON_STACK);
	hash_set_by_index(&info->displacements, index, 1, (item_t)value_displ);

	if (!type_is_array(info->sx, type)) // обычная переменная int a; или struct point p;
	{
		// TODO: структуры 
		// TODO: А если мы имеем дело с глобальными переменными? value_reg не работает

		if (has_init)
		{
			// TODO: тип char

			const node initializer = declaration_variable_get_initializer(nd);
			const rvalue initializer_rvalue = emit_expression(info, &initializer);

			emit_store_of_rvalue(info
				, initializer_rvalue
				, (lvalue) { .base_reg = value_reg
					, .kind = STACK
					, .loc.displ = value_displ
					, .type = initializer_rvalue.type});

			free_rvalue(info, initializer_rvalue);
		}
	}
	else
	{
		// TODO:
		const size_t dimensions = array_get_dim(info, type);
		const item_t element_type = array_get_type(info, type);
		const item_t usual = 1; // предстоит выяснить, что это такое

		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A0, R_ZERO, has_init ? dimensions - 1 : dimensions); // размерность
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A1, R_ZERO, type_size(info->sx, element_type) * 4); // размер элемента
		// передаём смещение относительно fp (положительное значение) или gp (отрицательное значение)
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A2, R_ZERO, -(item_t)value_displ);
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A3, R_ZERO, 4 * has_init + usual);
		uni_printf(info->sx->io, "\tjal DEFARR\n");
		uni_printf(info->sx->io, "\t# addr 0($fp) now contains array size, addr 4($fp) contains first array element\n");
	}
	uni_printf(info->sx->io, "\n");
}


/**
 * Emit function definition
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_function_definition(information *const info, const node *const nd)
{
	const size_t ref_ident = declaration_function_get_id(nd);
	const item_t func_type = ident_get_type(info->sx, ref_ident);
	const size_t parameters = type_function_get_parameter_amount(info->sx, func_type);

	// TODO: TYPE_FLOATING
	const size_t displ_for_parameters = (parameters > ARG_REG_AMOUNT) 
		? (parameters - ARG_REG_AMOUNT)*WORD_LENGTH : 0;

	info->curr_function_ident = ref_ident;

	if (ref_ident == info->sx->ref_main)
	{
		info->main_label = ref_ident;
	}

	to_code_L(info->sx->io, IC_MIPS_J, L_NEXT, ref_ident);
	to_code_label(info->sx->io, L_FUNC, ref_ident);

	uni_printf(info->sx->io, "\t# \"%s\" function:\n", ident_get_spelling(info->sx, ref_ident));

	uni_printf(info->sx->io, "\n\t# saving $sp and $ra:\n");
	to_code_R_I_R(info->sx->io
		, IC_MIPS_SW
		, R_RA
		, -(item_t)RA_SIZE
		, R_FP);
	to_code_R_I_R(info->sx->io
		, IC_MIPS_SW
		, R_SP
		, -(item_t)(RA_SIZE + SP_SIZE)
		, R_FP);
	
	info->max_displ = 0;

	// Сохранение перед началом работы функции
	uni_printf(info->sx->io, "\n\t# preserved registers:\n");

	// Сохранение s0-s7
	for (size_t i = 0; i < 8; i++)
	{
		to_code_R_I_R(info->sx->io
			, IC_MIPS_SW
			, R_S0 + i
			, -(item_t)(RA_SIZE + SP_SIZE + (i+1)*WORD_LENGTH)
			, R_FP);
	}

	uni_printf(info->sx->io, "\n");

	// Сохранение fs0-fs10 (в цикле 5, т.к. операции одинарной точности => нужны только четные регистры)
	for (size_t i = 0; i < 5; i++)
	{
		to_code_R_I_R(info->sx->io
			, IC_MIPS_S_S
			, R_FS0 + 2*i
			, -(item_t)(RA_SIZE + SP_SIZE + (i+1)*WORD_LENGTH + 8*WORD_LENGTH /* за $s0-$s7 */)
			, R_FP);
	}

	info->max_displ = FUNC_DISPL_PRESEREVED;

	// Выравнивание смещения на 8
	if (info->max_displ % 8)
	{
		const size_t padding = 8 - (info->max_displ % 8);
		info->max_displ += padding;
		if (padding)
		{
			uni_printf(info->sx->io, "\n\t# padding -- max displacement == %zu\n", info->max_displ);
		}
	}

	// Создание буфера для тела функции
	universal_io *old_io = info->sx->io;
	universal_io new_io = io_create();
  	out_set_buffer(&new_io, BUFFER_SIZE);
	info->sx->io = &new_io;
	const size_t max_displ_prev = info->max_displ;
	info->max_displ = 0;

	uni_printf(info->sx->io, "\n\t# function body:\n");
	node body = declaration_function_get_body(nd);
	emit_statement(info, &body);

	// Извлечение буфера с телом функции в старый io
	char *buffer = out_extract_buffer(info->sx->io);
	info->sx->io = old_io;

	uni_printf(info->sx->io, "\n\t# setting up $fp:\n");
	// $fp указывает на конец динамики (которое в данный момент равно концу статики)
	to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_FP, -(item_t)(info->max_displ + max_displ_prev + WORD_LENGTH));

	uni_printf(info->sx->io, "\n\t# setting up $sp:\n");
	// $sp указывает на конец статики (которое в данный момент равно концу динамики)
	to_code_2R(info->sx->io, IC_MIPS_MOVE, R_SP, R_FP);

	uni_printf(info->sx->io, "\n\t# function parameters:\n"); 
	
	for (size_t i = 0; i < parameters; i++)
	{
		const size_t id = declaration_function_get_param(nd, i);

		// Вносим переменную в таблицу символов 
		const size_t index = hash_add(&info->displacements, id, 2);
		
		uni_printf(info->sx->io, "\t# parameter \"%s\" ", ident_get_spelling(info->sx, id));

		// TODO: TYPE_FLOATING
		if (i < ARG_REG_AMOUNT)
		{
			// Рассматриваем их как регистровые переменные
			const mips_register_t curr_reg = R_A0 + i;
			uni_printf(info->sx->io, "is in register ");
			mips_register_to_io(info->sx->io, curr_reg);
			uni_printf(info->sx->io, "\n");

			hash_set_by_index(&info->displacements, index, 0, !IS_ON_STACK);
			hash_set_by_index(&info->displacements, index, 1, (item_t)curr_reg);
		}
		else
		{
			uni_printf(info->sx->io
				, "stays on stack and has displacement %zu from $sp\n"
				, (displ_for_parameters - (i - ARG_REG_AMOUNT)*WORD_LENGTH) + FUNC_DISPL_PRESEREVED);

			hash_set_by_index(&info->displacements, index, 0, IS_ON_STACK);
			hash_set_by_index(&info->displacements
				, index
				, 1
				, (item_t)(displ_for_parameters - (i - ARG_REG_AMOUNT)*WORD_LENGTH) + FUNC_DISPL_PRESEREVED);
		}
	}

	// Выделение на стеке памяти для тела функции  
	uni_printf(info->sx->io, "%s", buffer);
	free(buffer);

	to_code_label(info->sx->io, L_FUNCEND, ref_ident);

	// Восстановление стека после работы функции
	uni_printf(info->sx->io, "\n\t# data restoring:\n");

	// Ставим $fp на его положение в предыдущей функции
	to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_SP, (item_t)(info->max_displ + max_displ_prev + WORD_LENGTH));

	uni_printf(info->sx->io, "\n");

	// Восстановление s0-s7
	for (size_t i = 0; i < 8; i++)
	{
		to_code_R_I_R(info->sx->io
			, IC_MIPS_LW
			, R_S0 + i
			, -(item_t)(RA_SIZE + SP_SIZE + (i+1)*WORD_LENGTH)
			, R_FP);
	}

	uni_printf(info->sx->io, "\n");

	// Восстановление fs0-fs7
	for (size_t i = 0; i < 5; i++)
	{
		to_code_R_I_R(info->sx->io
			, IC_MIPS_L_S
			, R_FS0 + 2*i
			, -(item_t)(RA_SIZE + SP_SIZE + (i+1)*WORD_LENGTH + /* за s0-s7 */ 8*WORD_LENGTH)
			, R_FP);
	}

	uni_printf(info->sx->io, "\n");

	// Возвращаем $sp его положение в предыдущей функции
	to_code_R_I_R(info->sx->io
		, IC_MIPS_LW
		, R_SP
		, -(item_t)(RA_SIZE + SP_SIZE)
		, R_FP);

	to_code_R_I_R(info->sx->io
		, IC_MIPS_LW
		, R_RA
		, -(item_t)(RA_SIZE)
		, R_FP);

	// Прыгаем далее
	to_code_R(info->sx->io, IC_MIPS_JR, R_RA);
	to_code_label(info->sx->io, L_NEXT, ref_ident);
}

static void emit_declaration(information *const info, const node *const nd)
{
	switch (declaration_get_class(nd))
	{
		case DECL_VAR:
			emit_variable_declaration(info, nd);
			return;

		case DECL_FUNC:
			emit_function_definition(info, nd);
			return;

		default:
			// С объявлением типа ничего делать не нужно
			return;
	}
}


/*
 *	 ______     ______   ______     ______   ______     __    __     ______     __   __     ______   ______
 *	/\  ___\   /\__  _\ /\  __ \   /\__  _\ /\  ___\   /\ "-./  \   /\  ___\   /\ "-.\ \   /\__  _\ /\  ___\
 *	\ \___  \  \/_/\ \/ \ \  __ \  \/_/\ \/ \ \  __\   \ \ \-./\ \  \ \  __\   \ \ \-.  \  \/_/\ \/ \ \___  \
 *	 \/\_____\    \ \_\  \ \_\ \_\    \ \_\  \ \_____\  \ \_\ \ \_\  \ \_____\  \ \_\\"\_\    \ \_\  \/\_____\
 *	  \/_____/     \/_/   \/_/\/_/     \/_/   \/_____/   \/_/  \/_/   \/_____/   \/_/ \/_/     \/_/   \/_____/
 */


/**
 * Emit compound statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_compound_statement(information *const info, const node *const nd)
{
	const size_t size = statement_compound_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node sub_stmt = statement_compound_get_substmt(nd, i);
		emit_statement(info, &sub_stmt);
	}
}

/**
 *	Emit if statement
 *
 *	@param	info			Encoder
 *	@param	nd				Node in AST
 */
static void emit_if_statement(information *const info, const node *const nd)
{
	uni_printf(info->sx->io, "\n\t# \"if\" statement:\n");

	const item_t old_label = info->label_num;
	info->label_else = info->label_num++;

	const node condition = statement_if_get_condition(nd);
	const rvalue condition_rvalue = emit_expression(info, &condition);
	free_rvalue(info, condition_rvalue);

	const node then_substmt = statement_if_get_then_substmt(nd);
	emit_statement(info, &then_substmt);

	if (statement_if_has_else_substmt(nd))
	{
		info->label_else = old_label;

		to_code_L(info->sx->io, IC_MIPS_J, L_END, old_label);
		to_code_label(info->sx->io, L_ELSE, old_label);

		const node else_substmt = statement_if_get_else_substmt(nd);
		emit_statement(info, &else_substmt);

		to_code_label(info->sx->io, L_END, old_label);
	}
	else
	{
		to_code_label(info->sx->io, L_ELSE, info->label_num);
	}

	uni_printf(info->sx->io, "\n");
}

/**
 * Emit while statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_while_statement(information *const info, const node *const nd)
{
	const item_t label = info->label_num++;
	const item_t old_label = info->label_else;

	info->label_else = label;
	to_code_label(info->sx->io, L_BEGIN_CYCLE, label);
 
	const node condition = statement_while_get_condition(nd);
	rvalue condition_rvalue = emit_expression(info, &condition);

	if (condition_rvalue.kind == CONST)
	{
		// Предварительно загружаем константу в rvalue вида REGISTER
		const rvalue tmp_rval = condition_rvalue;
		condition_rvalue = (rvalue) { .kind = REGISTER
			, .val.reg_num = type_is_floating(tmp_rval.type) ? get_float_register(info) : get_register(info)
			, .type = tmp_rval.type
			, .from_lvalue = !FROM_LVALUE };
		emit_store_rvalue_to_rvalue(info, condition_rvalue, tmp_rval);
	}

	uni_printf(info->sx->io, "\t");
	instruction_to_io(info->sx->io, IC_MIPS_BLTZ);
	uni_printf(info->sx->io, " ");
	rvalue_to_io(info, condition_rvalue);
	uni_printf(info->sx->io, ", ");
	mips_label_to_io(info->sx->io, L_END);
	uni_printf(info->sx->io, "%" PRIitem "\n", label);

	free_rvalue(info, condition_rvalue);
 
	const node body = statement_while_get_body(nd);
	emit_statement(info, &body);

	to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, label);
	to_code_label(info->sx->io, L_ELSE, label);

	info->label_else = old_label;
}

/**
 * Emit do statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_do_statement(information *const info, const node *const nd)
{
	const item_t label = info->label_num++;
	const item_t old_label = info->label_else;

	info->label_else = label;
	to_code_label(info->sx->io, L_BEGIN_CYCLE, label);
 
	const node body = statement_do_get_body(nd);
	emit_statement(info, &body);

	const node condition = statement_do_get_condition(nd);
	const rvalue condition_rvalue = emit_expression(info, &condition);

	uni_printf(info->sx->io, "\t");
	instruction_to_io(info->sx->io, IC_MIPS_BLTZ);
	uni_printf(info->sx->io, " ");
	rvalue_to_io(info, condition_rvalue);
	uni_printf(info->sx->io, ", ");
	mips_label_to_io(info->sx->io, L_END);
	uni_printf(info->sx->io, "%" PRIitem "\n", label);

	to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, label);
	to_code_label(info->sx->io, L_ELSE, label);

	info->label_else = old_label;

	free_rvalue(info, condition_rvalue);
}

/**
 * Emit for statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_for_statement(information *const info, const node *const nd)
{
	const item_t label = info->label_num++;
	const item_t old_label = info->label_else;

	if (statement_for_has_inition(nd))
	{
		// TODO: рассмотреть случаи, если тут объявление
		const node inition = statement_for_get_inition(nd);
		emit_statement(info, &inition);
	}

	info->label_else = label;
	to_code_label(info->sx->io, L_BEGIN_CYCLE, label);

	if (statement_for_has_condition(nd))
	{
		const node condition = statement_for_get_condition(nd);
		const rvalue condition_rvalue = emit_expression(info, &condition);

		uni_printf(info->sx->io, "\t");
		instruction_to_io(info->sx->io, IC_MIPS_BLTZ);
		uni_printf(info->sx->io, " ");
		rvalue_to_io(info, condition_rvalue);
		uni_printf(info->sx->io, ", ");
		mips_label_to_io(info->sx->io, L_END);
		uni_printf(info->sx->io, "%" PRIitem "\n", label);

		free_rvalue(info, condition_rvalue);
	}

	const node body = statement_for_get_body(nd);
	emit_statement(info, &body);

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		const rvalue increment_rvalue = emit_expression(info, &increment);
		free_rvalue(info, increment_rvalue);
	}

	to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, label);
	to_code_label(info->sx->io, L_END, label);

	info->label_else = old_label;
}

/**
 * Emit return statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_return_statement(information *const info, const node *const nd)
{
	// имеет ли функция возвращаемое значение
	if (node_get_amount(nd))
	{
		uni_printf(info->sx->io, "\n\t# return:\n"); 
		const node returning = node_get_child(nd, 0);
		const rvalue returning_rvalue = emit_expression(info, &returning);

		assert(returning_rvalue.kind != VOID);

		const rvalue tmp = { .kind = REGISTER
			, .type = returning_rvalue.type
			, .val.reg_num = (type_is_floating(returning_rvalue.type)) ? R_FV0 : R_V0 };
		emit_store_rvalue_to_rvalue(info, tmp, returning_rvalue);

		free_rvalue(info, returning_rvalue);
	}

	// Прыжок на следующую метку
	uni_printf(info->sx->io, "\n");
	to_code_L(info->sx->io, IC_MIPS_J, L_FUNCEND, info->curr_function_ident);
}

/**
 * Emit declaration statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_declaration_statement(information *const info, const node *const nd)
{
	const size_t size = statement_declaration_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = statement_declaration_get_declarator(nd, i);
		emit_declaration(info, &decl);
	}
}

/**
 * Emit statement
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static void emit_statement(information *const info, const node *const nd)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			emit_declaration_statement(info, nd);
			break;

		case STMT_CASE:
			// emit_case_statement(info, nd);
			break;

		case STMT_DEFAULT:
			// emit_default_statement(info, nd);
			break;

		case STMT_COMPOUND:
			emit_compound_statement(info, nd);
			break;

		case STMT_EXPR:
			emit_void_expression(info, nd);
			break;

		case STMT_NULL:
			break;

		case STMT_IF:
			emit_if_statement(info, nd);
			break;

		case STMT_SWITCH:
			// emit_switch_statement(info, nd);
			break;

		case STMT_WHILE:
			emit_while_statement(info, nd);
			break;

		case STMT_DO:
			emit_do_statement(info, nd);
			break;

		case STMT_FOR:
			emit_for_statement(info, nd);
			break;

		case STMT_CONTINUE:
			to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, info->label_else);
			break;

		case STMT_BREAK:
			to_code_L(info->sx->io, IC_MIPS_J, L_ELSE, info->label_else);
			break;

		case STMT_RETURN:
			emit_return_statement(info, nd); 
			break;

		// case STMT_PRINTF:
		// 	emit_printf_statement(info, nd);
		// 	return;

		// Printid и Getid, которые будут сделаны парсере
		default:
			break;
	}

	uni_printf(info->sx->io, "\n");
}

/**
 * Emit translation unit
 *
 * @param	info			Encoder
 * @param	nd				Node in AST
 */
static int emit_translation_unit(information *const info, const node *const nd)
{
	const size_t size = translation_unit_get_size(nd);
	for (size_t i = 0; i < size; i++)
	{
		const node decl = translation_unit_get_declaration(nd, i);
		emit_declaration(info, &decl);
	}

	return info->sx->rprt.errors != 0;
}


// В дальнейшем при необходимости сюда можно передавать флаги вывода директив
// TODO: подписать, что значит каждая директива и команда
static void pregen(syntax *const sx)
{
	// Подпись "GNU As:" для директив GNU 
	// Подпись "MIPS Assembler:" для директив ассемблера MIPS

	uni_printf(sx->io, "\t.section .mdebug.abi32\n"); // ?
	uni_printf(sx->io, "\t.previous\n"); // следующая инструкция будет перенесена в секцию, описанную выше
	uni_printf(sx->io, "\t.nan\tlegacy\n"); // ?
	uni_printf(sx->io, "\t.module fp=xx\n"); // ?
	uni_printf(sx->io, "\t.module nooddspreg\n"); // ?
	uni_printf(sx->io, "\t.abicalls\n"); // ?
	uni_printf(sx->io, "\t.option pic0\n"); // как если бы при компиляции была включена опция "-fpic" (?) (что означает?)
	uni_printf(sx->io, "\t.text\n"); // последующий код будет перенесён в текстовый сегмент памяти
	uni_printf(sx->io, "\t.align 2\n"); // выравнивание последующих данных/команд по границе, кратной 2^n байт (в данном случае 2^2 = 4)

	uni_printf(sx->io, "\n\t.globl\tmain\n"); // делает метку main глобальной -- её можно вызывать извне кода (например, используется при линковке)
	uni_printf(sx->io, "\t.ent\tmain\n"); // начало процедуры main
	uni_printf(sx->io, "\t.type\tmain, @function\n"); // тип "main" -- функция
	uni_printf(sx->io, "main:\n");

	// инициализация gp
	uni_printf(sx->io, "\tlui $gp, %%hi(__gnu_local_gp)\n"); // "__gnu_local_gp" -- локация в памяти, где лежит Global Pointer
	uni_printf(sx->io, "\taddiu $gp, $gp, %%lo(__gnu_local_gp)\n");

	to_code_2R(sx->io, IC_MIPS_MOVE, R_FP, R_SP);
	to_code_2R_I(sx->io, IC_MIPS_ADDI, R_FP, R_FP, -4);
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_RA, 0, R_FP);
	to_code_R_I(sx->io, IC_MIPS_LI, R_T0, LOW_DYN_BORDER);
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_T0, -(item_t)HEAP_DISPL - 60, R_GP);
	uni_printf(sx->io, "\n");
}

// создаём метки всех строк в программе
static void strings_declaration(information *const info)
{
	uni_printf(info->sx->io, "\t.rdata\n");
	uni_printf(info->sx->io, "\t.align 2\n");

	const size_t amount = strings_amount(info->sx);
	for (size_t i = 0; i < amount; i++)
	{
		item_t args_for_printf = 0; 

		to_code_label(info->sx->io, L_STRING, i);
		uni_printf(info->sx->io, "\t.ascii \"");

		const char *string = string_get(info->sx, i);
		for (size_t j = 0; string[j] != '\0'; j++)
		{
			const char ch = string[j];
			if (ch == '\n')
			{
				uni_printf(info->sx->io, "\\n");
			}
			else if (ch == '%')
			{
				args_for_printf++;
				j++;

				uni_printf(info->sx->io, "%c", ch);
				uni_printf(info->sx->io, "%c", string[j]);

				uni_printf(info->sx->io, "\\0\"\n");
				to_code_label(info->sx->io, L_STRING, i + args_for_printf * amount);
				uni_printf(info->sx->io, "\t.ascii \"");
			}
			else
			{
				uni_printf(info->sx->io, "%c", ch);
			}
		}

		uni_printf(info->sx->io, "\\0\"\n");
	}
	uni_printf(info->sx->io, "\t.text\n");
	uni_printf(info->sx->io, "\t.align 2\n\n");
}

// TODO: подписать, что значит каждая директива и команда
static void postgen(information *const info)
{
	uni_printf(info->sx->io, "\n");
	to_code_L(info->sx->io, IC_MIPS_JAL, L_FUNC, info->main_label);
	to_code_R_I_R(info->sx->io, IC_MIPS_LW, R_RA, 0, R_FP);
	to_code_R(info->sx->io, IC_MIPS_JR, R_RA);

	// вставляем runtime.s в конец файла
	/*
	uni_printf(info->sx->io, "\n\n# runtime\n");
	char *runtime = "../runtimeMIPS/runtime.s"; 
	FILE *file = fopen(runtime, "r+");
	if (runtime != NULL)
	{
		char string[1024];
		while (fgets(string, sizeof(string), file) != NULL)
		{
			uni_printf(info->sx->io, "%s", string);
		}
	}
	fclose(file);
	uni_printf(info->sx->io, "# runtime end\n\n");

	uni_printf(info->sx->io, "\t.end\tmain\n");
	uni_printf(info->sx->io, "\t.size\tmain, .-main\n");
	// TODO: тут ещё часть вывод таблицы типов должен быть (вроде это для написанных самими функции типа printid)
	*/
}


/*
 *	 __	 __   __	 ______   ______	 ______	 ______   ______	 ______	 ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\	\ \_\  \ \_____\  \ \_\ \_\  \ \_\	\ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/	 \/_/   \/_____/   \/_/ /_/   \/_/	 \/_/\/_/   \/_____/   \/_____/
 */


int encode_to_mips(const workspace *const ws, syntax *const sx)
{
	if (!ws_is_correct(ws) || sx == NULL)
	{
		return -1;
	}

	information info;
	info.sx = sx;
	info.main_label = 0;
	info.next_register = R_T0;
	info.next_float_register = R_FT0;
	info.label_num = 1;
	info.label_else = 1;

	info.displacements = hash_create(HASH_TABLE_SIZE);

	pregen(sx);
	strings_declaration(&info);
	// TODO: нормальное получение корня
	const node root = node_get_root(&info.sx->tree);
	const int ret = emit_translation_unit(&info, &root);
	postgen(&info);

	hash_clear(&info.displacements);
	return ret;
}
