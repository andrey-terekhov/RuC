/*
 *	Copyright 2023 Andrey Terekhov, Victor Y. Fadeev
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

#include "item.h"
#include "locator.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Operator Precedence */
typedef enum TOKEN
{
	TK_COMPL,			/**< '~' operator */
	TK_NOT,				/**< '!' operator */

	TK_MULT,			/**< '*' operator */
	TK_DIV,				/**< '/' operator */
	TK_MOD,				/**< '%' operator */

	TK_ADD,				/**< '+' operator */
	TK_SUB,				/**< '-' operator */

	TK_L_SHIFT,			/**< '<<' operator */
	TK_R_SHIFT,			/**< '>>' operator */

	TK_LESS,			/**< '<' operator */
	TK_GREATER,			/**< '>' operator */
	TK_LESS_EQ,			/**< '<=' operator */
	TK_GREATER_EQ,		/**< '>=' operator */

	TK_EQ,				/**< '==' operator */
	TK_NOT_EQ,			/**< '!=' operator */

	TK_BIT_AND,			/**< '&' operator */

	TK_XOR,				/**< '^' operator */

	TK_BIT_OR,			/**< '|' operator */

	TK_AND,				/**< '&&' operator */

	TK_OR,				/**< '||' operator */
} token_t;

/** Computer structure */
typedef struct computer
{
	location *loc;					/**< Default location */
	bool was_error;					/**< Set, if error message occurred */
} computer;


/**
 *	Create computer structure
 *
 *	@param	loc			Default location
 *
 *	@return	Computer structure
 */
computer computer_create(location *const loc);


/**
 *	Push the read token onto computer stack
 *
 *	@param	comp		Computer structure
 *	@param	loc			Parsed location
 *	@param	tk			Operator token
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int computer_push_token(computer *const comp, location *const loc, const token_t tk);

/**
 *	Push the read number onto computer stack
 *
 *	@param	comp		Computer structure
 *	@param	loc			Parsed location
 *	@param	num			Read number
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int computer_push_number(computer *const comp, location *const loc, const item_t num);


/**
 *	Check that computer is correct
 *
 *	@param	comp		Computer structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool computer_is_correct(const computer *const comp);


/**
 *	Clear computer structure
 *
 *	@param	comp		Computer structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int computer_clear(computer *const comp);

#ifdef __cplusplus
} /* extern "C" */
#endif
