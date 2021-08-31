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
#include "operations.h"
#include "uniprinter.h"


const item_t LOW_DYN_BORDER = 0x10010000;		// Нижняя граница динамической памяти
const item_t HEAP_DISPL = -8000;				// Смещение кучи относительно глобальной памяти
// TODO: расписать, что за данные сохраняются в стеке при вызове
const item_t FUNC_DISPL = 80;					// Смещение в стеке для сохранения данных вызова функции
const item_t SP_DISPL = 20;						// Смещение в стеке для сохранения значения регистра R_SP
const item_t RA_DISPL = 16;						// Смещение в стеке для сохранения значения регистра R_RA


// Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition
// TODO: надо будет ещё добавить регистры для чисел с плавающей точкой
typedef enum REGISTERS
{
	R_ZERO,							/**< always has the value 0 */	
	R_AT,							/**< temporary generally used by assembler */
// used for expression evaluations and to hold the integer
// and pointer type function return values
	R_V0,
	R_V1,
// used for passing arguments to functions; values are not
// preserved across function calls
	R_A0,
	R_A1,
	R_A2,
	R_A3,
// temporary registers used for expression evaluation; 
// values are not preserved across function calls
	R_T0,
	R_T1,
	R_T2,
	R_T3,
	R_T4,
	R_T5,
	R_T6,
	R_T7,
// saved registers; values are preserved across function calls
	R_S0,
	R_S1,
	R_S2,
	R_S3,
	R_S4,
	R_S5,
	R_S6,
	R_S7,
// temporary registers used for expression evaluations;
// values are not preserved across function calls.  When
// calling position independent functions $25 must contain
// the address of the called function
	R_T8,
	R_T9,
// used only by the operating system 
	R_K0,
	R_K1,
	R_GP,							/**< global pointer and context pointer */
	R_SP,							/**< stack pointer */
	R_FP,							/**< saved register (like s0-s7) or frame pointer */
	R_RA,							/**< return address.  The return address is the location to
									which a function should return control */
} mips_register_t;

typedef enum INSTRUCTIONS
{
	INSTR_MOVE,						/**< MIPS Pseudo-Instruction. Move the contents of one register to another */
	INSTR_LI,						/**< MIPS Pseudo-Instruction. Load a constant into a register */
	INSTR_ADDI,						/**< To add a constant to a 32-bit integer. If overflow occurs, then trap */
	INSTR_SW,						/**< To store a word to memory */
	INSTR_LW,						/**< To load a word from memory as a signed value */
	INSTR_JR,						/**< To execute a branch to an instruction address in a register */
	INSTR_JAL,						/**< To execute a procedure call within the current 256MB-aligned region */
	INSTR_J,						/**< To branch within the current 256 MB-aligned region */
} mips_instruction_t;

typedef enum LABELS
{
	L_FUNC,						/**< Тип метки -- вход в функцию */
	L_NEXT,						/**< Тип метки -- следующая функция */
	L_FUNCEND,					/**< Тип метки -- выход из функции */
} mips_label_t;

typedef struct information
{
	syntax *sx;							/**< Структура syntax с таблицами */

	item_t main_label;					/**< Метка функции main */
} information;


static void register_to_io(universal_io *const io, const mips_register_t reg)
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
		case INSTR_MOVE:
			uni_printf(io, "move");
			break;
		case INSTR_LI:
			uni_printf(io, "li");
			break;
		case INSTR_ADDI:
			uni_printf(io, "addi");
			break;
		case INSTR_SW:
			uni_printf(io, "sw");
			break;
		case INSTR_LW:
			uni_printf(io, "lw");
			break;
		case INSTR_JR:
			uni_printf(io, "jr");
			break;
		case INSTR_JAL:
			uni_printf(io, "jal");
			break;
		case INSTR_J:
			uni_printf(io, "j");
			break;
	}
}

static void label_to_io(universal_io *const io, const mips_label_t label)
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
	}
}


// Вид инструкции:	instr	reg1, reg2
static void to_code_2R(universal_io *const io, const mips_instruction_t instruction, const mips_register_t reg1, const mips_register_t reg2)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	register_to_io(io, reg1);
	uni_printf(io, ", ");
	register_to_io(io, reg2);
	uni_printf(io, "\n");
}

// Вид инструкции:	instr	reg1, reg2, imm
static void to_code_2R_I(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg1, const mips_register_t reg2, const item_t imm)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	register_to_io(io, reg1);
	uni_printf(io, ", ");
	register_to_io(io, reg2);
	uni_printf(io, ", %" PRIitem "\n", imm);
}

// Вид инструкции:	instr	reg1, imm(reg2)
static void to_code_R_I_R(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg1, const item_t imm, const mips_register_t reg2)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	register_to_io(io, reg1);
	uni_printf(io, ", %" PRIitem "(", imm);
	register_to_io(io, reg2);
	uni_printf(io, ")\n");
}

