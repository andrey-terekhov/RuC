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

static const size_t LOW_DYN_BORDER = 0x10010000;		/**< Нижняя граница динамической памяти */
static const size_t HEAP_DISPL = 8000;					/**< Смещение кучи относительно глобальной памяти */

static const size_t FUNC_DISPL_PRESEREVED = 80;			/**< Смещение в стеке для сохранения оберегаемых регистров,
															 без учёта оптимизаций */
// s0-s7: 4 байт * 8 = 32 байта     | 
// sp: 4 байта						| => 80 байт 
// ra: 4 байта						| 
// fs0-fs10: 4 байта * 10 = 40 байт |

static const size_t FUNC_DISPL_NONPRESERVED = 112;		/**< Смещение в стеке для сохранения необерегаемых регистров,
															 без учёта оптимизаций */
// t0-t9: 4 байта * 10 = 40 байт    		 | 
// a0-a3: 4 байта * 4 = 16 байт				 | => 112 байт 
// ft0-ft10: 4 байта * 10 = 40 байт 		 |
// fa0-fa3: 4 байта * 4 = 16 байт			 |

static const size_t SP_DISPL = 4;						/**< Смещение в стеке для сохранения значения регистра R_SP */
static const size_t RA_DISPL = 4;						/**< Смещение в стеке для сохранения значения регистра R_RA */


/** Kinds of lvalue */
typedef enum LVALUE_OPERAND
{
	VARIABLE,		/**< Variable operand */
	ADDRESS,		/**< Address operand */
} lvalue_operand_t;
 
typedef struct LVALUE
{ 
	lvalue_operand_t kind;			/**< Value kind */ 
	item_t loc;						/**< Value location */
	item_t type;					/**< Value type */
} lvalue;  

/** Kinds of rvalue */
typedef enum RVALUE_LOC
{
	REGISTER,		
	STACK,		
} stored_in_t;
 
typedef struct RVALUE
{
	stored_in_t stored_in;			/**< Where the value is stored */ 
	item_t loc;						/**< Value location */
	item_t type;					/**< Value type */
} rvalue;  

/** Rvalue or lvalue*/
typedef enum VALUE_TYPE
{	
	LVAL,	
	RVAL,		
} value_t;

typedef struct VALUE
{
	rvalue rv;
	lvalue lv;
	value_t value_type; 
} value;


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

	IC_MIPS_LA,						/**< Load the address of a named memory location into a register (не из вышеуказанной книги)*/

	IC_MIPS_NOP,					/**<To perform no operation */
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
	syntax *sx;							/**< Структура syntax с таблицами */

	item_t main_label;					/**< Метка функции main */

	size_t max_displ;					/**< Максимальное смещение */
	hash displacements;					/**< Хеш таблица с информацией о расположении идентификаторов:
												@с key		 - ссылка на таблицу идентификаторов
												@c value[0]	 - флаг, лежит ли переменная на стеке или в регистре 
												@c value[1]  - смещение или номер регистра */

	mips_register_t next_register;		/**< Следующий обычный регистр для выделения */
	mips_register_t next_f_register;		/**< Следующий регистр с плавающей точкой для выделения */

	item_t label_num;					/**< Номер метки */

	item_t label_else;					/**< Метка перехода на else */

	bool reverse_logic_command;			/**< Флаг требования противоположной логической операции команды */		
} information;


static lvalue emit_lvalue(information *info, const node *const nd);
static value emit_expression(information *const info, const node *const nd);
static void emit_statement(information *const info, const node *const nd);


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


/** Get MIPS assembler instuction 
 * 
 * @param info Codegen info (?)
 * @param operation_type Type of operation in AST
*/
static mips_instruction_t get_instruction(information *const info, const item_t operation_type, const bool is_imm)
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
			return info->reverse_logic_command ? IC_MIPS_BNE : IC_MIPS_BEQ;
		case BIN_NE:
			return info->reverse_logic_command ? IC_MIPS_BEQ : IC_MIPS_BNE;
		case BIN_GT:
			return info->reverse_logic_command ? IC_MIPS_BLEZ : IC_MIPS_BGTZ;
		case BIN_LT:
			return info->reverse_logic_command ? IC_MIPS_BGEZ : IC_MIPS_BLTZ;
		case BIN_GE:
			return info->reverse_logic_command ? IC_MIPS_BLTZ : IC_MIPS_BGEZ;
		case BIN_LE:
			return info->reverse_logic_command ? IC_MIPS_BGTZ : IC_MIPS_BLEZ;

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

// Вид инструкции:	instr	fst_reg, snd_reg, imm
static void to_code_3R(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t fst_reg, const mips_register_t snd_reg, const mips_register_t thd_reg)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, fst_reg);
	uni_printf(io, ", ");
	mips_register_to_io(io, snd_reg);
	uni_printf(io, ", ");
	mips_register_to_io(io, thd_reg);
	uni_printf(io, "\n");
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

// Вид инструкции:	instr	reg, label
static void to_code_R_L(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg , const mips_label_t label, const item_t label_num)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, reg);
	uni_printf(io, ", ");
	mips_label_to_io(io, label);
	uni_printf(io, "%" PRIitem "\n", label_num);
}

// Вид инструкции:	instr	fst_reg, snd_reg, label
static void to_code_2R_L(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t fst_reg, const mips_register_t snd_reg, const mips_label_t label, const item_t label_num)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	mips_register_to_io(io, fst_reg);
	uni_printf(io, ", ");
	mips_register_to_io(io, snd_reg);
	uni_printf(io, ", ");
	mips_label_to_io(io, label);
	uni_printf(io, "%" PRIitem "\n", label_num);
}

// Вид инструкции:	label:
static void to_code_label(universal_io *const io, const mips_label_t label, const item_t label_num)
{
	mips_label_to_io(io, label);
	uni_printf(io, "%" PRIitem ":\n", label_num);
}


// TODO: в этих двух функциях реализовано распределение регистров. Сейчас оно такое
static inline mips_register_t get_register(information *const info)
{
	return info->next_register++;
}

