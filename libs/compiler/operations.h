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

#include <stdbool.h>
#include <stddef.h>
#include "token.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Unary operator kinds */
typedef enum UNARY
{
	// Postfix increment and decrement
	UN_POSTINC,			/**< Postfix increment operator */
	UN_POSTDEC,			/**< Postfix decrement operator */
	// Prefix increment and decrement
	UN_PREINC,			/**< Pretfix increment operator */
	UN_PREDEC,			/**< Pretfix decrement operator */
	// Address and indirection
	UN_ADDRESS,			/**< '&' operator */
	UN_INDIRECTION,		/**< '*' operator */
	// Unary arithmetic
	UN_MINUS,			/**< '-' operator */
	UN_NOT,				/**< '~' operator */
	UN_LOGNOT,			/**< '!' operator */
	// Abs operator
	UN_ABS,				/**< 'abs' operator */
	// Upb operator
	UN_UPB,				/**< 'upb' operator */
} unary_t;

/** Binary operator kinds */
typedef enum BINARY
{
	// Multiplicative operators
	BIN_MUL,			/**< Multiplication */
	BIN_DIV,			/**< Division */
	BIN_REM,			/**< Remainder */
	// Additive operators
	BIN_ADD,			/**< Addition */
	BIN_SUB,			/**< Subtraction */
	// Bitwise shift operators
	BIN_SHL,			/**< Left shift */
	BIN_SHR,			/**< Right shift */
	// Relational operators
	BIN_LT,				/**< Less than */
	BIN_GT,				/**< Greater than */
	BIN_LE,				/**< Less than or equal to */
	BIN_GE,				/**< Greater than or equal to */
	// Equality operators
	BIN_EQ,				/**< Equal to */
	BIN_NE,				/**< Not equal to */
	// Bitwise AND operator
	BIN_AND,			/**< Bitwise AND */
	// Bitwise XOR operator
	BIN_XOR,			/**< Bitwise exclusive OR */
	// Bitwise OR operator
	BIN_OR,				/**< Bitwise inclusive OR */
	// Logical AND operator
	BIN_LOG_AND,		/**< Logical AND */
	// Logical OR operator
	BIN_LOG_OR,			/**< Logical OR */
	// Assignment operators
	BIN_ASSIGN,			/**< Simple assignment */
	BIN_MUL_ASSIGN,		/**< Multiplication assignment */
	BIN_DIV_ASSIGN,		/**< Division assignment */
	BIN_REM_ASSIGN,		/**< Remainder assignment */
	BIN_ADD_ASSIGN,		/**< Addition assignment */
	BIN_SUB_ASSIGN,		/**< Subtraction assignment */
	BIN_SHL_ASSIGN,		/**< Left shift assignment */
	BIN_SHR_ASSIGN,		/**< Right shift assignment */
	BIN_AND_ASSIGN,		/**< Bitwise AND assignment */
	BIN_XOR_ASSIGN,		/**< Bitwise exclusive OR assignment */
	BIN_OR_ASSIGN,		/**< Bitwise inclusive OR assignment */
	// Comma operator
	BIN_COMMA, 			/**< Comma */
} binary_t;

/** Binary/ternary operator precedence levels */
typedef enum PRECEDENCE
{
	PREC_UNKNOWN,			/**< Not binary operator */
	PREC_COMMA,				/**< Comma operator precedence */
	PREC_ASSIGNMENT,		/**< Assignment operator precedence */
	PREC_CONDITIONAL,		/**< Conditional operator precedence */
	PREC_LOGICAL_OR,		/**< Logical OR operator precedence */
	PREC_LOGICAL_AND,		/**< Logical AND operator precedence */
	PREC_OR,				/**< Bitwise OR operator precedence */
	PREC_XOR,				/**< Bitwise XOR operator precedence */
	PREC_AND,				/**< Bitwise AND operator precedence */
	PREC_EQUALITY,			/**< Equality operators precedence */
	PREC_RELATIONAL,		/**< Relational operators precedence */
	PREC_SHIFT,				/**< Shift operators precedence */
	PREC_ADDITIVE,			/**< Additive operators precedence */
	PREC_MULTIPLICATIVE,	/**< Multiplicative operators precedence */
} precedence_t;

