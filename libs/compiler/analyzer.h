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


//#define GENERATE_TREE

#define TREE		(prs->sx->tree)

#define REPRTAB		(prs->sx->reprtab)
#define REPRTAB_POS (prs->lxr->repr)
#define REPRTAB_LEN (prs->sx->rp)


#ifdef __cplusplus
extern "C" {
#endif

/** Определение глобальных переменных */
typedef struct parser
{
	syntax *sx;					/**< Syntax structure */
	lexer *lxr;					/**< Lexer structure */

	token_t curr_token;			/**< Current token */
	token_t next_token;			/**< Lookahead token */

	size_t function_mode;		/**< Mode of currenty parsed function */
	size_t array_dimensions;	/**< Array dimensions counter */
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

	size_t lastid;	// useless
	int op;			// useless
	int buf_flag;	// useless
	int buf_cur;	// useless

	int func_def;				/**< @c 0 for function without arguments,
									 @c 1 for function definition,
									 @c 2 for function declaration,
									 @c 3 for others */

	int flag_strings_only;		/**< @c 0 for non-string initialization,
									 @c 1 for string initialization,
									 @c 2 for parsing before initialization */

	int flag_array_in_struct;	/**< Set, if parsed struct declaration has an array */
	int flag_empty_bounds;		/**< Set, if array declaration has empty bounds */
	int flag_was_return;		/**< Set, if was return in parsed function */
	int flag_in_switch;			/**< Set, if parser is in switch body */
	int flag_in_assignment;		/**< Set, if parser is in assignment */
	int flag_in_loop;			/**< Set, if parser is in loop body */
	int flag_was_type_def;		/**< Set, if was type definition */

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