static inline void free_register(information *const info)
{
	info->next_register--;
}

static inline mips_register_t get_register_amount(information *const info)
{
	return info->next_register;
}

static inline mips_register_t get_f_register(information *const info)
{
	return info->next_f_register++;
}

static inline void free_f_register(information *const info)
{
	info->next_f_register--;
}


/*
 *	 ______     __  __     ______   ______     ______     ______     ______     __     ______     __   __     ______
 *	/\  ___\   /\_\_\_\   /\  == \ /\  == \   /\  ___\   /\  ___\   /\  ___\   /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \  __\   \/_/\_\/_  \ \  _-/ \ \  __<   \ \  __\   \ \___  \  \ \___  \  \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \_____\   /\_\/\_\  \ \_\    \ \_\ \_\  \ \_____\  \/\_____\  \/\_____\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/_____/   \/_/\/_/   \/_/     \/_/ /_/   \/_____/   \/_____/   \/_____/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */
 

/**
 *	Emit literal expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static rvalue emit_literal_expression(information *const info, const node *const nd)
{
	const item_t type = expression_get_type(nd);

	// TODO: оставшиеся типы

	// Кладём на стек (иначе непонятно, как действовать -- один и тот же регистр за один expression может быть использован дважды)
	if (type_is_integer(info->sx, type))
	{  
		const mips_register_t tmp_reg = get_register(info);
		to_code_R_I(info->sx->io, IC_MIPS_LI, tmp_reg, expression_literal_get_integer(nd));  
		free_register(info);

		info->max_displ += 4*type_size(info->sx, type); 
		const item_t displ = -(item_t)info->max_displ;

		to_code_R_I_R(info->sx->io, IC_MIPS_SW, tmp_reg, displ, R_SP);

		return (rvalue){ .loc = displ, .stored_in = STACK, .type = type };
	}
	
	return (rvalue){ .loc = 123, .stored_in = REGISTER, .type = TYPE_VOID };
}

/**
 *	Emit identifier expression
 *
 *	@param	info	Codegen info (?)
 *	@param	nd		Node in AST
 */
static lvalue emit_identifier_expression(information *const info, const node *const nd)
{
	const size_t identifier = expression_identifier_get_id(nd);
	const item_t type = ident_get_type(info->sx, identifier);
	const item_t displ = hash_get(&info->displacements, identifier, 1); 

	return (lvalue){ .kind = VARIABLE, .type = type, .loc = displ };
}

/**
 *	Emit call expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static value emit_call_expression(information *const info, const node *const nd)
{
	const node callee = expression_call_get_callee(nd);
	const size_t func_ref = expression_identifier_get_id(&callee);
	const size_t args_amount = expression_call_get_arguments_amount(nd); 

	if (func_ref == BI_PRINTF)
	{
		const node string = expression_call_get_argument(nd, 0);
		const size_t index = expression_literal_get_string(&string);
		const size_t amount = strings_amount(info->sx);

		size_t i = 1;
		for (i = 1; i < args_amount; i++)
		{
			// TODO: хорошо бы определённый регистр тоже через функцию выделять

			const node arg = expression_call_get_argument(nd, i);

			const value arg_value = emit_expression(info, &arg);

			uni_printf(info->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + i * amount);
			uni_printf(info->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + i * amount);
			uni_printf(info->sx->io, "\tjal printf\n");
		}

		uni_printf(info->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + i * amount);
		uni_printf(info->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + i * amount);
		uni_printf(info->sx->io, "\tjal printf\n");
	}
	else if (func_ref >= BEGIN_USER_FUNC)
	{
		size_t f_arg_count = 0;
		size_t arg_count = 0;
		
		uni_printf(info->sx->io, "\n\t#arguments saving\n");

		for (size_t i = 0; i < args_amount; i++)
		{
			const node arg = expression_call_get_argument(nd, i);
			const item_t arg_type = expression_get_type(&arg);
			
			if (type_is_floating(arg_type))
			{
				if (f_arg_count < 4) // в регистр fa0-fa3
				{
					//to_code_R_I(info->sx->io, IC_MIPS_LI, /* код регистра -> */ 36 + f_arg_count, );

				}
				else // иначе на стек 
				{

				}
				f_arg_count++;
			}
			else
			{
				if (arg_count < 4) // в регистр a0-a3
				{
					if (type_is_scalar(info->sx, arg_type))
					{
						// identifier либо literal expression
						if (expression_get_type(&arg) == EXPR_LITERAL)
						{
							//expression_literal_get_integer(&arg);
							//to_code_R_I(info->sx->io, IC_MIPS_LI, /* код регистра -> */ 4 + arg_count, );
						}
						else
						{
							//item_t arg_id = expression_identifier_get_id(&arg);
							//bool in_register = hash_get(&info->displacements, arg_id, 0);
							//if (in_register)
							//	to_code_3R(info->sx->io, IC_MIPS_ADD, /* код регистра -> */ 4 + arg_count,
							//		hash_get(&info->displacements, arg_id, 1), R_ZERO);
						}
					}
				}
				else // иначе на стек
				{

				}
				arg_count++;
			}
		}
	}
	uni_printf(info->sx->io, "\tJAL %s\n", ident_get_spelling(info->sx, func_ref));

	// TODO: При вызове функции
	rvalue res = { .loc = 123, .stored_in = 0, .type = TYPE_INTEGER };
	return (value) { .rv = res, .value_type = 1 };
}

