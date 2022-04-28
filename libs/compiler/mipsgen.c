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
// TODO: расписать, что за данные сохраняются в стеке при вызове
static const size_t FUNC_DISPL = 80;					/**< Смещение в стеке для сохранения данных вызова функции */
static const size_t SP_DISPL = 20;						/**< Смещение в стеке для сохранения значения регистра R_SP */
static const size_t RA_DISPL = 16;						/**< Смещение в стеке для сохранения значения регистра R_RA */


typedef enum ANSWER
{
	A_REG,								/**< Ответ находится в регистре */
	A_CONST,							/**< Ответ является константой */
} answer_t;

typedef enum REQUEST
{
	RQ_REG,								/**< Переменная находится в регистре */
	RQ_REG_CONST,						/**< Переменная находится в регистре или константе */
	RQ_FREE,							/**< Свободный запрос значения */
	RQ_NO_REQUEST,						/**< Нет запроса */
} request_t;

// Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition
// TODO: надо будет ещё добавить регистры для чисел с плавающей точкой
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
										calling position independent functions $25 must contain
										the address of the called function */

	R_K0,
	R_K1,							/**< Used only by the operating system */

	R_GP,							/**< Global pointer and context pointer */
	R_SP,							/**< Stack pointer */
	R_FP,							/**< Saved register (like s0-s7) or frame pointer */
	R_RA,							/**< Return address. The return address is the location to
										which a function should return control */
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

	IC_MIPS_NOP,					/**< o perform no operation */
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

	mips_register_t request_reg;		/**< Регистр на запрос */
	request_t request_kind;				/**< Вид запроса */

	item_t answer_reg;					/**< Регистр с ответом */
	item_t answer_const;				/**< Константа с ответом */
	item_t answer_displ;				/**< Смемещние с ответом */
	answer_t answer_kind;				/**< Вид ответа */

	size_t max_displ;					/**< Максимальное смещение */
	hash displacements;					/**< Хеш таблица с информацией о расположении идентификаторов:
												@с key		 - ссылка на таблицу идентификаторов
												@c value[0]	 - флаг, лежит ли переменная на стеке или в регистре 
												@c value[1]  - смещение или номер регистра */

	mips_register_t next_register;		/**< Следующий регистр для выделения */

	item_t label_num;					/**< Номер метки */

	item_t label_else;					/**< Метка перехода на else */

	bool reverse_logic_command;			/**< Флаг требования противоположной логической операции команды */		
} information;


static void emit_expression(information *const info, const node *const nd);
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


static mips_instruction_t get_instruction(information *const info, const item_t operation_type)
{
	switch (operation_type)
	{
		case BIN_ADD_ASSIGN:
		case BIN_ADD:
			return info->answer_kind == A_CONST ? IC_MIPS_ADDI : IC_MIPS_ADD;

		case BIN_SUB_ASSIGN:
		case BIN_SUB:
			return info->answer_kind == A_CONST ? IC_MIPS_ADDI : IC_MIPS_SUB;

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
			return info->answer_kind == A_CONST ? IC_MIPS_SLL : IC_MIPS_SLLV;

		case BIN_SHR_ASSIGN:
		case BIN_SHR:
			return info->answer_kind == A_CONST ? IC_MIPS_SRA : IC_MIPS_SRAV;

		case BIN_AND_ASSIGN:
		case BIN_AND:
			return info->answer_kind == A_CONST ? IC_MIPS_ANDI : IC_MIPS_AND;

		case BIN_XOR_ASSIGN:
		case BIN_XOR:
			return info->answer_kind == A_CONST ? IC_MIPS_XORI : IC_MIPS_XOR;

		case BIN_OR_ASSIGN:
		case BIN_OR:
			return info->answer_kind == A_CONST ? IC_MIPS_ORI : IC_MIPS_OR;

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
static void emit_literal_expression(information *const info, const node *const nd)
{
	const item_t type = expression_get_type(nd);

	if (type_is_integer(info->sx, type))
	{
		const int num = (int)expression_literal_get_integer(nd);

		if (info->request_kind == RQ_REG)
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, info->request_reg, R_ZERO, num);

			info->answer_kind = A_REG;
			info->answer_reg = info->request_reg;
		}
		else
		{
			info->answer_kind = A_CONST;
			info->answer_const = num;
		}
	}
}

