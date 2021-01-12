/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Ilya Andreev
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
#include "tokens.h"
#include "syntax.h"
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct lexer
{
	universal_io *io;					/**< Universal io structure */
	syntax *sx;							/**< Syntax structure */
	
	char32_t curr_char;					/**< Current character */
	char32_t next_char;					/**< Lookahead character */
	int repr;							/**< Pointer to representation of the read identifier */
	int num;							/**< Value of the read integer number */
	double num_double;					/**< Value of the read double number */
	char32_t lexstr[MAXSTRINGL + 1];	/**< Representation of the read string literal */
	
	int error_flag;						/**< Error flag */
} lexer;

/**
 *	Create lexer structure
 *
 *	@param	io		Universal io structure
 *	@param	sx		Syntax structure
 *
 *	@return	Lexer structure
 */
lexer create_lexer(universal_io *const io, syntax *const sx);

/**
 *	Read next character from io
 *
 *	@param	lxr		Lexer structure
 *
 *	@return	Character
 */
char32_t get_char(lexer *const lxr);

/**
 *	Lex next token from io
 *
 *	@param	lxr		Lexer structure
 *
 *	@return	Token
 */
token lex(lexer *const lxr);

#ifdef __cplusplus
} /* extern "C" */
#endif