/**
 *	Emit increment/decrement expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static value emit_inc_dec_expression(information *const info, const node *const nd)
{
	const unary_t operation = expression_unary_get_operator(nd);

	const mips_register_t result = get_register(info);
	const node identifier = expression_unary_get_operand(nd);
	const value identifier_value = emit_expression(info, &identifier);

	if (identifier_value.value_type) // RVAL
	{
		if (identifier_value.rv.stored_in == REGISTER)
			to_code_3R(info->sx->io, IC_MIPS_ADD, result, identifier_value.rv.loc, R_ZERO);
		else 
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, result, identifier_value.rv.loc, R_SP); 
	}
	else // LVAL => на стеке
	{
		to_code_R_I_R(info->sx->io, IC_MIPS_LW, result, identifier_value.lv.loc, R_SP);
	}

	switch (operation)
	{
		case UN_PREDEC:
		case UN_POSTDEC:
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, result, R_ZERO, -1);
			break;
		case UN_PREINC:
		case UN_POSTINC:
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, result, R_ZERO, 1);
			break;

		default:
			break;
	}

	to_code_R_I_R(info->sx->io, IC_MIPS_SW, result, identifier_value.lv.loc, R_SP); // Тут точно LVAL?

	free_register(info);

	return identifier_value;
} 

/**
 *	Emit unary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static value emit_unary_expression(information *const info, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd);
	// const node operand = expression_unary_get_operand(nd);

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
			const mips_register_t result = get_register(info);
			const node operand = expression_unary_get_operand(nd);
			const value operand_value = emit_expression(info, &operand);

			if (operator == UN_MINUS)
			{
				to_code_3R(info->sx->io, IC_MIPS_SUB, result, R_ZERO, result);
			}
			else
			{
				to_code_2R_I(info->sx->io, IC_MIPS_XORI, result, result, -1);
			}

			// Закидываем result в расположение результата

			if (operand_value.value_type) // RVAL
			{
				if (operand_value.rv.stored_in == REGISTER)
				{
					to_code_2R(info->sx->io, IC_MIPS_MOVE, operand_value.rv.loc, result);
				}
				else
				{
					to_code_R_I_R(info->sx->io, IC_MIPS_SW, result, operand_value.rv.loc, R_SP);
				}
			}
			else // LVAL => на стеке
			{
				to_code_R_I_R(info->sx->io, IC_MIPS_SW, result, operand_value.lv.loc, R_SP);
			}

			return operand_value;
		}
		break;

		case UN_LOGNOT:
		{
			info->reverse_logic_command = !info->reverse_logic_command;
			const node operand = expression_unary_get_operand(nd);
			return emit_expression(info, &operand);
		}
		break;

		case UN_ADDRESS:
		case UN_INDIRECTION:
		case UN_ABS:
			break;

		default:
			// TODO: оставшиеся унарные операторы
			return (value) {};
	}
	return (value) {};
} 

/**
 *	Emit logic binary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */ 
static value emit_logic_expression(information *const info, const node *const nd)
{
	const binary_t operation = expression_binary_get_operator(nd);

	const mips_register_t curr_reg = get_register_amount(info);

	// TODO: вещественные числа 
	
	const node LHS = expression_binary_get_LHS(nd);
	const value lhs_value = emit_expression(info, &LHS);
	mips_register_t left_reg;
	if (lhs_value.value_type) // lhs -- это RVAL
	{
		left_reg = (lhs_value.rv.stored_in == REGISTER) ? lhs_value.rv.stored_in : get_register(info);
	}
	else // lhs -- это LVAL
	{
		left_reg = get_register(info);
		to_code_R_I_R(info->sx->io, IC_MIPS_LW, left_reg, lhs_value.lv.loc, R_SP);
	}


	const node RHS = expression_binary_get_RHS(nd);
	const value rhs_value = emit_expression(info, &RHS);
	mips_register_t right_reg;
	if (rhs_value.value_type) // rhs -- это RVAL
	{
		right_reg = (rhs_value.rv.stored_in == REGISTER) ? rhs_value.rv.stored_in : get_register(info);
	}
	else // rhs -- это LVAL
	{
		right_reg = get_register(info);
		to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_reg, rhs_value.lv.loc, R_SP);
	}

	info->reverse_logic_command = !info->reverse_logic_command;

	if (operation == BIN_EQ || operation == BIN_NE)
	{
		to_code_2R_L(info->sx->io, get_instruction(info, operation, /* Два регистра => 0 в get_instruction() -> */ 0), left_reg, right_reg,
			L_ELSE, info->label_else);
	}
	else
	{
		to_code_3R(info->sx->io, IC_MIPS_SUB, left_reg, left_reg, right_reg);
		to_code_R_L(info->sx->io, get_instruction(info, operation, /* Два регистра => 0 в get_instruction() -> */ 0), left_reg, 
			L_ELSE, info->label_else);
	}

	info->reverse_logic_command = false;

	while (get_register_amount(info) > curr_reg)
		free_register(info); 

	rvalue res = { .loc = left_reg, .stored_in = REGISTER, .type = (lhs_value.value_type) ? lhs_value.rv.type : lhs_value.lv.type };
	return (value){ .value_type = 1, .rv = res };
} 

/**
 *	Emit non-assignment binary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */ 
static value emit_integral_expression(information *const info, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd); 

	const mips_register_t curr_reg = get_register_amount(info);

	const mips_register_t result = get_register(info);

	const node LHS = expression_binary_get_LHS(nd);
	const value lhs_value = emit_expression(info, &LHS);
	mips_register_t left_reg;

	const node RHS = expression_binary_get_RHS(nd);
	const value rhs_value = emit_expression(info, &RHS);
	mips_register_t right_reg;

	// TODO: вещественные числа

	if (lhs_value.value_type) // lhs -- это RVAL
	{
		if (lhs_value.rv.stored_in == REGISTER) 
			left_reg = lhs_value.rv.loc;
		else
		{
			left_reg = get_register(info);
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, left_reg, lhs_value.rv.loc, R_SP);
		}
	}
	else // lhs -- это LVAL
	{
		left_reg = get_register(info);
		to_code_R_I_R(info->sx->io, IC_MIPS_LW, left_reg, lhs_value.lv.loc, R_SP);
	}

	if (rhs_value.value_type) // rhs -- это RVAL
	{
		if (rhs_value.rv.stored_in == REGISTER)
			right_reg = rhs_value.rv.loc;
		else
		{
			right_reg = get_register(info);
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_reg, rhs_value.rv.loc, R_SP);
		} 
	}
	else // rhs -- это LVAL
	{
		right_reg = get_register(info);
		to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_reg, rhs_value.lv.loc, R_SP);
	}

	to_code_3R(info->sx->io, get_instruction(info, operator, /* Два регистра => 0 в get_instruction() -> */ 0), result, left_reg, right_reg); 
	
	while (get_register_amount(info) > curr_reg)
		free_register(info); 

	rvalue res = { .loc = result, .stored_in = REGISTER, .type = (lhs_value.value_type) ? lhs_value.rv.type : lhs_value.lv.type};

	return (value){ .value_type = 1, .rv = res };
} 

