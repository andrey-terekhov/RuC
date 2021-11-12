/*
 *	Copyright 2021 Andrey Terekhov, Ilya Andreev
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

#include <assert.h>
#include <limits.h>
#include "utf8.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum TOKEN
{
	TK_MAIN = 0,					/**< The main function token */
	TK_EOF = SHRT_MIN,				/**< End of file token */

	// Keywords
	TK_ABS,							/**< 'abs' keyword */
	TK_BREAK,						/**< 'break' keyword */
	TK_CASE,						/**< 'case' keyword */
	TK_CHAR,						/**< 'char' keyword */
	TK_CONTINUE,					/**< 'continue' keyword */
	TK_DEFAULT,						/**< 'default' keyword */
	TK_DO,							/**< 'do' keyword */
	TK_DOUBLE,						/**< 'double' keyword */
	TK_ELSE,						/**< 'else' keyword */
	TK_ENUM,						/**< 'enum' keyword */
	TK_FILE,						/**< 'file' keyword */
	TK_FLOAT,						/**< 'float' keyword */
	TK_FOR,							/**< 'for' keyword */
	TK_GOTO,						/**< 'goto' keyword */
	TK_IF,							/**< 'if' keyword */
	TK_INT,							/**< 'int' keyword	*/
	TK_LONG,						/**< 'long' keyword */
	TK_NULL,						/**< 'null' keyword */
	TK_RETURN,						/**< 'return' keyword */
	TK_STRUCT,						/**< 'struct' keyword */
	TK_SWITCH,						/**< 'switch' keyword */
	TK_TYPEDEF,						/**< 'typedef' keyword */
	TK_VOID,						/**< 'void' keyword */
	TK_WHILE,						/**< 'while' keyword */

	// Identifiers
	TK_IDENTIFIER,					/**< Identifier */

	// Constants
	TK_INT_LITERAL,					/**< Integer literal */
	TK_FLOAT_LITERAL,				/**< Floating literal */
	TK_CHAR_LITERAL,				/**< Character literal */
	TK_STRING_LITERAL,				/**< String literal */

	// Punctuators
	TK_L_SQUARE,					/**< '[' punctuator */
	TK_L_PAREN,						/**< '(' punctuator */
	TK_L_BRACE,						/**< '{' punctuator */
	TK_R_SQUARE		= 0b00000001,	/**< ']' punctuator */
	TK_R_PAREN		= 0b00000010,	/**< ')' punctuator */
	TK_R_BRACE		= 0b00000100,	/**< '}' punctuator */
	TK_COMMA		= 0b00001000,	/**< ',' punctuator */
	TK_COLON		= 0b00010000,	/**< ':' punctuator */
	TK_SEMICOLON	= 0b00100000,	/**< ';' punctuator */
	TK_QUESTION 	= TK_L_BRACE+1,	/**< '?' punctuator */
	TK_TILDE,						/**< '~' punctuator */
	TK_PERIOD,						/**< '.' punctuator */
	TK_PLUS,						/**< '+' punctuator */
	TK_MINUS,						/**< '-' punctuator */
	TK_STAR,						/**< '*' punctuator */
	TK_SLASH,						/**< '/' punctuator */
	TK_PERCENT,						/**< '%' punctuator */
	TK_EXCLAIM,						/**< '!' punctuator */
	TK_CARET,						/**< '^' punctuator */
	TK_PIPE,						/**< '|' punctuator */
	TK_AMP,							/**< '&' punctuator */
	TK_EQUAL,						/**< '=' punctuator */
	TK_LESS,						/**< '<' punctuator */
	TK_GREATER,						/**< '>' punctuator */
	TK_ARROW,						/**< '->' punctuator */
	TK_PLUS_EQUAL,					/**< '+=' punctuator */
	TK_PLUS_PLUS,					/**< '++' punctuator */
	TK_MINUS_EQUAL,					/**< '-=' punctuator */
	TK_MINUS_MINUS,					/**< '--' punctuator */
	TK_STAR_EQUAL,					/**< '*=' punctuator */
	TK_SLASH_EQUAL,					/**< '/=' punctuator */
	TK_PERCENT_EQUAL,				/**< '%=' punctuator */
	TK_EXCLAIM_EQUAL,				/**< '!=' punctuator */
	TK_CARET_EQUAL,					/**< '^=' punctuator */
	TK_PIPE_EQUAL,					/**< '|=' punctuator */
	TK_PIPE_PIPE,					/**< '||' punctuator */
	TK_AMP_EQUAL,					/**< '&=' punctuator */
	TK_AMP_AMP,						/**< '&&' punctuator */
	TK_EQUAL_EQUAL,					/**< '==' punctuator */
	TK_LESS_EQUAL,					/**< '<=' punctuator */
	TK_LESS_LESS,					/**< '<<' punctuator */
	TK_GREATER_EQUAL,				/**< '>=' punctuator */
	TK_GREATER_GREATER,				/**< '>>' punctuator */
	TK_LESS_LESS_EQUAL,				/**< '<<=' punctuator */
	TK_GREATER_GREATER_EQUAL,		/**< '>>=' punctuator */
} token_t;

/** Source location */
typedef struct location
{
	size_t begin;
	size_t end;
} location;

/** Token */
typedef struct token
{
	location loc;					/**< Source location */
	token_t kind;					/**< Token kind */
	union
	{
		size_t ident_repr;			/**< Index of representation in representation table */
		char32_t char_value;		/**< Value of character literal */
		int64_t int_value;			/**< Value of integer literal */
		double float_value;			/**< Value of floating literal */
		size_t string_num;			/**< Index of string literal in strings vector */
	} data;
} token;


inline location token_get_location(const token *const tk)
{
	return tk->loc;
}

inline token_t token_get_kind(const token *const tk)
{
	return tk->kind;
}

inline size_t token_get_ident_name(const token *const tk)
{
	assert(tk->kind == TK_IDENTIFIER);
	return tk->data.ident_repr;
}

inline char32_t token_get_char_value(const token *const tk)
{
	assert(tk->kind == TK_CHAR_LITERAL);
	return tk->data.char_value;
}

inline int64_t token_get_int_value(const token *const tk)
{
	assert(tk->kind == TK_INT_LITERAL);
	return tk->data.int_value;
}

inline double token_get_float_value(const token *const tk)
{
	assert(tk->kind == TK_FLOAT_LITERAL);
	return tk->data.float_value;
}

inline size_t token_get_string_num(const token *const tk)
{
	assert(tk->kind == TK_STRING_LITERAL);
	return tk->data.string_num;
}

inline bool token_is(const token *const tk, const token_t kind)
{
	return tk->kind == kind;
}

inline bool token_is_not(const token *const tk, const token_t kind)
{
	return tk->kind != kind;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
