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


// TODO: надо будет ещё добавить регистры для чисел с плавающей точкой
typedef enum REGISTERS
{
	ZERO,						/**< Всегда хранит ноль */	
	AT,							/**< Зарезервировано для сборки (assembler temporary) */
	V0,							/**< Для возвращаемых функциями значений (values) */
	V1,
	A0,							/**< Первые четыре параметра для функции (arguments) */
	A1,
	A2,
	A3,
	T0,							/**< Для временных значений (temporaries) */
	T1,
	T2,
	T3,
	T4,
	T5,
	T6,
	T7,
	S0,							/**< Для постоянных значений (saved values) */
	S1,
	S2,
	S3,
	S4,
	S5,
	S6,
	S7,
	T8,							/**< Для временных значений (temporaries) */
	T9,
	K0,							/**< Зарезервированы для обработчика (ядро ОС) */
	K1,
	GP,							/**< Указывает на глобальную область (global pointer) */
	SP,							/**< Значение равно верхнему адресу стека (stack pointer) */
	FP,							/**< Указатель на фрейм (frame pointer) */
	RA,							/**< Возвращаемый адрес (return address) */
} register_t;

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
