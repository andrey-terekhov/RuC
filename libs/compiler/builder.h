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
#include "operations.h"
#include "syntax.h"
#include "tree.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Expression structure */
typedef struct expression
{
	bool is_valid;			/**< Set if is valid */
	location location;		/**< Source location */
	node nd;				/**< Node in AST */
} expression;

/** Expression list structure */
typedef struct expression_list
{
	unsigned length;
	expression expressions[128];
} expression_list;

/**
 *	Get expression type
 *
 *	@param	expr	Expression
 *
 *	@return	Expression type
 */
item_t expression_get_type(const expression expr);

/**
 *	Return invalid expression
 *
 *	@return	Invalid expression
 */
expression build_invalid_expression(void);

/**
 *	Build an identifier expression
 *
 *	@param	sx				Syntax structure
 *	@param	name			Identifier name
 *	@param	loc				Source location
 *
 *	@return	Identifier expression
 */
expression build_identifier_expression(syntax *const sx, const size_t name, const location loc);

/**
 *	Build an integer literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Integer literal expression
 */
expression build_integer_literal_expression(syntax *const sx, const int value, const location loc);

/**
 *	Build a floating literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Floating literal expression
 */
expression build_floating_literal_expression(syntax *const sx, const double value, const location loc);

/**
 *	Build a string literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	String literal expression
 */
expression build_string_literal_expression(syntax *const sx, const vector value, const location loc);

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
expression build_subscript_expression(syntax *const sx, const expression base, const expression index
									  , const location l_loc, const location r_loc);

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
expression build_call_expression(syntax *const sx, const expression callee, const expression_list *args
								, const location l_loc, const location r_loc);

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
expression build_member_expression(syntax *const sx, const expression base, const bool is_arrow, const size_t name
								   , const location op_loc, const location id_loc);

/**
 *	Build a upb expression
 *
 *	@param	sx				Syntax structure
 *	@param	dimension		First operand of upb expression
 *	@param	array			Second operand of upb expression
 *
 *	@return	Upb expression
 */
expression build_upb_expression(syntax *const sx, const expression dimension, const expression array);

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
expression build_unary_expression(syntax *const sx, const expression operand
								  , const unary_t op_kind, const location op_loc);

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
expression build_binary_expression(syntax *const sx, const expression left, const expression right
								   , const binary_t op_kind, const location op_loc);

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
expression build_ternary_expression(syntax *const sx, const expression left, const expression middle
									, const expression right, const location op_loc);

/**
 *	Build an initializer list
 *
 *	@param	sx				Syntax structure
 *	@param	inits			List of initializers
 *	@param	type			Target type for initializer list
 *	@param	l_loc			Left brace location
 *	@param	r_loc			Right brace location
 *
 *	@return	Initializer list expression
 */
expression build_init_list_expression(syntax *const sx, const expression_list *inits, const item_t type
									  , const location l_loc, const location r_loc);
