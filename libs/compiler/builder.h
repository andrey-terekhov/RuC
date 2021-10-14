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

/** AST builder */
typedef struct builder
{
	syntax *sx;				/**< Syntax structure */

	node context;			/**< Context for creating new nodes */
	vector labels;			/**< Labels table */

	item_t func_type;		/**< Type of current parsed function */
} builder;

/**
 *	Create AST builder
 *
 *	@param	sx				Syntax structure
 *
 *	@return	AST builder
 */
builder bld_create(syntax *const sx);

/**
 *	Free allocated memory
 *
 *	@param	bld				AST builder
 */
void bld_clear(builder *const bld);

/**
 *	Check assignment operands
 *	@note	TODO: Remove
 *
 *	@param	bld				AST builder
 *	@param	expected_type	Expected type
 *	@param	init			Initializer
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool check_assignment_operands(builder *const bld, const item_t expected_type, node *const init);

/**
 *	Build an identifier expression
 *
 *	@param	bld				AST builder
 *	@param	name			Index of record in representations table
 *	@param	loc				Source location
 *
 *	@return	Identifier expression node
 */
node build_identifier_expression(builder *const bld, const size_t name, const location loc);

/**
 *	Build an integer literal expression
 *
 *	@param	bld				AST builder
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Integer literal expression node
 */
node build_integer_literal_expression(builder *const bld, const item_t value, const location loc);

/**
 *	Build a floating literal expression
 *
 *	@param	bld				AST builder
 *	@param	value			Literal value
 *	@param	loc				Source location
 *
 *	@return	Floating literal expression node
 */
node build_floating_literal_expression(builder *const bld, const double value, const location loc);

/**
 *	Build a string literal expression
 *
 *	@param	bld				AST builder
 *	@param	index			Literal index in strings vector
 *	@param	loc				Source location
 *
 *	@return	String literal expression node
 */
node build_string_literal_expression(builder *const bld, const size_t index, const location loc);

/**
 *	Build a null pointer literal expression
 *
 *	@param	bld				AST builder
 *	@param	loc				Source location
 *
 *	@return Null pointer literal expression node
 */
node build_null_pointer_literal_expression(builder *const bld, const location loc);

/**
 *	Build a subscript expression
 *
 *	@param	bld				AST builder
 *	@param	base			First operand of subscripting expression
 *	@param	index			Second operand of subscripting expression
 *	@param	l_loc			Left square bracket location
 *	@param	r_loc			Right square bracket location
 *
 *	@return	Subscript expression node
 */
node build_subscript_expression(builder *const bld, node *const base, node *const index
	, const location l_loc, const location r_loc);

/**
 *	Build a call expression
 *
 *	@param	bld				AST builder
 *	@param	callee			Called expression
 *	@param	args			Argument list
 *	@param	l_loc			Left paren location
 *	@param	r_loc			Right paren location
 *
 *	@return	Call expression node
 */
node build_call_expression(builder *const bld, node *const callee
	, node_vector *const args, const location l_loc, const location r_loc);

/**
 *	Build a member expression
 *
 *	@param	bld				AST builder
 *	@param	base			First operand of member expression
 *	@param	name			Second operand of member expression
 *	@param	is_arrow		Set if operator is '->'
 *	@param	op_loc			Operator source location
 *	@param	id_loc			Identifier source location
 *
 *	@return	Member expression node
 */
node build_member_expression(builder *const bld, node *const base, const size_t name
	, const bool is_arrow, const location op_loc, const location id_loc);

/**
 *	Build a cast expression
 *
 *	@param	target_type		Value type
 *	@param	expr			Operand
 *
 *	@return	Cast expression node
 */
node build_cast_expression(const item_t target_type, node *const expr);

/**
 *	Build an unary expression
 *
 *	@param	bld				AST builder
 *	@param	operand			Operand of unary operator
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Unary expression node
 */
node build_unary_expression(builder *const bld, node *const operand, const unary_t op_kind, const location op_loc);

/**
 *	Build a binary expression
 *
 *	@param	bld				AST builder
 *	@param	LHS				Left operand
 *	@param	RHS				Right operand
 *	@param	op_kind			Operator kind
 *	@param	op_loc			Operator location
 *
 *	@return	Binary expression node
 */
node build_binary_expression(builder *const bld, node *const LHS, node *const RHS
	, const binary_t op_kind, const location op_loc);