/**
 *	Emit identifier expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_identifier_expression(information *const info, const node *const nd)
{
	const size_t id = expression_identifier_get_id(nd);
	const bool is_on_stack = hash_get(&info->displacements, id, 0) != 0;
	const item_t value_displ = hash_get(&info->displacements, id, 1);

	if (is_on_stack)
	{
		// TODO: глобальные переменные и тип float
		if (info->request_kind == RQ_REG || info->request_kind == RQ_REG_CONST)
		{
			to_code_R_I_R(info->sx->io, IC_MIPS_LW, info->request_reg, -value_displ, R_SP);

			info->answer_kind = A_REG;
			info->answer_reg = info->request_reg;
			info->answer_displ = value_displ;
		}
	}
	// TODO: регистровые переменные
}

/**
 *	Emit call expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_call_expression(information *const info, const node *const nd)
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
			info->request_kind = RQ_REG;
			// TODO: хорошо бы определённый регистр тоже через функцию выделять
			info->request_reg = R_A1;

			const node arg = expression_call_get_argument(nd, i);
			emit_expression(info, &arg);

			uni_printf(info->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + i * amount);
			uni_printf(info->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + i * amount);
			uni_printf(info->sx->io, "\tjal printf\n");
		}

		uni_printf(info->sx->io, "\tlui $t1, %%hi(STRING%zu)\n", index + i * amount);
		uni_printf(info->sx->io, "\taddiu $a0, $t1, %%lo(STRING%zu)\n", index + i * amount);
		uni_printf(info->sx->io, "\tjal printf\n");

		info->request_kind = RQ_NO_REQUEST;
	}
}

/**
 *	Emit increment/decrement expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_inc_dec_expression(information *const info, const node *const nd)
{
	const unary_t operation = expression_unary_get_operator(nd);
	bool was_allocate_reg = false;

	if (!(info->request_kind == RQ_REG_CONST || info->request_kind == RQ_REG))
	{
		info->request_kind = RQ_REG_CONST;
		info->request_reg = get_register(info);
		was_allocate_reg = true;
	}
	const mips_register_t result = info->request_reg;
	const node identifier = expression_unary_get_operand(nd);
	emit_expression(info, &identifier);

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

	to_code_R_I_R(info->sx->io, IC_MIPS_SW, result, -info->answer_displ, R_SP);

	if (operation == UN_POSTDEC)
	{
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, result, R_ZERO, 1);
	}
	else if (operation == UN_POSTINC)
	{
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, result, R_ZERO, -1);
	}

	info->answer_kind = A_REG;
	info->answer_reg = result;
	free_register(info);

	if (was_allocate_reg)
	{
		free_register(info);
		info->request_kind = RQ_NO_REQUEST;
	}
}

/**
 *	Emit unary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_unary_expression(information *const info, const node *const nd)
{
	const unary_t operator = expression_unary_get_operator(nd);
	// const node operand = expression_unary_get_operand(nd);

	switch (operator)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
		case UN_PREINC:
		case UN_PREDEC:
			emit_inc_dec_expression(info, nd);
			return;

		case UN_MINUS:
		case UN_NOT:
		{
			bool was_allocate_reg = false;

			if (!(info->request_kind == RQ_REG_CONST || info->request_kind == RQ_REG))
			{
				info->request_kind = RQ_REG_CONST;
				info->request_reg = get_register(info);
				was_allocate_reg = true;
			}
			const mips_register_t result = info->request_reg;
			const node operand = expression_unary_get_operand(nd);
			emit_expression(info, &operand);

			if (operator == UN_MINUS)
			{
				to_code_3R(info->sx->io, IC_MIPS_SUB, result, R_ZERO, result);
			}
			else
			{
				to_code_2R_I(info->sx->io, IC_MIPS_XORI, result, result, -1);
			}

			info->answer_kind = A_REG;
			info->answer_reg = result;
			free_register(info);

			if (was_allocate_reg)
			{
				free_register(info);
				info->request_kind = RQ_NO_REQUEST;
			}
		}
		break;

		case UN_LOGNOT:
		{
			info->request_kind = RQ_FREE;
			info->reverse_logic_command = !info->reverse_logic_command;
			const node operand = expression_unary_get_operand(nd);
			emit_expression(info, &operand);
		}
		break;

		case UN_ADDRESS:
		case UN_INDIRECTION:
		case UN_ABS:
			break;

		default:
			// TODO: оставшиеся унарные операторы
			return;
	}
}

/**
 *	Emit logic binary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_logic_expression(information *const info, const node *const nd)
{
	const binary_t operation = expression_binary_get_operator(nd);
	bool was_allocate_reg_left = false;

	if (!(info->request_kind == RQ_REG_CONST || info->request_kind == RQ_REG))
	{
		info->request_kind = RQ_REG_CONST;
		info->request_reg = get_register(info);
		was_allocate_reg_left = true;
	}
	const mips_register_t result = info->request_reg;
	const node LHS = expression_binary_get_LHS(nd);
	emit_expression(info, &LHS);

	const answer_t left_kind = info->answer_kind;
	const item_t left_reg = info->answer_kind == A_REG ? info->answer_reg : info->request_reg;
	const item_t left_const = info->answer_const;

	info->request_kind = RQ_REG_CONST;
	info->request_reg = get_register(info);
	const node RHS = expression_binary_get_RHS(nd);
	emit_expression(info, &RHS);

	const answer_t right_kind = info->answer_kind;
	const item_t right_reg = info->answer_kind == A_REG ? info->answer_reg : info->request_reg;
	const item_t right_const = info->answer_const;

	if (left_kind == A_REG && right_kind == A_REG)
	{
		info->reverse_logic_command = !info->reverse_logic_command;

		if (operation == BIN_EQ || operation == BIN_NE)
		{
			to_code_2R_L(info->sx->io, get_instruction(info, operation), left_reg, right_reg
				, L_ELSE, info->label_else);
		}
		else
		{
			to_code_3R(info->sx->io, IC_MIPS_SUB, left_reg, left_reg, right_reg);
			to_code_R_L(info->sx->io, get_instruction(info, operation), left_reg, L_ELSE, info->label_else);
		}
	}
	else if (left_kind == A_REG && right_kind == A_CONST)
	{
		info->reverse_logic_command = !info->reverse_logic_command;

		if (operation == BIN_EQ || operation == BIN_NE)
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, right_reg, R_ZERO, right_const);
			to_code_2R_L(info->sx->io, get_instruction(info, operation), left_reg, right_reg
				, L_ELSE, info->label_else);
		}
		else
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, left_reg, left_reg, -right_const);
			to_code_R_L(info->sx->io, get_instruction(info, operation), left_reg, L_ELSE, info->label_else);
		}
	}
	else if (left_kind == A_CONST && right_kind == A_REG)
	{
		if (operation == BIN_EQ || operation == BIN_NE)
		{
			info->reverse_logic_command = !info->reverse_logic_command;

			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, left_reg, R_ZERO, left_const);
			to_code_2R_L(info->sx->io, get_instruction(info, operation), left_reg, right_reg
				, L_ELSE, info->label_else);
		}
		else
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, right_reg, right_reg, -left_const);
			to_code_R_L(info->sx->io, get_instruction(info, operation), right_reg, L_ELSE, info->label_else);
		}
	}

	// над этими действиями надо позже подумать, когда будут делаться сложные выражения
	info->answer_kind = A_REG;
	info->answer_reg = result;
	info->reverse_logic_command = false;
	free_register(info);

	if (was_allocate_reg_left)
	{
		free_register(info);
		info->request_kind = RQ_NO_REQUEST;
	}
}

/**
 *	Emit non-assignment binary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_integral_expression(information *const info, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	const bool was_allocate_reg_left = info->request_kind != RQ_REG_CONST && info->request_kind != RQ_REG;
	if (was_allocate_reg_left)
	{
		info->request_kind = RQ_REG_CONST;
		info->request_reg = get_register(info);
	}

	const mips_register_t result = info->request_reg;
	const node LHS = expression_binary_get_LHS(nd);
	emit_expression(info, &LHS);

	const answer_t left_kind = info->answer_kind;
	const item_t left_reg = info->answer_kind == A_REG ? info->answer_reg : info->request_reg;
	const item_t left_const = info->answer_const;

	info->request_kind = RQ_REG_CONST;
	info->request_reg = get_register(info);
	const node RHS = expression_binary_get_RHS(nd);
	emit_expression(info, &RHS);

	const answer_t right_kind = info->answer_kind;
	const item_t right_reg = info->answer_kind == A_REG ? info->answer_reg : info->request_reg;
	const item_t right_const = info->answer_const;

	if (left_kind == A_REG && right_kind == A_REG)
	{
		to_code_3R(info->sx->io, get_instruction(info, operator), result, left_reg, right_reg);
	}
	else if (left_kind == A_REG && right_kind == A_CONST)
	{
		// Операции, для которых есть команды, работающие с константами, благодаря чему их можно сделать оптимальнее
		if (operator != BIN_MUL && operator != BIN_DIV && operator != BIN_REM)
		{
			to_code_2R_I(info->sx->io, get_instruction(info, operator), result, left_reg
				, operator != BIN_SUB ? right_const : -right_const);
		}
		else
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, right_reg, R_ZERO, right_const);
			to_code_3R(info->sx->io, get_instruction(info, operator), result, left_reg, right_reg);
		}
	}
	else if (left_kind == A_CONST && right_kind == A_REG)
	{
		// Коммутативные операции, для которых есть команды, работающие с константами
		if (operator == BIN_ADD || operator == BIN_AND || operator == BIN_OR || operator == BIN_XOR)
		{
			info->answer_kind = A_CONST;
			to_code_2R_I(info->sx->io, get_instruction(info, operator), result, right_reg, left_const);
		}
		else
		{
			to_code_2R_I(info->sx->io, IC_MIPS_ADDI, left_reg, R_ZERO, left_const);
			to_code_3R(info->sx->io, get_instruction(info, operator), result, left_reg, right_reg);
		}
	}

	info->answer_kind = A_REG;
	info->answer_reg = result;
	free_register(info);

	if (was_allocate_reg_left)
	{
		free_register(info);
		info->request_kind = RQ_NO_REQUEST;
	}
}

/**
 *	Emit assignment expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_assignment_expression(information *const info, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	const item_t operation_type = expression_get_type(nd);

	const node LHS = expression_binary_get_LHS(nd);

	// TODO: обработать случай, когда слева вырезка или выборка
	const size_t id = expression_identifier_get_id(&LHS);

	// TODO: обработать случай регистровых и глобальных переменных
	const item_t displ = hash_get(&info->displacements, id, 1);
	const bool was_allocate_reg = info->request_kind != RQ_REG_CONST && info->request_kind != RQ_REG;
	if (was_allocate_reg)
	{
		info->request_kind = RQ_REG_CONST;
		info->request_reg = get_register(info);
	}

	const mips_register_t result = info->request_reg;
	const node RHS = expression_binary_get_RHS(nd);
	emit_expression(info, &RHS);

	if (operator != BIN_ASSIGN)
	{
		mips_register_t variable;

		// Операции, для которых есть команды, работающие с константами, благодаря чему их можно сделать оптимальнее
		if (info->answer_kind == A_CONST && operator != BIN_MUL_ASSIGN && operator != BIN_DIV_ASSIGN
			&& operator != BIN_REM_ASSIGN)
		{
			variable = result;

			to_code_R_I_R(info->sx->io, IC_MIPS_LW, variable, -displ, R_SP);
			to_code_2R_I(info->sx->io, get_instruction(info, operator), result, variable
				, operator != BIN_SUB_ASSIGN ? info->answer_const : -info->answer_const);
		}
		else
		{
			variable = get_register(info);

			to_code_R_I_R(info->sx->io, IC_MIPS_LW, variable, -displ, R_SP);
			if (info->answer_kind == A_CONST)
			{
				to_code_2R_I(info->sx->io, IC_MIPS_ADDI, result, R_ZERO, info->answer_const);
			}
			to_code_3R(info->sx->io, get_instruction(info, operator), result, variable, result);

			free_register(info);
		}

		info->answer_kind = A_REG;
	}

	if (info->answer_kind == A_CONST && type_is_integer(info->sx, operation_type)) // A_CONST и операция =
	{
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, result, R_ZERO, info->answer_const);
	}

	to_code_R_I_R(info->sx->io, IC_MIPS_SW, result, -displ, R_SP);

	info->answer_kind = A_REG;
	info->answer_reg = result;

	if (was_allocate_reg)
	{
		free_register(info);
		info->request_kind = RQ_NO_REQUEST;
	}
}

/**
 *	Emit binary expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_binary_expression(information *const info, const node *const nd)
{
	const binary_t operator = expression_binary_get_operator(nd);
	if (operation_is_assignment(operator))
	{
		emit_assignment_expression(info, nd);
		return;
	}

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
			emit_integral_expression(info, nd);
			return;

		case BIN_LT:
		case BIN_GT:
		case BIN_LE:
		case BIN_GE:
		case BIN_EQ:
		case BIN_NE:
			emit_logic_expression(info, nd);
			return;

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
			info->request_kind = RQ_FREE;
			const node LHS = expression_binary_get_LHS(nd);
			emit_expression(info, &LHS);

			info->request_kind = RQ_FREE;
			info->label_else = old_label_else;
			info->reverse_logic_command = false;
			const node RHS = expression_binary_get_RHS(nd);
			emit_expression(info, &RHS);

			to_code_label(info->sx->io, L_ELSE, label_then);
		}
		return;

		default:
			// TODO: оставшиеся бинарные операторы
			return;
	}
}

/**
 *	Emit expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_expression(information *const info, const node *const nd)
{
	switch (expression_get_class(nd))
	{
		case EXPR_CAST:
			// emit_cast_expression(info, nd);
			return;

		case EXPR_IDENTIFIER:
			emit_identifier_expression(info, nd);
			return;

		case EXPR_LITERAL:
			emit_literal_expression(info, nd);
			return;

		case EXPR_SUBSCRIPT:
			// emit_subscript_expression(info, nd);
			return;

		case EXPR_CALL:
			emit_call_expression(info, nd);
			return;

		case EXPR_MEMBER:
			// emit_member_expression(info, nd);
			return;

		case EXPR_UNARY:
			emit_unary_expression(info, nd);
			return;

		case EXPR_BINARY:
			emit_binary_expression(info, nd);
			return;

		default:
			// TODO: генерация оставшихся выражений
			return;
	}
}

/**
 *	Emit expression which will be evaluated as a void expression
 *
 *	@param	info	Encoder
 *	@param	nd		Node in AST
 */