/**
 *	Emit assignment expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static value emit_assignment_expression(information *const info, const node *const nd)
{  
	const mips_register_t curr_reg = get_register_amount(info);

	const binary_t operator = expression_assignment_get_operator(nd);
	const item_t operation_type = expression_get_type(nd);

	const node LHS = expression_assignment_get_LHS(nd);
	const value lhs_value = emit_expression(info, &LHS);
 
	const node RHS = expression_assignment_get_RHS(nd);
	const value rhs_value = emit_expression(info, &RHS);

	mips_register_t result = get_register(info); 
 
	if (operator != BIN_ASSIGN) // "+=", "-=" и т.п. 
	{
		mips_register_t right_reg; 

		if (rhs_value.value_type) // rhs -- это RVAL
		{
			if (rhs_value.rv.stored_in == REGISTER)
			{
				right_reg = rhs_value.rv.loc;
			}
			else
			{
				right_reg = get_register(info);
				to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_reg, rhs_value.rv.loc, R_SP);
			}
		}
		else // rhs -- это LVAL
		{
			right_reg = get_register(info);
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_reg, rhs_value.lv.loc, R_SP);
		}

		to_code_R_I_R(info->sx->io, IC_MIPS_LW, result, /* точно LVAL -> */ lhs_value.lv.loc, R_SP);

		to_code_3R(info->sx->io, get_instruction(info, operator, 0), result, right_reg, result); 
	} 
	else if (type_is_integer(info->sx, operation_type)) 
	{
		if (rhs_value.value_type) // rhs -- это RVAL
		{
			if (rhs_value.rv.stored_in == REGISTER)
				to_code_3R(info->sx->io, IC_MIPS_ADD, result, rhs_value.rv.loc, R_ZERO);
			else
				to_code_R_I_R(info->sx->io, IC_MIPS_LW, result, rhs_value.rv.loc, R_SP);
		}
		else // rhs -- это LVAL => на стеке
		{
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, result, rhs_value.lv.loc, R_SP);
		}
	}
	// TODO: оставшиеся типы

	to_code_R_I_R(info->sx->io, IC_MIPS_SW, result, lhs_value.lv.loc, R_SP);

	while (get_register_amount(info) > curr_reg)
		free_register(info);  
	
	return lhs_value; 
}

/**
 *	Emit binary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */ 
static value emit_binary_expression(information *const info, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);

	switch (operator)
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
			return emit_integral_expression(info, nd);

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
			const item_t label_then = info->label_num++;
			const item_t old_label_else = info->label_else;

			if (operator == BIN_LOG_OR)
			{
				info->label_else = label_then;
				info->reverse_logic_command = true;
			}
			else
			{
				info->label_else = old_label_else;
				info->reverse_logic_command = false;
			}

			const mips_register_t curr_reg = get_register_amount(info);

			const node LHS = expression_binary_get_LHS(nd);
			const value lhs_value = emit_expression(info, &LHS);
			mips_register_t left_register; 
			if  (lhs_value.value_type) // lhs -- это RVAL
			{
				if (lhs_value.rv.stored_in == REGISTER)
					left_register = lhs_value.rv.loc;
				else
				{
					left_register = get_register(info);
					to_code_R_I_R(info->sx->io, IC_MIPS_LW, left_register, lhs_value.rv.loc, R_SP);
				}
			} 
			else
			{
				left_register = get_register(info);
				to_code_R_I_R(info->sx->io, IC_MIPS_LW, left_register, lhs_value.lv.loc, R_SP);
			}

			info->label_else = old_label_else;
			info->reverse_logic_command = false;
			const node RHS = expression_binary_get_RHS(nd);
			const value rhs_value = emit_expression(info, &RHS); 
			mips_register_t right_register; 
			if  (rhs_value.value_type) // rhs -- это RVAL
			{
				if (rhs_value.rv.stored_in == REGISTER)
					left_register = rhs_value.rv.loc;
				else
				{
					right_register = get_register(info);
					to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_register, rhs_value.rv.loc, R_SP);
				}
			} 
			else
			{
				right_register = get_register(info);
				to_code_R_I_R(info->sx->io, IC_MIPS_LW, right_register, rhs_value.lv.loc, R_SP);
			}

			mips_register_t result = get_register(info);

			to_code_3R(info->sx->io, (operator == BIN_LOG_OR) ? IC_MIPS_OR : IC_MIPS_AND, left_register, left_register, right_register);

			to_code_label(info->sx->io, L_ELSE, label_then);

			while (get_register_amount(info) > curr_reg)
				free_register(info);
 
			rvalue res = { .loc = result, .type = (lhs_value.value_type) ? lhs_value.rv.type : lhs_value.lv.type, .stored_in = REGISTER };
			return (value) { .value_type = 1, .rv = res };
		} 

		default:
			// TODO: оставшиеся бинарные операторы
			return (value) { .value_type = 1};
	}
}


/**
 * Emit lvalue type expression
 * 
 * @param info		Information
 * @param nd		Node in AST
 */
static lvalue emit_lvalue(information *info, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_IDENTIFIER:
			return emit_identifier_expression(info, nd);

		case EXPR_SUBSCRIPT:
			//return emit_subscript_expression(info, nd);

		case EXPR_MEMBER:
			//return emit_member_expression(info, nd);

		case EXPR_UNARY: 
		{ 
			
		}

		default:
			// Cannot be an lvalue
			system_error(node_unexpected, nd);
			return (lvalue){ .loc = ITEM_MAX };
	}

}