// Вид инструкции:	instr	reg1, imm
static void to_code_R_I(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg1, const item_t imm)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	register_to_io(io, reg1);
	uni_printf(io, ", %" PRIitem "\n", imm);
}

// Вид инструкции:	instr	reg1
static void to_code_R(universal_io *const io, const mips_instruction_t instruction
	, const mips_register_t reg1)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	register_to_io(io, reg1);
	uni_printf(io, "\n");
}

// Вид инструкции:	instr	label
static void to_code_L(universal_io *const io, const mips_instruction_t instruction
	, const mips_label_t label, const item_t label_num)
{
	uni_printf(io, "\t");
	instruction_to_io(io, instruction);
	uni_printf(io, " ");
	label_to_io(io, label);
	uni_printf(io, "%" PRIitem "\n", label_num);
}

// Вид инструкции:	label:
static void to_code_label(universal_io *const io, const mips_label_t label, const item_t label_num)
{
	label_to_io(io, label);
	uni_printf(io, "%" PRIitem ":\n", label_num);
}


static int codegen(information *const info)
{
	node root = node_get_root(&info->sx->tree);

	while (true)
	{
		switch (node_get_type(&root))
		{
			case OP_FUNC_DEF:
			{
				const size_t ref_ident = (size_t)node_get_arg(&root, 0);
				// Выравнивание смещения на 8
				const item_t max_displ = (node_get_arg(&root, 1) + 7) * 8 / 8;
				const item_t func_type = ident_get_type(info->sx, ref_ident);
				const size_t parameters = type_function_get_parameter_amount(info->sx, func_type);

				if (ident_get_prev(info->sx, ref_ident) == TK_MAIN)
				{
					info->main_label = ref_ident;
				}

				node_set_next(&root);
				to_code_L(info->sx->io, INSTR_J, L_NEXT, ref_ident);
				to_code_label(info->sx->io, L_FUNC, ref_ident);

				// Выделение на стеке памяти для функции
				to_code_2R_I(info->sx->io, INSTR_ADDI, R_FP, R_FP, -max_displ - FUNC_DISPL);
				// Сохранение данных перед началом работы функции
				to_code_R_I_R(info->sx->io, INSTR_SW, R_SP, SP_DISPL, R_FP);
				to_code_2R(info->sx->io, INSTR_MOVE, R_SP, R_FP);
				to_code_R_I_R(info->sx->io, INSTR_SW, R_RA, RA_DISPL, R_SP);
				uni_printf(info->sx->io, "\n");

				for (size_t i = 0; i < parameters; i++)
				{
					const size_t id = (size_t)node_get_arg(&root, 0);
					const item_t param_type = ident_get_type(info->sx, id);
					node_set_next(&root);

					// TODO: сделать параметры
					if (type_is_floating(param_type))
					{

					}
					else
					{

					}
				}

				// block(info, &root);

				uni_printf(info->sx->io, "\n");
				to_code_label(info->sx->io, L_FUNCEND, ref_ident);
				// Восстановление стека после работы функции
				to_code_R_I_R(info->sx->io, INSTR_LW, R_RA, RA_DISPL, R_SP);
				to_code_2R_I(info->sx->io, INSTR_ADDI, R_FP, R_SP, max_displ + FUNC_DISPL);
				to_code_R_I_R(info->sx->io, INSTR_LW, R_SP, SP_DISPL, R_SP);
				to_code_R(info->sx->io, INSTR_JR, R_RA);
				to_code_label(info->sx->io, L_NEXT, ref_ident);
			}
			break;
			default:
			{
				if (node_set_next(&root) != 0)
				{
					return 0;
				}
			}
		}
	}
}


// В дальнейшем при необходимости сюда можно передавать флаги вывода директив
// TODO: подписать, что значит каждая директива и команда
static void precodegen(syntax *const sx)
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

	to_code_2R(sx->io, INSTR_MOVE, R_FP, R_SP);
	to_code_2R_I(sx->io, INSTR_ADDI, R_FP, R_FP, -4);
	to_code_R_I_R(sx->io, INSTR_SW, R_RA, 0, R_FP);
	to_code_R_I(sx->io, INSTR_LI, R_T0, LOW_DYN_BORDER);
	to_code_R_I_R(sx->io, INSTR_SW, R_T0, HEAP_DISPL - 60, R_GP);
	uni_printf(sx->io, "\n");
}

// TODO: подписать, что значит каждая директива и команда
static void postcodegen(information *const info)
{
	uni_printf(info->sx->io, "\n");
	to_code_L(info->sx->io, INSTR_JAL, L_FUNC, info->main_label);
	to_code_R_I_R(info->sx->io, INSTR_LW, R_RA, -4, R_SP);
	to_code_R(info->sx->io, INSTR_JR, R_RA);

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

	precodegen(info.sx);
	const int ret = codegen(&info);
	postcodegen(&info);
	return ret;
}