static void emit_void_expression(information *const info, const node *const nd)
{
	const request_t old_request = info->request_kind;
	info->request_kind = RQ_NO_REQUEST;
	emit_expression(info, nd);
	info->request_kind = old_request;
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

	info->max_displ += type_size(info->sx, type) * 4;
	const size_t value_displ = info->max_displ;
	// TODO: в глобальных переменных регистр gp
	const mips_register_t value_reg = R_SP;

	if (!type_is_array(info->sx, type)) // обычная переменная int a; или struct point p;
	{
		const size_t index = hash_add(&info->displacements, id, 2);
		hash_set_by_index(&info->displacements, index, 0, IS_ON_STACK);
		hash_set_by_index(&info->displacements, index, 1, value_displ);
		// TODO: регистровые переменные
		// TODO: вещественные числа

		if (has_init)
		{
			info->request_kind = RQ_REG;
			info->request_reg = get_register(info);

			// TODO: тип char

			const node initializer = declaration_variable_get_initializer(nd);
			emit_expression(info, &initializer);
			to_code_R_I_R(info->sx->io, IC_MIPS_SW, info->request_reg, -(item_t)value_displ, value_reg);

			info->answer_kind = A_REG;
			info->answer_reg = info->request_reg;
			free_register(info);
			info->request_kind = RQ_NO_REQUEST;
		}
	}
	else
	{
		const size_t dimensions = array_get_dim(info, type);
		const item_t element_type = array_get_type(info, type);
		const item_t usual = 1; // предстоит выяснить, что это такое

		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A0, R_ZERO, has_init ? dimensions - 1 : dimensions);
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A1, R_ZERO, type_size(info->sx, element_type) * 4);
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A2, R_ZERO, -(item_t)value_displ);
		to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_A3, R_ZERO, 4 * has_init + usual);
		uni_printf(info->sx->io, "\tjal DEFARR\n");
	}
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

	if (ident_get_prev(info->sx, ref_ident) == TK_MAIN)
	{
		info->main_label = ref_ident;
	}

	to_code_L(info->sx->io, IC_MIPS_J, L_NEXT, ref_ident);
	to_code_label(info->sx->io, L_FUNC, ref_ident);

	// Создание буфера для тела функции
	universal_io *old_io = info->sx->io;
	universal_io new_io = io_create();
  	out_set_buffer(&new_io, BUFFER_SIZE);
	info->sx->io = &new_io;

	// Сохранение данных перед началом работы функции
	to_code_R_I_R(info->sx->io, IC_MIPS_SW, R_SP, SP_DISPL, R_FP);
	to_code_2R(info->sx->io, IC_MIPS_MOVE, R_SP, R_FP);
	to_code_R_I_R(info->sx->io, IC_MIPS_SW, R_RA, RA_DISPL, R_SP);
	uni_printf(info->sx->io, "\n");
	info->max_displ = FUNC_DISPL;

	for (size_t i = 0; i < parameters; i++)
	{
		const size_t id = declaration_function_get_param(nd, i);
		const item_t param_type = ident_get_type(info->sx, id);

		// TODO: сделать параметры
		if (type_is_floating(param_type))
		{

		}
		else
		{

		}
	}

	const node body = declaration_function_get_body(nd);
	emit_statement(info, &body);

	// Выравнивание смещения на 8
	// info->max_displ = (info->max_displ + 7) * 8 / 8;

	// Извлечение буфера с телом функции в старый io
	char *buffer = out_extract_buffer(info->sx->io);
	info->sx->io = old_io;

	// Выделение на стеке памяти для тела функции
	to_code_2R_I(info->sx->io, IC_MIPS_ADDI, R_FP, R_FP, -(item_t)info->max_displ);
	uni_printf(info->sx->io, "%s", buffer);
	free(buffer);

	uni_printf(info->sx->io, "\n");
	to_code_label(info->sx->io, L_FUNCEND, ref_ident);

	// Восстановление стека после работы функции
	to_code_R_I_R(info->sx->io, IC_MIPS_LW, R_RA, RA_DISPL, R_SP);
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
	const item_t label = info->label_num++;

	info->label_else = label;

	info->request_kind = RQ_FREE;
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

	info->request_kind = RQ_FREE;
	const node condition = statement_while_get_condition(nd);
	emit_expression(info, &condition);

	info->request_kind = RQ_NO_REQUEST;
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

	info->request_kind = RQ_NO_REQUEST;
	const node body = statement_do_get_body(nd);
	emit_statement(info, &body);

	info->request_kind = RQ_FREE;
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
			// emit_return_statement(info, nd);
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
	uni_printf(sx->io, "\t.section .mdebug.abi32\n");
	uni_printf(sx->io, "\t.previous\n");
	uni_printf(sx->io, "\t.nan\tlegacy\n");
	uni_printf(sx->io, "\t.module fp=xx\n");
	uni_printf(sx->io, "\t.module nooddspreg\n");
	uni_printf(sx->io, "\t.abicalls\n");
	uni_printf(sx->io, "\t.option pic0\n");
	uni_printf(sx->io, "\t.text\n");
	uni_printf(sx->io, "\t.align 2\n");

	uni_printf(sx->io, "\n\t.globl\tmain\n");
	uni_printf(sx->io, "\t.ent\tmain\n");
	uni_printf(sx->io, "\t.type\tmain, @function\n");
	uni_printf(sx->io, "main:\n");

	// инициализация gp
	uni_printf(sx->io, "\tlui $28, %%hi(__gnu_local_gp)\n");
	uni_printf(sx->io, "\taddiu $28, $28, %%lo(__gnu_local_gp)\n");

	to_code_2R(sx->io, IC_MIPS_MOVE, R_FP, R_SP);
	to_code_2R_I(sx->io, IC_MIPS_ADDI, R_FP, R_FP, -4);
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_RA, 0, R_FP);
	to_code_R_I(sx->io, IC_MIPS_LI, R_T0, LOW_DYN_BORDER);
	to_code_R_I_R(sx->io, IC_MIPS_SW, R_T0, -(item_t)HEAP_DISPL - 60, R_GP);
	uni_printf(sx->io, "\n");
}

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
	to_code_R_I_R(info->sx->io, IC_MIPS_LW, R_RA, -4, R_SP);
	to_code_R(info->sx->io, IC_MIPS_JR, R_RA);

	// char *runtime = "runtime.s";
	// FILE *file = fopen(runtime, "r+");
	// if (runtime != NULL)
	// {
	// 	printf("here1\n");
	// 	printf("%i\n", fgetc(file));
	// 	// char string[1024];
	// 	// while (fgets(string, sizeof(string), file) != NULL)
	// 	// {
	// 	// 	printf("here2\n");
	// 	// 	printf("%s", string);
	// 	// 	// uni_printf(info->sx->io, string);
	// 	// }
	// }

	// fclose(file);

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
	info.request_kind = RQ_NO_REQUEST;
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