/**
 *	Emit expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static value emit_expression(information *const info, const node *const nd)
{
	if (expression_is_lvalue(nd))
	{
		lvalue val = emit_lvalue(info, nd);
		return (value){ .lv = val, .value_type = 0}; 
	}

	// Иначе rvalue:
	value val;

	mips_register_t curr_reg = get_register_amount(info);
	switch (expression_get_class(nd))
	{
		/*
		case EXPR_CAST:
			return emit_cast_expression(info, nd);
		*/
		case EXPR_LITERAL:
			val = (value){ .rv = emit_literal_expression(info, nd), .value_type = 1 };
			break;
		/*
		case EXPR_SUBSCRIPT:
			return emit_subscript_expression(info, nd);
		*/ 
		case EXPR_CALL:
			val = emit_call_expression(info, nd); 
			break;
		/*
		case EXPR_MEMBER:
			return emit_member_expression(info, nd);
		*/
		
		case EXPR_UNARY:
			val = emit_unary_expression(info, nd);
			break;
		
		case EXPR_BINARY:
			val = emit_binary_expression(info, nd); 
			break;
		
		case EXPR_ASSIGNMENT:
			val = emit_assignment_expression(info, nd); 
			break;
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
			return (value){ .value_type = 1};
		
	}
	
	while(get_register_amount(info) > curr_reg)
		free_register(info);
	
	return val;
} 

/**
 *	Emit expression which will be evaluated as a void expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_void_expression(information *const info, const node *const nd)
{ 
	emit_expression(info, nd); 
}


/*
 *	 _____     ______     ______     __         ______     ______     ______     ______   __     ______     __   __     ______
 *	/\  __-.  /\  ___\   /\  ___\   /\ \       /\  __ \   /\  == \   /\  __ \   /\__  _\ /\ \   /\  __ \   /\ "-.\ \   /\  ___\
 *	\ \ \/\ \ \ \  __\   \ \ \____  \ \ \____  \ \  __ \  \ \  __<   \ \  __ \  \/_/\ \/ \ \ \  \ \ \/\ \  \ \ \-.  \  \ \___  \
 *	 \ \____-  \ \_____\  \ \_____\  \ \_____\  \ \_\ \_\  \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \ \_____\  \ \_\\"\_\  \/\_____\
 *	  \/____/   \/_____/   \/_____/   \/_____/   \/_/\/_/   \/_/ /_/   \/_/\/_/     \/_/   \/_/   \/_____/   \/_/ \/_/   \/_____/
 */


/**
 *	Emit variable declaration
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_variable_declaration(information *const info, const node *const nd)
{
	const size_t id = declaration_variable_get_id(nd);
	const bool has_init = declaration_variable_has_initializer(nd);
	const item_t type = ident_get_type(info->sx, id);

	uni_printf(info->sx->io, "\t# \"%s\" variable declaration:\n", ident_get_spelling(info->sx, id));

	info->max_displ += type_size(info->sx, type) * 4;
	const size_t value_displ = info->max_displ; 

	// TODO: в глобальных переменных регистр gp
	// просто эту конструкцию в рекурсивную функцию
	/*
	node parent_node = node_get_parent(nd);
	item_t parent_type = node_get_type(&parent_node);
	const mips_register_t value_reg = (type_is_function(info->sx, parent_type)) ? R_SP : R_GP;
	*/
	const mips_register_t value_reg = R_SP;

	const size_t index = hash_add(&info->displacements, id, 2);
	hash_set_by_index(&info->displacements, index, 0, IS_ON_STACK);
	hash_set_by_index(&info->displacements, index, 1, -(item_t)value_displ);

	if (!type_is_array(info->sx, type)) // обычная переменная int a; или struct point p;
	{ 
		// TODO: вещественные числа
		// TODO: структуры 
		// TODO: массивы 

		if (has_init)
		{
			// TODO: тип char

			const node initializer = declaration_variable_get_initializer(nd);
			const value initializer_value = emit_expression(info, &initializer);

			// TODO: А если мы имеем дело с глобальными переменными? value_reg не работает
			if (initializer_value.value_type) // RVAL
			{
				if (initializer_value.rv.stored_in == REGISTER)
					to_code_R_I_R(info->sx->io, IC_MIPS_SW, initializer_value.rv.loc, -(item_t)value_displ, value_reg); 
				else 
				{
					// копируем значение с одного положения в стеке в другое
					mips_register_t tmp_reg = get_register(info);
					to_code_R_I_R(info->sx->io, IC_MIPS_LW, tmp_reg, initializer_value.rv.loc, value_reg);
					to_code_R_I_R(info->sx->io, IC_MIPS_SW, tmp_reg, -(item_t)value_displ, value_reg); 
					free_register(info);
				}
			}
			else // LVAL => на стеке
			{
				// копируем значение с одного положения в стеке в другое
				mips_register_t tmp_reg = get_register(info);
				to_code_R_I_R(info->sx->io, IC_MIPS_LW, tmp_reg, initializer_value.lv.loc, value_reg);
				to_code_R_I_R(info->sx->io, IC_MIPS_SW, tmp_reg, -(item_t)value_displ, value_reg);
				free_register(info);
			} 
		}
	}
	else
	{
		const size_t dimensions = array_get_dim(info, type);
		const item_t element_type = array_get_type(info, type);
		const item_t usual = 1; // предстоит выяснить, что это такое
 
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A0, R_ZERO, has_init ? dimensions - 1 : dimensions); // передаём размерность
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A1, R_ZERO, type_size(info->sx, element_type) * 4); // передаём размер элемента
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A2, R_ZERO, -(item_t)value_displ); // передаём смещение относительно fp (положительное значение) или gp (отрицательное значение)
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A3, R_ZERO, 4 * has_init + usual);
		uni_printf(info->sx->io, "\tjal DEFARR\n");
		uni_printf(info->sx->io, "\t#addr 0($fp) now contains array size, addr 4($fp) contains first array element\n");
	}
	uni_printf(info->sx->io, "\n");
}

