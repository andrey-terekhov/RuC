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
#include "errors.h"
#include "syntax.h"
#include "tree.h"
#include "operations.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Expression structure */
typedef struct expression
{
	bool is_valid;			/**< Set if is valid */
	location_t location;	/**< Source location */
	node nd;				/**< Node in AST */
} expression;

/** Expression list structure */
typedef struct expression_list
{
	unsigned length;
	expression expressions[128];
} expression_list;

/** Get expression type */
item_t expression_get_type(const expression expr);

/** Return invalid expression */
expression invalid_expression(void);

/**
 *	Build an identifier expression
 *
 *	@param	sx				Syntax structure
 *	@param	name			Identifier name
 *	@param	loc				Source location
 *
 *	@return	Identifier expression
 */
expression identifier_expression(syntax *const sx, const size_t name, const location_t loc);

/**
 *	Build an integer literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Integer literal expression
 */
expression integer_literal_expression(syntax *const sx, const int value, const location_t loc);

/**
 *	Build a floating literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Floating literal expression
 */
expression floating_literal_expression(syntax *const sx, const double value, const location_t loc);

/**
 *	Build a string literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	String literal expression
 */
expression string_literal_expression(syntax *const sx, const vector value, const location_t loc);

/**
 *	Build a subscript expression
 *
 *	@param	sx				Syntax structure
 *	@param	base			First operand of subscripting expression
 *	@param	index			Second operand of subscripting expression
 *	@param	l_loc			Left square bracket location
 *	@param	r_loc			Right square bracket location
 *
 *	@return	Subscript expression
 */
expression subscript_expression(syntax *const sx, const expression base, const expression index
								, const location_t l_loc, const location_t r_loc);

/**
 *	Build a call expression
 *
 *	@param	sx				Syntax structure
 *	@param	callee			Callee expression
 *	@param	args			Argument list
 *	@param	l_loc			Left paren location
 *	@param	r_loc			Right paren location
 *
 *	@return	Call expression
 */
expression call_expression(syntax *const sx, const expression callee, const expression_list *args
						   , const location_t l_loc, const location_t r_loc);

/**
 *	Build a member expression
 *
 *	@param	sx				Syntax structure
 *	@param	base			First operand of member expression
 *	@param	is_arrow		Set if operator is arrow
 *	@param	op_loc			Operator source location
 *	@param	name			Second operand of member expression
 *	@param	id_loc			Identifier source location
 *
 *	@return	Member expression
 */
expression member_expression(syntax *const sx, const expression base, const bool is_arrow, const size_t name
							, const location_t op_loc, const location_t id_loc);

/**
 *	Build a upb expression
 *
 *	@param	sx				Syntax structure
 *	@param	dimension		First operand of upb expression
 *	@param	array			Second operand of upb expression
 *
 *	@return	Upb expression
 */
expression upb_expression(syntax *const sx, const expression dimension, const expression array);

/**
 *	Build an unary expression
 *
 *	@param	sx				Syntax structure
 *	@param	operand			Operand of unary operator
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Unary expression
 */
expression unary_expression(syntax *const sx, const expression operand, const unary_t op_kind, const location_t op_loc);

/**
 *	Build a binary expression
 *
 *	@param	sx				Syntax structure
 *	@param	left			Left operand
 *	@param	right			Right operand
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Binary expression
 */
expression binary_expression(syntax *const sx, const expression left, const expression right
							 , const binary_t op_kind, const location_t op_loc);

/**
 *	Build a ternary expression
 *
 *	@param	sx				Syntax structure
 *	@param	left			First operand
 *	@param	middle			Second operand
 *	@param	right			Third operand
 *	@param	op_loc			Operator location
 *
 *	@return	Ternary expression
 */
expression ternary_expression(syntax *const sx, const expression left, const expression middle
							  , const expression right, const location_t op_loc);

expression init_list_expression(syntax *const sx, const expression_list *inits, const item_t type
								, const location_t l_loc, const location_t r_loc);
