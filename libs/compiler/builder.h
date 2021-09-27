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
 *	Check assignment operands
 *	@note	TODO: Remove
 *
 *	@param	sx				Syntax structure
 *	@param	expected_type	Expected type
 *	@param	init			Initializer
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool check_assignment_operands(syntax *const sx, const item_t expected_type, node *const init);

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
node build_integer_literal_expression(syntax *const sx, const item_t value, const location loc);

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
 *	Build a null pointer literal expression
 *
 *	@param	sx				Syntax structure
 *	@param	loc				Source location
 *
 *	@return Null pointer literal expression node
 */
node build_null_pointer_literal_expression(syntax *const sx, const location loc);

/**
 *	Build a subscript expression
 *
 *	@param	sx				Syntax structure
 *	@param	base			First operand of subscripting expression
 *	@param	index			Second operand of subscripting expression
 *	@param	l_loc			Left square bracket location
 *	@param	r_loc			Right square bracket location
 *
 *	@return	Subscript expression node
 */
node build_subscript_expression(syntax *const sx, const node *const base, const node *const index
	, const location l_loc, const location r_loc);

/**
 *	Build a call expression
 *
 *	@param	sx				Syntax structure
 *	@param	callee			Called expression
 *	@param	args			Argument list
 *	@param	l_loc			Left paren location
 *	@param	r_loc			Right paren location
 *
 *	@return	Call expression node
 */
node build_call_expression(syntax *const sx, const node *const callee
	, node_vector *const args, const location l_loc, const location r_loc);

/**
 *	Build a member expression
 *
 *	@param	sx				Syntax structure
 *	@param	base			First operand of member expression
 *	@param	name			Second operand of member expression
 *	@param	is_arrow		Set if operator is '->'
 *	@param	op_loc			Operator source location
 *	@param	id_loc			Identifier source location
 *
 *	@return	Member expression node
 */
node build_member_expression(syntax *const sx, const node *const base, const size_t name
	, const bool is_arrow, const location op_loc, const location id_loc);

/**
 *	Build a cast expression
 *
 *	@param	target_type		Value type
 *	@param	expr			Operand
 *
 *	@return	Cast expression node
 */
node build_cast_expression(const item_t target_type, const node *const expr);

/**
 *	Build an unary expression
 *
 *	@param	sx				Syntax structure
 *	@param	operand			Operand of unary operator
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Unary expression node
 */
node build_unary_expression(syntax *const sx, node *const operand, const unary_t op_kind, const location op_loc);

/**
 *	Build a binary expression
 *
 *	@param	sx				Syntax structure
 *	@param	LHS				Left operand
 *	@param	RHS				Right operand
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Binary expression node
 */
node build_binary_expression(syntax *const sx, node *const LHS, node *const RHS
	, const binary_t op_kind, const location op_loc);

/**
 *	Build a ternary expression
 *
 *	@param	sx				Syntax structure
 *	@param	cond			First operand
 *	@param	LHS				Second operand
 *	@param	RHS				Third operand
 *	@param	op_loc			Operator location
 *
 *	@return	Ternary expression node
 */
node build_ternary_expression(syntax *const sx, node *const cond, node *const LHS, node *const RHS, const location op_loc);

/**
 *	Build an initializer list
 *
 *	@param	sx				Syntax structure
 *	@param	exprs			Vector of expressions
 *	@param	l_loc			Left brace location
 *	@param	r_loc			Right brace location
 *
 *	@return	Initializer list expression
 */
node build_init_list_expression(syntax *const sx, node_vector *const exprs, const location l_loc, const location r_loc);

#ifdef __cplusplus
} /* extern "C" */
#endif