/**
 * Emit function definition
 *
 * @param	info	Encoder
 * @param	nd		Node in AST
 */
static void emit_function_definition(information *const info, const node *const nd)
{
	const size_t ref_ident = declaration_function_get_id(nd);
	const item_t func_type = ident_get_type(info->sx, ref_ident);
	const size_t parameters = type_function_get_parameter_amount(info->sx, func_type);
 
	bool is_main = 0;
	if (ident_get_prev(info->sx, ref_ident) == TK_MAIN)
	{
		info->main_label = ref_ident;
		is_main = 1;
	} 

	to_code_L(info->sx->io, IC_MIPS_J, L_NEXT, ref_ident);
	to_code_label(info->sx->io, L_FUNC, ref_ident);

	// Создание буфера для тела функции
	universal_io *old_io = info->sx->io;
	universal_io new_io = io_create();
  	out_set_buffer(&new_io, BUFFER_SIZE);
	info->sx->io = &new_io;

	uni_printf(info->sx->io, "\t#\"%s\" function:\n", ident_get_spelling(info->sx, ref_ident));

	if (!is_main)
	{
		// Сохранение данных перед началом работы функции
		info->max_displ = FUNC_DISPL_PRESEREVED;
		
		uni_printf(info->sx->io, "\n\t#data saving:\n");
		
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_FP, -(item_t)info->max_displ); 	
		to_code_R_I_R(info->sx->io, IC_MIPS_SW, R_SP, SP_DISPL, R_FP);
		to_code_2R(info->sx->io, IC_MIPS_MOVE, R_SP, R_FP);  

		to_code_R_I_R(info->sx->io, IC_MIPS_SW, R_RA, RA_DISPL, R_SP);

		uni_printf(info->sx->io, "\n");

		// Не вынесено в отдельную подпрограмму для упрощения последующего создания оптимизаций
		// TODO: оптимизация обращений к памяти
		// Сохранение s0-s7
		for (size_t i = 0; i < 8; i++)
			to_code_R_I_R(info->sx->io, IC_MIPS_SW, /* код регистра -> */ 16 + i, /* 4 за ra (sp уже учтен) + смещение -> */ 4 + 4*(i+1), R_SP);
		
		uni_printf(info->sx->io, "\n");

		// Сохранение fs0-fs7
		for (size_t i = 0; i < 10; i++)
			to_code_R_I_R(info->sx->io, IC_MIPS_SW, /* код регистра -> */ 52 + i, /* 36 за ra + s0-s7 + смещение -> */ 36 + 4*(i+1), R_SP);

		uni_printf(info->sx->io, "\n"); 
	}

	uni_printf(info->sx->io, "\n\t#argument loading:\n"); 

	size_t f_arg_count = 0;
	size_t arg_count = 0;
	size_t arg_displ_sum = 0; 
	for (size_t i = 0; i < parameters; i++)
	{
		const size_t id = declaration_function_get_param(nd, i);
		const item_t param_type = ident_get_type(info->sx, id); 

		// Вносим переменную в таблицу символов 
		const size_t index = hash_add(&info->displacements, id, 2);

		uni_printf(info->sx->io, "\t#argument \"%s\":\n", ident_get_spelling(info->sx, id));

		// Первые 4 аргумента в a0-a3 (либо в fa0-fa3), остальные переданы вызывающей функцией над fp 
		if (type_is_floating(param_type))
		{ 
			if (f_arg_count < 12) // количество временных регистров
			{
				if (f_arg_count < 4) 
					// берём из fa0-fa3
					to_code_2R(info->sx->io, IC_MIPS_MOVE, /* код регистра ft0-ft11 -> */ 40 + f_arg_count, /* код регистра fa0-fa3 -> */ 36 + f_arg_count);
				else
				{
					// берём со стека
					arg_displ_sum += 4; 
					to_code_R_I_R(info->sx->io, IC_MIPS_LW, /* код регистра ft0-ft11 -> */ 40 + f_arg_count, arg_displ_sum, R_FP);
				}
				
				// Все переменные обязательно должны быть на стеке
				to_code_R_I_R(info->sx->io, IC_MIPS_SW, 40 + f_arg_count, -(item_t)(arg_displ_sum + 4), R_SP);

				hash_set_by_index(&info->displacements, index, 1, -(item_t)(arg_displ_sum + 4));
			}
			// иначе остаётся на стеке
			else 
			{
				uni_printf(info->sx->io, "\t#stays on stack\n");

				hash_set_by_index(&info->displacements, index, 1, (arg_displ_sum + 4));
			}

			f_arg_count++;
			get_f_register(info);

			hash_set_by_index(&info->displacements, index, 0, IS_ON_STACK);
		}
		else 
		{ 
			if (arg_count < 8) // количество временных регистров
			{  
				if (arg_count < 4) 
					// берём из a0-a3
					to_code_2R(info->sx->io, IC_MIPS_MOVE, /* код регистра t0-t7 -> */ 8 + arg_count, /* код регистра a0-a3 -> */ 4 + arg_count);
				else 
				{
					// берём со стека
					arg_displ_sum += 4;
					to_code_R_I_R(info->sx->io, 
						/* в случае не скалярного типа берём адрес первого элемента */ type_is_scalar(info->sx, param_type) ? IC_MIPS_LW : IC_MIPS_LA, 
						/* код регистра t0-t7 -> */ 8 + arg_count, arg_displ_sum, R_FP); 

					// FIXME: лучше бы это обернуть в функцию
					// нужно узнать смещение (в случае скалярного типа увеличить на 4 в начале достаточно, поэтому считаем только для aggregate)
					if (type_is_aggregate(info->sx, param_type))
					{
						if (type_is_structure(info->sx, param_type))
							// FIXME: обязательно четыре?
							arg_displ_sum += (type_structure_get_member_amount(info->sx, param_type) - 1)*4*
								type_size(info->sx, param_type); // структура обязана иметь поля, так что безопасно
						else 
						{
							// TODO: массивы
							// адрес размера массива, следом за ним идут элементы
							item_t arr_displ = hash_get(&info->displacements, id, 1); 
							if (i != parameters - 1) // иначе нам всё равно
							{
								// находим адрес следующего элемента (чтобы посчитать смещение для следующего аргумента)
								// он ниже на стеке (т.е. смещение у него больше)
								item_t next_element_id = declaration_function_get_param(nd, i + 1);
								item_t next_element_displ = hash_get(&info->displacements, next_element_id, 1); 
								arg_displ_sum += next_element_displ - arr_displ; // берём вместе с размером массива (он уже учтён в увеличении на 4 выше)
								// не забыть учесть то, что на нужное кол-во байт мы уже сместились выше
							}
						}
					} 
				}

				// Все переменные обязательно должны быть на стеке
				to_code_R_I_R(info->sx->io, IC_MIPS_SW, 8 + arg_count, -(item_t)(arg_displ_sum + 4), R_SP);

				hash_set_by_index(&info->displacements, index, 1, -(item_t)(arg_displ_sum + 4));
			}
			// иначе остаётся на стеке
			else 
			{
				uni_printf(info->sx->io, "\t#stays on stack\n");

				hash_set_by_index(&info->displacements, index, 1, (arg_displ_sum + 4));
			} 
			
			arg_count++;
			get_register(info);

			hash_set_by_index(&info->displacements, index, 0, IS_ON_STACK);
		}

		uni_printf(info->sx->io, "\n");
	}

	uni_printf(info->sx->io, "\n\t#function body:\n");
 
	node body = declaration_function_get_body(nd);
	emit_statement(info, &body);

	// Выравнивание смещения на 8
	info->max_displ += info->max_displ % 8;

	// Извлечение буфера с телом функции в старый io
	char *buffer = out_extract_buffer(info->sx->io);
	info->sx->io = old_io;

	// Выделение на стеке памяти для тела функции  
	uni_printf(info->sx->io, "%s", buffer);
	free(buffer);

	to_code_label(info->sx->io, L_FUNCEND, ref_ident);


	if (!is_main)
	{
		// Восстановление стека после работы функции
		uni_printf(info->sx->io, "\n\t#data restoring:\n");

		to_code_R_I_R(info->sx->io, IC_MIPS_LW, R_RA, RA_DISPL, R_SP);

		uni_printf(info->sx->io, "\n");

		// Восстановление s0-s7
		for (size_t i = 0; i < 8; i++)
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, /* код регистра -> */ 16 + i, /* 4 за ra (sp уже учтен) + смещение -> */ 4 + 4*(i+1), R_SP);
		
		uni_printf(info->sx->io, "\n");

		// Восстановление fs0-fs7
		for (size_t i = 0; i < 10; i++)
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, /* код регистра -> */ 52 + i, /* 36 за ra + s0-s7 + смещение -> */ 36 + 4*(i+1), R_SP); 
	}

	uni_printf(info->sx->io, "\n");

	to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_SP, info->max_displ);
	to_code_R_I_R(info->sx->io, IC_MIPS_LW, R_SP, SP_DISPL, R_SP);
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
 *	Emit compound statement
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
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
 *	@param	info		Encoder
 *	@param	nd			Node in AST
 */