/**
 *	Build a ternary expression
 *
 *	@param	bld				AST builder
 *	@param	cond			First operand
 *	@param	LHS				Second operand
 *	@param	RHS				Third operand
 *	@param	op_loc			Operator location
 *
 *	@return	Ternary expression node
 */
node build_ternary_expression(builder *const bld, node *const cond, node *const LHS, node *const RHS, location op_loc);

/**
 *	Build an initializer list
 *
 *	@param	bld				AST builder
 *	@param	exprs			Vector of expressions
 *	@param	l_loc			Left brace location
 *	@param	r_loc			Right brace location
 *
 *	@return	Initializer list
 */
node build_initializer_list(builder *const bld, node_vector *const exprs, const location l_loc, const location r_loc);


/**
 *	Build a labeled statement
 *
 *	@param	bld				AST builder
 *	@param	name			Index in representations table
 *	@param	substmt			Substatement
 *	@param	id_loc			Identifier location
 *
 *	@return	Labeled statement
 */
node build_labeled_statement(builder *const bld, const size_t name, node *const substmt, const location id_loc);

/**
 *	Build a case statement
 *
 *	@param	bld				AST builder
 *	@param	expr			Case expression
 *	@param	substmt			Substatement
 *	@param	case_loc		Keyword location
 *
 *	@return	Case statement
 */
node build_case_statement(builder *const bld, node *const expr, node *const substmt, const location case_loc);

/**
 *	Build a default statement
 *
 *	@param	bld				AST builder
 *	@param	substmt			Substatement
 *	@param	default_loc		Keyword location
 *
 *	@return	Default statement
 */
node build_default_statement(builder *const bld, node *const substmt, const location default_loc);

/**
 *	Build a compound statement
 *
 *	@param	bld				AST builder
 *	@param	stmts			Vector of substatements
 *	@param	l_loc			Left brace location
 *	@param	r_loc			Right brace location
 *
 *	@return	Compound statement
 */
node build_compound_statement(builder *const bld, node_vector *const stmts, location l_loc, location r_loc);

/**
 *	Build an if statement
 *
 *	@param	bld				AST builder
 *	@param	cond			Contidion
 *	@param	then_stmt		Then-substatement
 *	@param	else_stmt		Else-substatement
 *	@param	if_loc			Keyword location
 *
 *	@return	If statement
 */
node build_if_statement(builder *const bld, node *const cond
	, node *const then_stmt, node *const else_stmt, const location if_loc);

/**
 *	Build a switch statement
 *
 *	@param	bld				AST builder
 *	@param	cond			Contidion
 *	@param	body			Substatement
 *	@param	switch_loc		Keyword location
 *
 *	@return	Switch statement
 */
node build_switch_statement(builder *const bld, node *const cond, node *const body, const location switch_loc);

/**
 *	Build a while statement
 *
 *	@param	bld				AST builder
 *	@param	cond			Contidion
 *	@param	body			Substatement
 *	@param	while_loc		Keyword location
 *
 *	@return	While statement
 */
node build_while_statement(builder *const bld, node *const cond, node *const body, const location while_loc);

/**
 *	Build a do statement
 *
 *	@param	bld				AST builder
 *	@param	body			Substatement
 *	@param	cond			Contidion
 *	@param	do_loc			Keyword location
 *
 *	@return	Do statement
 */
node build_do_statement(builder *const bld, node *const body, node *const cond, const location do_loc);

/**
 *	Build a goto statement
 *
 *	@param	bld				AST builder
 *	@param	name			Index in representations table
 *	@param	goto_loc		Keyword location
 *	@param	id_loc			Identifier location	
 *
 *	@return	Goto statement
 */
node build_goto_statement(builder *const bld, const size_t name, const location goto_loc, const location id_loc);

/**
 *	Build a continue statement
 *
 *	@param	bld				AST builder
 *	@param	continue_loc	Keyword location
 *
 *	@return	Continue statement
 */
node build_continue_statement(builder *const bld, const location continue_loc);

/**
 *	Build a break statement
 *
 *	@param	bld				AST builder
 *	@param	break_loc		Keyword location
 *
 *	@return	Break statement
 */
node build_break_statement(builder *const bld, const location break_loc);

/**
 *	Build a return statement
 *
 *	@param	bld				AST builder
 *	@param	return_loc		Keyword location
 *
 *	@return	Return statement
 */
node build_return_statement(builder *const bld, node *const expr, const location return_loc);

#ifdef __cplusplus
} /* extern "C" */
#endif
