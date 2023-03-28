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
#include <stdint.h>
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
	TK_BOOL,						/**< 'bool' keyword */
	TK_BREAK,						/**< 'break' keyword */
	TK_CASE,						/**< 'case' keyword */
	TK_CHAR,						/**< 'char' keyword */
	TK_CONST, 						/**< 'const' keyword */
	TK_CONTINUE,					/**< 'continue' keyword */
	TK_DEFAULT,						/**< 'default' keyword */
	TK_DO,							/**< 'do' keyword */
	TK_DOUBLE,						/**< 'double' keyword */
	TK_ELSE,						/**< 'else' keyword */
	TK_ENUM,						/**< 'enum' keyword */
	TK_FALSE,						/**< 'false' keyword */
	TK_FILE,						/**< 'file' keyword */
	TK_FLOAT,						/**< 'float' keyword */
	TK_FOR,							/**< 'for' keyword */
	TK_IF,							/**< 'if' keyword */
	TK_INT,							/**< 'int' keyword */
	TK_LONG,						/**< 'long' keyword */
	TK_NULL,						/**< 'null' keyword */
	TK_RETURN,						/**< 'return' keyword */
	TK_STRUCT,						/**< 'struct' keyword */
	TK_SWITCH,						/**< 'switch' keyword */
	TK_TRUE,						/**< 'true' keyword */
	TK_TYPEDEF,						/**< 'typedef' keyword */
	TK_UPB,							/**< 'upb' keyword */
	TK_VOID,						/**< 'void' keyword */
	TK_WHILE,						/**< 'while' keyword */

	// Identifiers
	TK_IDENTIFIER,					/**< Identifier */

	// Literals
	TK_INT_LITERAL,					/**< Integer literal */
	TK_FLOAT_LITERAL,				/**< Floating literal */
	TK_CHAR_LITERAL,				/**< Character literal */
	TK_STRING_LITERAL,				/**< String literal */

	// Punctuators
	TK_L_SQUARE,					/**< '[' punctuator */
	TK_L_PAREN,						/**< '(' punctuator */
	TK_L_BRACE,						/**< '{' punctuator */
	TK_R_SQUARE		= 0x01,			/**< ']' punctuator */
	TK_R_PAREN		= 0x02,			/**< ')' punctuator */
	TK_R_BRACE		= 0x04,			/**< '}' punctuator */
	TK_COMMA		= 0x08,			/**< ',' punctuator */
	TK_COLON		= 0x10,			/**< ':' punctuator */
	TK_SEMICOLON	= 0x20,			/**< ';' punctuator */
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

/** Token representation */
typedef struct token
{
	location loc;					/**< Source location */
	token_t kind;					/**< Token kind */
	union
	{
		size_t ident_repr;			/**< Index of representation in representation table */
		char32_t char_value;		/**< Value of character literal */
		uint64_t int_value;			/**< Value of integer literal */
		double float_value;			/**< Value of floating literal */
		size_t string_num;			/**< Index of string literal in strings vector */
	} data;
} token;


/**
 *	Get token location
 *
 *	@param	tk			Token
 *
 *	@return	Token location
 */
location token_get_location(const token *const tk);

/**
 *	Get token kind
 *
 *	@param	tk			Token
 *
 *	@return	Token kind
 */
token_t token_get_kind(const token *const tk);

/**	Check if token is of @c kind kind */
bool token_is(const token *const tk, const token_t kind);

/**	Check if token is not of @c kind kind */
bool token_is_not(const token *const tk, const token_t kind);


/**
 *	Create identifier token
 *
 *	@param	loc			Token location
 *	@param	name		Identifier name
 *
 *	@return	Identifier token
 */
token token_identifier(const location loc, const size_t name);

/**
 *	Get identifier name from identifier token
 *
 *	@param	tk			Identifier token
 *
 *	@return	Identifier name
 */
size_t token_get_ident_name(const token *const tk);


/**
 *	Create character literal token
 *
 *	@param	loc			Token location
 *	@param	value		Literal value
 *
 *	@return	Character literal token
 */
token token_char_literal(const location loc, const char32_t value);

/**
 *	Get value from character literal token
 *
 *	@param	tk			Character literal token
 *
 *	@return	Value
 */
char32_t token_get_char_value(const token *const tk);


/**
 *	Create integer literal token
 *
 *	@param	loc			Token location
 *	@param	value		Literal value
 *
 *	@return	Integer literal token
 */
token token_int_literal(const location loc, const uint64_t value);

/**
 *	Get value from integer literal token
 *
 *	@param	tk			Integer literal token
 *
 *	@return	Value
 */
uint64_t token_get_int_value(const token *const tk);


/**
 *	Create floating literal token
 *
 *	@param	loc			Token location
 *	@param	value		Literal value
 *
 *	@return	Floating literal token
 */
token token_float_literal(const location loc, const double value);

/**
 *	Get value from floating literal token
 *
 *	@param	tk			Floating literal token
 *
 *	@return	Value
 */
double token_get_float_value(const token *const tk);


/**
 *	Create string literal token
 *
 *	@param	loc			Token location
 *	@param	string_num	String index from string table
 *
 *	@return	String literal token
 */
token token_string_literal(const location loc, const size_t string_num);

/**
 *	Get string index from string literal token
 *
 *	@param	tk			String literal token
 *
 *	@return	Value
 */
size_t token_get_string_num(const token *const tk);


/**
 *	Create end-of-file token
 *
 *	@return	EOF token
 */
token token_eof(void);

/**
 *	Create keyword token
 *
 *	@param	loc			Token location
 *	@param	kind		Keyword kind
 *
 *	@return	Keyword token
 */
token token_keyword(const location loc, const token_t kind);

/**
 *	Create punctuator token
 *
 *	@param	loc			Token location
 *	@param	kind		Punctuator kind
 *
 *	@return	Punctuator token
 */
token token_punctuator(const location loc, const token_t kind);

#ifdef __cplusplus
} /* extern "C" */
#endif