static void emit_if_statement(information *const info, const node *const nd)
{
	uni_printf(info->sx->io, "\n\t#\"if\" statement:\n");
	const item_t label = info->label_num++;

	info->label_else = label;

	const node condition = statement_if_get_condition(nd);
	emit_expression(info, &condition);

	const node then_substmt = statement_if_get_then_substmt(nd);
	emit_statement(info, &then_substmt);

	if (statement_if_has_else_substmt(nd))
	{
		to_code_L(info->sx->io, IC_MIPS_J, L_END, label);
		to_code_label(info->sx->io, L_ELSE, label);

		const node else_substmt = statement_if_get_else_substmt(nd);
		emit_statement(info, &else_substmt);

		to_code_label(info->sx->io, L_END, label);
	}
	else
	{
		to_code_label(info->sx->io, L_ELSE, label);
	}

	uni_printf(info->sx->io, "\n");
}

/**
 *	Emit while statement
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
 */
static void emit_while_statement(information *const info, const node *const nd)
{
	const item_t label = info->label_num++;
	const item_t old_label = info->label_else;

	info->label_else = label;
	to_code_label(info->sx->io, L_BEGIN_CYCLE, label);
 
	const node condition = statement_while_get_condition(nd);
	emit_expression(info, &condition);
 
	const node body = statement_while_get_body(nd);
	emit_statement(info, &body);

	to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, label);
	to_code_label(info->sx->io, L_ELSE, label);

	info->label_else = old_label;
}

/**
 *	Emit do statement
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
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
	emit_expression(info, &condition);

	to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, label);
	to_code_label(info->sx->io, L_ELSE, label);

	info->label_else = old_label;
}

/**
 *	Emit for statement
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
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
		emit_expression(info, &condition);
	}

	const node body = statement_for_get_body(nd);
	emit_statement(info, &body);

	if (statement_for_has_increment(nd))
	{
		const node increment = statement_for_get_increment(nd);
		emit_expression(info, &increment);
	}

	to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, label);
	to_code_label(info->sx->io, L_ELSE, label);

	info->label_else = old_label;
} 

static node get_parent_function(information *const info, const node *const nd)
{
	if (!node_is_correct(nd))
		return node_broken();

	const item_t nd_type = node_get_type(nd);
	if (nd_type == OP_FUNC_DEF)
		return *nd;

	node parent = node_get_parent(nd);
	return get_parent_function(info, &parent);

}

/**
 *	Emit return statement
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
 */
