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

#include <stdbool.h>
#include "errors.h"
#include "syntax.h"
#include "token.h"
#include "uniio.h"
#include "workspace.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Lexer structure */
typedef struct lexer
{
	syntax *sx;								/**< Syntax structure */

	char32_t character;						/**< Current character */
	vector lexstr;							/**< Representation of the read string literal */
} lexer;

/**
 *	Create lexer structure
 *
 *	@param	sx		Syntax structure
 *
 *	@return	Lexer
 */
lexer lexer_create(syntax *const sx);

/**
 *	Lex next token from io
 *
 *	@param	lxr		Lexer
 *
 *	@return	Lexed token
 */
token lex(lexer *const lxr);

/**
 *	Peek next token from io
 *
 *	@param	lxr		Lexer
 *
 *	@return	Peeked token kind
 */
token_t peek(lexer *const lxr);

/**
 *	Free allocated memory
 *
 *	@param	lxr		Lexer
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int lexer_clear(lexer *const lxr);

#ifdef __cplusplus
} /* extern "C" */
#endif
