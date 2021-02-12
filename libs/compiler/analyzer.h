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


#define TREE		(context->sx->tree)

#define REPRTAB		(context->sx->reprtab)
#define REPRTAB_POS (context->lexer->repr)
#define REPRTAB_LEN (context->sx->rp)


#ifdef __cplusplus
extern "C" {
#endif

/** Определение глобальных переменных */
typedef struct
{
	universal_io *io;			/**< Universal io structure */
	syntax *sx;					/**< Syntax structure */
	lexer *lexer;				/**< Lexer structure */

	token curr_token;			/**< Current token */
	token next_token;			/**< Lookahead token */

	size_t function_mode;		/**< Mode of currenty parsed function */
	size_t array_dimension;		/**< Array dimension counter */
	item_t gotost[1000];		/**< Labels table */
	size_t pgotost;				/**< Labels counter */

	int stack[100];
	int stackop[100];
	int stackoperands[100];
	int stacklog[100];
	int sp;
	int sopnd;
	int anst;
	int ansttype;
	int anstdispl;
	int leftansttype;

	int lastid;		// useless
	int op;			// useless
	int buf_flag;	// useless
	int buf_cur;	// useless

	/// @c 0 for function without arguments, @c 1 for function definition,
	/// @c 2 for function declaration, @c 3 for others
	int func_def;
	/** @c 0 for inition not by strings, @c 1 for inition by strings, @c 2 before parsing inition */
	int flag_strings_only;
	int flag_array_in_struct;	/**< Flag if parsed struct declaration has an array */
	int flag_empty_bounds;		/**< Flag if array declaration has empty bounds */
	int flag_was_return;		/**< Flag if was return in parsed function */
	int flag_in_switch;			/**< Flag if parser is in switch body */
	int flag_in_assignment;		/**< Flag if parser is in assignment */
	int flag_in_loop;			/**< Flag if parser is in loop body */
	int flag_was_type_def;		/**< Flag if was type definition */

	int was_error;				/**< Error flag */
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