/** AST node kinds */
typedef enum OPERATION
{
	OP_NOP,
	
	// Expressions
	OP_IDENTIFIER,			/**< Identifier node */
	OP_LITERAL,				/**< Literal node */
	OP_CALL,				/**< Call node */
	OP_SELECT,				/**< Select operator node */
	OP_SLICE,				/**< Slice operator node */
	OP_CAST,				/**< Cast operator node */
	OP_UNARY,				/**< Unary operator node */
	OP_BINARY,				/**< Binary operator node */
	OP_TERNARY,				/**< Ternary operator node */
	OP_ASSIGNMENT,			/**< Assignment operator node */
	OP_INITIALIZER,			/**< Initializer node */

	// Statements
	OP_CASE,				/**< Case statement node */
	OP_DEFAULT,				/**< Default statement node */
	OP_BLOCK,				/**< Compound statement node */
	OP_IF,					/**< If statement node */
	OP_SWITCH,				/**< Switch statement node */
	OP_WHILE,				/**< While statement node */
	OP_DO,					/**< Do statement node */
	OP_FOR,					/**< For statement node */
	OP_CONTINUE,			/**< Continue statement node */
	OP_BREAK,				/**< Break statement node */
	OP_RETURN,				/**< Return statement node */
	OP_DECLSTMT,			/**< Declaration statement */

	// Declarations
	OP_DECL_VAR,			/**< Variable declaration node */
	OP_DECL_TYPE,			/**< Type declaration node */
	OP_FUNC_DEF,			/**< Function definition node */

	// For internal use
	OP_EMPTY_BOUND,			/**< Empty array size expression */
} operation_t;

typedef enum builtin
{
	// Diagnostics functions
	BI_ASSERT				= 2,

	// Math functions
	BI_ASIN					= 6,
	BI_COS					= 10,
	BI_SIN					= 14,
	BI_EXP					= 18,
	BI_LOG					= 22,
	BI_LOG10				= 26,
	BI_SQRT					= 30,
	BI_RAND					= 34,
	BI_ROUND				= 38,

	// String functions
	BI_STRCPY				= 42,
	BI_STRNCPY				= 46,
	BI_STRCAT				= 50,
	BI_STRNCAT				= 54,
	BI_STRCMP				= 58,
	BI_STRNCMP				= 62,
	BI_STRSTR				= 66,
	BI_STRLEN				= 70,

	// Robot functions
	BI_ROBOT_SEND_INT		= 74,
	BI_ROBOT_SEND_FLOAT		= 78,
	BI_ROBOT_SEND_STRING	= 82,
	BI_ROBOT_RECEIVE_INT	= 86,
	BI_ROBOT_RECEIVE_FLOAT	= 90,
	BI_ROBOT_RECEIVE_STRING	= 94,

	// Thread functions
	BI_T_CREATE				= 98,
	BI_T_GETNUM				= 102,
	BI_T_SLEEP				= 106,
	BI_T_JOIN				= 110,
	BI_T_EXIT				= 114,
	BI_T_INIT				= 118,
	BI_T_DESTROY			= 122,

	BI_SEM_CREATE			= 126,
	BI_SEM_WAIT				= 130,
	BI_SEM_POST				= 134,

	BI_MSG_SEND				= 138,
	BI_MSG_RECEIVE			= 142,

	BI_FOPEN				= 146,
	BI_FGETC				= 150,
	BI_FPUTC				= 154,
	BI_FCLOSE				= 158,

	BI_EXIT					= 162,

	BI_PRINTF				= 166,
	BI_PRINT				= 170,
	BI_PRINTID				= 174,
	BI_GETID				= 178,

	BEGIN_USER_FUNC			= 182,
} builtin_t;


/**
 *	Convert token to corresponding unary operator
 *
 *	@param	token		Token
 *
 *	@return	Unary operator
 */
unary_t token_to_unary(const token_t token);

/**
 *	Convert token to corresponding binary operator
 *
 *	@param	token		Token
 *
 *	@return	Binary operator
 */
binary_t token_to_binary(const token_t token);

/**
 *	Get precedence of the specified binary/ternary operator token
 *
 *	@param	token		Token
 *
 *	@return	Precedence of operator token
 */
precedence_t get_operator_precedence(const token_t token);

/**
 *	Check if operator is assignment
 *
 *	@param	op			Operator
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool operation_is_assignment(const binary_t op);

#ifdef __cplusplus
} /* extern "C" */
#endif