static void emit_return_statement(information *const info, const node *const nd)
{
	// имеет ли функция возвращаемое значение
	if (node_get_amount(nd))
	{
		uni_printf(info->sx->io, "\n\t#return:\n");
		const node returning = node_get_child(nd, 0); // либо identifier, либо literal
		const item_t ret_node_type = node_get_type(&returning);
		const item_t ret_val_type = expression_get_type(&returning);
		if (ret_node_type == OP_IDENTIFIER)
		{
			// сначала смотрим, лежит ли переменная в регистре или на стеке
			const item_t ident_id = expression_identifier_get_id(&returning);

			const bool on_stack = hash_get(&info->displacements, ident_id, 0);
			const size_t reg_or_displ = hash_get(&info->displacements, ident_id, 1); 

			if (on_stack) // если на стеке
				to_code_R_I_R(info->sx->io, IC_MIPS_LW, type_is_floating(ret_val_type) ? R_FV0 : R_V0, reg_or_displ, R_SP);
			else // в регистре
				to_code_2R(info->sx->io, IC_MIPS_MOVE,  type_is_floating(ret_val_type) ? R_FV0 : R_V0, reg_or_displ);
		}
		else // literal
		{
			switch (ret_val_type)
			{
				case TYPE_INTEGER:
					to_code_R_I(info->sx->io, IC_MIPS_LI, R_V0, expression_literal_get_integer(&returning));
					break;
				
				case TYPE_CHARACTER:
					to_code_R_I(info->sx->io, IC_MIPS_LI, R_V0, expression_literal_get_character(&returning));
					break;
				
				case TYPE_BOOLEAN:
					to_code_R_I(info->sx->io, IC_MIPS_LI, R_V0, expression_literal_get_boolean(&returning));
					break;

				case TYPE_FLOATING:
					to_code_R_I(info->sx->io, IC_MIPS_LI, R_FV0, expression_literal_get_floating(&returning));
					break;
				
				case TYPE_ARRAY: // literal array может быть только string
					// TODO: положить на стек и потом отдавать адрес?
					//to_code_R_I(info->sx->io, IC_MIPS_LA, R_V0, expression_literal_get_string(&returning));
					break;
				
				default:
					break;
			}
		}
	}

	// Прыжок на следующую метку
	uni_printf(info->sx->io, "\n");
	node parent_func = get_parent_function(info, nd);
	if (!node_is_correct(&parent_func))
		return;
	const size_t parent_func_id = declaration_function_get_id(&parent_func);
	to_code_L(info->sx->io, IC_MIPS_J, L_FUNCEND, parent_func_id); 
}

/**
 *	Emit translation unit
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
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
 *	Emit statement
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
 */
static void emit_statement(information *const info, const node *const nd)
{
	switch (statement_get_class(nd))
	{
		case STMT_DECL:
			emit_declaration_statement(info, nd);
			return;

		case STMT_CASE:
			// emit_case_statement(info, nd);
			return;

		case STMT_DEFAULT:
			// emit_default_statement(info, nd);
			return;

		case STMT_COMPOUND:
			emit_compound_statement(info, nd);
			return;

		case STMT_EXPR:
			emit_void_expression(info, nd);
			return;

		case STMT_NULL:
			return;

		case STMT_IF:
			emit_if_statement(info, nd);
			return;

		case STMT_SWITCH:
			// emit_switch_statement(info, nd);
			return;

		case STMT_WHILE:
			emit_while_statement(info, nd);
			return;

		case STMT_DO:
			emit_do_statement(info, nd);
			return;

		case STMT_FOR:
			emit_for_statement(info, nd);
			return;

		case STMT_CONTINUE:
			to_code_L(info->sx->io, IC_MIPS_J, L_BEGIN_CYCLE, info->label_else);
			return;

		case STMT_BREAK:
			to_code_L(info->sx->io, IC_MIPS_J, L_ELSE, info->label_else);
			return;

		case STMT_RETURN:
			emit_return_statement(info, nd);
			uni_printf(info->sx->io, "\n");
			//const size_t func_id = declaration_function_get_id(nd);
			//to_code_L(info->sx->io, IC_MIPS_J, L_FUNCEND, func_id); 
			return;

		// case STMT_PRINTF:
		// 	emit_printf_statement(info, nd);
		// 	return;

		// Printid и Getid, которые будут сделаны парсере
		default:
			return;
	}
}

/**
 *	Emit translation unit
 *
 *	@param	info		Encoder
 *	@param	nd			Node in AST
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
	uni_printf(sx->io, "\tlui $28, %%hi(__gnu_local_gp)\n"); // "__gnu_local_gp" -- локация в памяти, где лежит Global Pointer
	uni_printf(sx->io, "\taddiu $28, $28, %%lo(__gnu_local_gp)\n");

	to_code_2R(sx->io, IC_MIPS_MOVE, R_FP, R_SP); // инициализация frame pointer
	to_code_2R_I(sx->io, IC_MIPS_ADDI, R_FP, R_FP, -4); // выделяем память на стеке
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_RA, 0, R_FP); // return address отправляется в стек по адресу frame pointer
	to_code_R_I(sx->io, IC_MIPS_LI, R_T0, LOW_DYN_BORDER); // ?
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_T0, -(item_t)HEAP_DISPL - 60, R_GP); // ?
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
	to_code_L(info->sx->io, IC_MIPS_JAL, L_FUNC, info->main_label); // прыгаем обратно в main
	to_code_R_I_R(info->sx->io, IC_MIPS_LW, R_RA, -4, R_SP); // загружаем в return address значение из -4($sp) (что там находится?)
	to_code_R(info->sx->io, IC_MIPS_JR, R_RA); // прыгаем на это значение

	// вставляем runtime.s в конец файла
	uni_printf(info->sx->io, "\n\n# runtime\n");
	//char *runtime = "../runtimeMIPS/runtime.s";
	// FIXME: корректный путь к runtime
	char *runtime = "/home/mexanobar/programming/RuC/RuC/branch/mips_codegen/RuC/runtimeMIPS/runtime.s";
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
	info.max_displ = 0;
	info.next_register = R_T0;
	info.next_f_register = R_FT0; 
	info.label_num = 1;
	info.label_else = 1;
	info.reverse_logic_command = false;

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
