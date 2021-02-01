/*
 *	Copyright 2020 Andrey Terekhov, Maxim Menshikov, Dmitrii Davladov
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

#pragma once

#include "defs.h"
#include "syntax.h"
#include "uniio.h"
#include "lexer.h"


#define REPRTAB		(context->sx->reprtab)
#define REPRTAB_POS (context->lxr->repr)
#define REPRTAB_LEN (context->sx->rp)


#ifdef __cplusplus
extern "C" {
#endif

/** Определение глобальных переменных */
typedef struct
{
	universal_io *io;					/**< Universal io structure */
	syntax *sx;							/**< Syntax structure */
	lexer *lxr;							/**< Lexer structure */

	token curr_token;					/**< Current token */
	token next_token;					/**< Lookahead token */
	int line;
	int flag_was_type_def;
	int func_def;	// context->func_def = 0 - (),
					// 1 - определение функции,
					// 2 - это предописание,
					// 3 - не знаем или вообще не функция
	int stack[100];
	int stackop[100];
	int stackoperands[100];
	int stacklog[100];
	int sp;
	int sopnd;
	int lastid;
	int type;
	int op;
	int inass;
	int arrdim;
	int was_struct_with_arr;
	int usual;
	int flag_in_switch;
	int flag_in_loop;
	int function_type;
	int flag_was_return;
	int gotost[1000];
	int pgotost;
	int anst;
	int ansttype;
	int anstdispl;
	int leftansttype; // anst = VAL  - значение на стеке
	int x;							// useless
	char32_t bad_printf_placeholder;
	int onlystrings;

	int buf_flag;
	int buf_cur;
	int was_error;
} parser;


/**
 *	Analyze source code to generate syntax structure
 *
 *	@param	io		Universal io structure
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c error_code on failure
 */
int analyze(universal_io *const io, syntax *const sx);

#ifdef __cplusplus
} /* extern "C" */
#endif
