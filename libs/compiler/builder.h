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
#include "node_vector.h"
#include "operations.h"
#include "syntax.h"
#include "tree.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *	Build an identifier expression
 *
 *	@param	sx				Syntax structure
 *	@param	name			Index of record in representations table
 *	@param	loc				Source location
 *
 *	@return	Identifier expression node
 */
node build_identifier_expression(syntax *const sx, const size_t name, const location loc);

/**
 *	Build an integer literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Integer literal expression node
 */
node build_integer_literal_expression(syntax *const sx, const int value, const location loc);

/**
 *	Build a floating literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Floating literal expression node
 */
node build_floating_literal_expression(syntax *const sx, const double value, const location loc);

/**
 *	Build a string literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	index			Literal index in strings vector
 *	@param	loc				Source location
 *
 *	@return	String literal expression node
 */
node build_string_literal_expression(syntax *const sx, const size_t index, const location loc);

/**
 *	Build a subscript expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_fst			First operand of subscripting expression
 *	@param	nd_snd			Second operand of subscripting expression
 *	@param	l_loc			Left square bracket location
 *	@param	r_loc			Right square bracket location
 *
 *	@return	Subscript expression node
 */
node build_subscript_expression(syntax *const sx, const node *const nd_fst, const node *const nd_snd
	, const location l_loc, const location r_loc);

/**
 *	Build a call expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_func			Called function
 *	@param	args			Argument list
 *	@param	l_loc			Left paren location
 *	@param	r_loc			Right paren location
 *
 *	@return	Call expression node
 */
node build_call_expression(syntax *const sx, const node *const nd_func, const node_vector *args
	, const location l_loc, const location r_loc);

/**
 *	Build a member expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_base			First operand of member expression
 *	@param	name			Second operand of member expression
 *	@param	is_arrow		Set if operator is '->'
 *	@param	op_loc			Operator source location
 *	@param	id_loc			Identifier source location
 *
 *	@return	Member expression node
 */
node build_member_expression(syntax *const sx, const node *const nd_base, const size_t name, const bool is_arrow
	, const location op_loc, const location id_loc);

/**
 *	Build a upb expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_fst			First operand of upb expression
 *	@param	nd_snd			Second operand of upb expression
 *
 *	@return	Upb expression node
 */
node build_upb_expression(syntax *const sx, const node *const nd_fst, const node *const nd_snd);

/**
 *	Build an unary expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_operand		Operand of unary operator
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Unary expression node
 */
node build_unary_expression(syntax *const sx, const node *const nd_operand
	, const unary_t op_kind, const location op_loc);

/**
 *	Build a binary expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_left			Left operand
 *	@param	nd_right		Right operand
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Binary expression node
 */
node build_binary_expression(syntax *const sx, const node *const nd_left, const node *const nd_right
	, const binary_t op_kind, const location op_loc);

/**
 *	Build a ternary expression
 *
 *	@param	sx				Syntax structure
 *	@param	nd_left			First operand
 *	@param	nd_middle		Second operand
 *	@param	nd_right		Third operand
 *	@param	op_loc			Operator location
 *
 *	@return	Ternary expression node
 */
node build_ternary_expression(syntax *const sx, node *const nd_left, node *const nd_middle, node *const nd_right
	, const location op_loc);

/**
 *	Build an initializer list
 *
 *	@param	sx				Syntax structure
 *	@param	vec				Vector of initializer nodes
 *	@param	type			Target type for initializer list
 *	@param	l_loc			Left brace location
 *	@param	r_loc			Right brace location
 *
 *	@return	Initializer list expression node
 */
node build_init_list_expression(syntax *const sx, const node_vector *vec, const item_t type
	, const location l_loc, const location r_loc);

/**
 *	Build a broken expression
 *
 *	@return	Broken expression node
 */
node build_broken_expression();

#ifdef __cplusplus
} /* extern "C" */
#endif
