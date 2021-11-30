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
	TK_INT,							/**< 'int' keyword */
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

	// Literals
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
inline location token_get_location(const token *const tk)
{
	return tk->loc;
}

/**
 *	Get token kind
 *
 *	@param	tk			Token
 *
 *	@return	Token kind
 */
inline token_t token_get_kind(const token *const tk)
{
	return tk->kind;
}

/**	Check if token is of @c kind kind */
inline bool token_is(const token *const tk, const token_t kind)
{
	return tk->kind == kind;
}

/**	Check if token is not of @c kind kind */
inline bool token_is_not(const token *const tk, const token_t kind)
{
	return tk->kind != kind;
}


/**
 *	Create identifier token
 *
 *	@param	loc			Token location
 *	@param	name		Identifier name
 *
 *	@return	Identifier token
 */
inline token token_identifier(const location loc, const size_t name)
{
	return (token){ .loc = loc, .kind = TK_IDENTIFIER, .data.ident_repr = name };
}

/**
 *	Get identifier name from identifier token
 *
 *	@param	tk			Identifier token
 *
 *	@return	Identifier name
 */
inline size_t token_get_ident_name(const token *const tk)
{
	assert(tk->kind == TK_IDENTIFIER);
	return tk->data.ident_repr;
}


/**
 *	Create character literal token
 *
 *	@param	loc			Token location
 *	@param	value		Literal value
 *
 *	@return	Character literal token
 */
inline token token_char_literal(const location loc, const char32_t value)
{
	return (token){ .loc = loc, .kind = TK_CHAR_LITERAL, .data.char_value = value };
}

/**
 *	Get value from character literal token
 *
 *	@param	tk			Character literal token
 *
 *	@return	Value
 */
inline char32_t token_get_char_value(const token *const tk)
{
	assert(tk->kind == TK_CHAR_LITERAL);
	return tk->data.char_value;
}


/**
 *	Create integer literal token
 *
 *	@param	loc			Token location
 *	@param	value		Literal value
 *
 *	@return	Integer literal token
 */
inline token token_int_literal(const location loc, const uint64_t value)
{
	return (token){ .loc = loc, .kind = TK_INT_LITERAL, .data.int_value = value };
}

/**
 *	Get value from integer literal token
 *
 *	@param	tk			Integer literal token
 *
 *	@return	Value
 */
inline uint64_t token_get_int_value(const token *const tk)
{
	assert(tk->kind == TK_INT_LITERAL);
	return tk->data.int_value;
}


/**
 *	Create floating literal token
 *
 *	@param	loc			Token location
 *	@param	value		Literal value
 *
 *	@return	Floating literal token
 */
inline token token_float_literal(const location loc, const double value)
{
	return (token){ .loc = loc, .kind = TK_FLOAT_LITERAL, .data.float_value = value };
}

/**
 *	Get value from floating literal token
 *
 *	@param	tk			Floating literal token
 *
 *	@return	Value
 */
inline double token_get_float_value(const token *const tk)
{
	assert(tk->kind == TK_FLOAT_LITERAL);
	return tk->data.float_value;
}


/**
 *	Create string literal token
 *
 *	@param	loc			Token location
 *	@param	string_num	String index from string table
 *
 *	@return	String literal token
 */
inline token token_string_literal(const location loc, const size_t string_num)
{
	return (token){ .loc = loc, .kind = TK_STRING_LITERAL, .data.string_num = string_num };
}

/**
 *	Get string index from string literal token
 *
 *	@param	tk			String literal token
 *
 *	@return	Value
 */
inline size_t token_get_string_num(const token *const tk)
{
	assert(tk->kind == TK_STRING_LITERAL);
	return tk->data.string_num;
}


/**
 *	Create end-of-file token
 *
 *	@return	EOF token
 */
inline token token_eof()
{
	return (token){ .loc = { SIZE_MAX, SIZE_MAX }, .kind = TK_EOF };
}

/**
 *	Create keyword token
 *
 *	@param	loc			Token location
 *	@param	kind		Keyword kind
 *
 *	@return	Keyword token
 */
inline token token_keyword(const location loc, const token_t kind)
{
	return (token){ .loc = loc, .kind = kind };
}

/**
 *	Create punctuator token
 *
 *	@param	loc			Token location
 *	@param	kind		Punctuator kind
 *
 *	@return	Punctuator token
 */
inline token token_punctuator(const location loc, const token_t kind)
{
	return (token){ .loc = loc, .kind = kind };
}

#ifdef __cplusplus
} /* extern "C" */
#endif
