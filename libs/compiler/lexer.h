/*
 *	Copyright 2019 Andrey Terekhov, Victor Y. Fadeev, Ilya Andreev
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

#ifndef RUC_LEXER_H
#define RUC_LEXER_H

#include "defs.h"
#include "tokens.h"
#include "syntax.h"
#include "uniio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Lexer
{
	universal_io *io;					/**< Universal io structure */
	syntax *sx;							/**< Syntax structure */
	
	char32_t curchar;					/**< Current character */
	char32_t nextchar;					/**< Lookahead character */
	int num;							/**< Value of the read integer number */
	struct { int fst; int snd; } numr;	/**< Value of the read double number */
	char32_t lexstr[MAXSTRINGL + 1];	/**< Representation of the read string literal */
	
	int keywordsnum;
	int error_flag;						/**< Error flag */
} Lexer;

/**
 *	Create lexer structure
 *
 *	@param	io		Universal io structure
 *
 *	@return	Lexer structure
 */
Lexer lexer_create(universal_io *const io, syntax *const sx);

/**
 *	Read next character from io
 *
 *	@param	lexer	Lexer structure
 *
 *	@return	Character
 */
char32_t get_char(Lexer *const lexer);

/**
 *	Lex next token from io
 *
 *	@param	lexer	Lexer structure
 *
 *	@return	Token
 */
int lex(Lexer *const lexer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RUC_LEXER_H */
