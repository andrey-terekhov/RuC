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

#include "tokens.h"
#include <stdbool.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/** Unary operator kinds */
typedef enum UNARY
{
	// [C99 6.5.2.4] Postfix increment and decrement
	UN_POSTINC,			/**< Postfix increment operator */
	UN_POSTDEC,			/**< Postfix decrement operator */
	// [C99 6.5.3.1] Prefix increment and decrement
	UN_PREINC,			/**< Pretfix increment operator */
	UN_PREDEC,			/**< Pretfix decrement operator */
	// [C99 6.5.3.2] Address and indirection
	UN_ADDRESS,			/**< "&" operator */
	UN_INDIRECTION,		/**< "*" operator */
	// [C99 6.5.3.3] Unary arithmetic
	UN_PLUS,			/**< "+" operator */
	UN_MINUS,			/**< "-" operator */
	UN_NOT,				/**< "~" operator */
	UN_LOGNOT,			/**< "!" operator */
} unary_t;

/** Binary operator kinds */
typedef enum BINARY
{
	// [C99 6.5.5] Multiplicative operators
	BIN_MUL,			/**< Multiplication */
	BIN_DIV,			/**< Division */
	BIN_REM,			/**< Remainder */
	// [C99 6.5.6] Additive operators
	BIN_ADD,			/**< Addition */
	BIN_SUB,			/**< Subtraction */
	// [C99 6.5.7] Bitwise shift operators
	BIN_SHL,			/**< Left shift */
	BIN_SHR,			/**< Right shift */
	// [C99 6.5.8] Relational operators
	BIN_LT,				/**< Less than */
	BIN_GT,				/**< Greater than */
	BIN_LE,				/**< Less than or equal to */
	BIN_GE,				/**< Greater than or equal to */
	// [C99 6.5.9] Equality operators
	BIN_EQ,				/**< Equal to */
	BIN_NE,				/**< Not equal to */
	// [C99 6.5.10] Bitwise AND operator
	BIN_AND,			/**< Bitwise AND */
	// [C99 6.5.11] Bitwise XOR operator
	BIN_XOR,			/**< Bitwise exclusive OR */
	// [C99 6.5.12] Bitwise OR operator
	BIN_OR,				/**< Bitwise inclusive OR */
	// [C99 6.5.13] Logical AND operator
	BIN_LOG_AND,		/**< Logical AND */
	// [C99 6.5.14] Logical OR operator
	BIN_LOG_OR,			/**< Logical OR */
	// [C99 6.5.16] Assignment operators
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
	// [C99 6.5.17] Comma operator
	BIN_COMMA, 			/**< Comma */
} binary_t;

/** AST node kinds */
typedef enum OPERATION
{
	OP_NOP,
	// Statement
	OP_LABEL,				/**< Label statement node */
	OP_CASE,				/**< Case statement node */
	OP_DEFAULT,				/**< Default statement node */
	OP_BLOCK,				/**< Compound statement node */
	OP_IF,					/**< If statement node */
	OP_SWITCH,				/**< Switch statement node */
	OP_WHILE,				/**< While statement node */
	OP_DO,					/**< Do statement node */
	OP_FOR,					/**< For statement node */
	OP_GOTO,				/**< Goto statement node */
	OP_CONTINUE,			/**< Continue statement node */
	OP_BREAK,				/**< Break statement node */
	OP_RETURN,				/**< Return statement node */
	OP_THREAD,				/**< Create direct thread node */

	// Declarations
	OP_DECL_ID,				/**< Identifier declaration node */
	OP_DECL_ARR,			/**< Array declaration node */
	OP_DECL_STRUCT,			/**< Struct declaration node */
	OP_FUNC_DEF,			/**< Function definition node */
	OP_ARRAY_INIT,			/**< Array inition node */
	OP_STRUCT_INIT,			/**< Struct inition node */

	// End nodes
	OP_DECL_STRUCT_END,		/**< End of struct declaration node */

	// Expressions
	OP_IDENTIFIER,			/**< Identifier node */
	OP_CONSTANT,			/**< Constant node */
	OP_STRING,				/**< String litaral node */
	OP_CALL,				/**< Call node */
	OP_SELECT,				/**< Select operator node */
	OP_SLICE,				/**< Slice operator node */
	OP_UNARY,				/**< Unary operator node */
	OP_BINARY,				/**< Binary operator node */
	OP_TERNARY,				/**< Ternary operator node */

	// Built-in functions
	OP_PRINTID,
	OP_PRINT,
	OP_GETID,
	OP_PRINTF,
} operation_t;

unary_t token_to_unary(const token_t token);
binary_t token_to_binary(const token_t token);
bool operation_is_assignment(const binary_t operator);

#ifdef __cplusplus
} /* extern "C" */
#endif
