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
#include "codes.h"
#include "uniprinter.h"


// Назначение регистров взято из документации SYSTEM V APPLICATION BINARY INTERFACE MIPS RISC Processor, 3rd Edition
// TODO: надо будет ещё добавить регистры для чисел с плавающей точкой
typedef enum REGISTERS
{
	ZERO,						/**< always has the value 0 */	
	AT,							/**< temporary generally used by assembler */

	V0,							/**< used for expression evaluations and to hold the integer
									and pointer type function return values */
	V1,
	
	A0,							/**< used for passing arguments to functions; values are not
									preserved across function calls */
	A1,
	A2,
	A3,

	T0,							/**< temporary registers used for expression evaluation; 
									values are not preserved across function calls */
	T1,
	T2,
	T3,
	T4,
	T5,
	T6,
	T7,

	S0,							/**< saved registers; values are preserved across function
									calls */
	S1,
	S2,
	S3,
	S4,
	S5,
	S6,
	S7,

	T8,							/**< temporary registers used for expression evaluations;
									values are not preserved across function calls.  When
									calling position independent functions $25 must contain
									the address of the called function */
	T9,

	K0,							/**< used only by the operating system */
	K1,

	GP,							/**< global pointer and context pointer */
	SP,							/**< stack pointer */
	FP,							/**< saved register (like s0-s7) or frame pointer */
	RA,							/**< return address.  The return address is the location to
									which a function should return control */
} registers_t;

typedef struct information
{
	syntax *sx;							/**< Структура syntax с таблицами */
} information;


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
	tables_and_tree("tree.txt", &(sx->identifiers), &(sx->types), &(sx->tree));

	information info;
	info.sx = sx;

	precodegen(info.sx);
	// const int ret = codegen(&info);
	return 0;
}
