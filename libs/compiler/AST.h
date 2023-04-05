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

#include <stdlib.h>
#include "node_vector.h"
#include "operations.h"
#include "syntax.h"
#include "tree.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Value category */
typedef enum CATEGORY
{
	LVALUE,					/**< An expression that designates an object */
	RVALUE,					/**< An expression detached from any specific storage */
} category_t;

/** Expression class */
typedef enum EXPRESSION
{
	EXPR_IDENTIFIER,	/**< Identifier expression */
	EXPR_LITERAL,		/**< Literal expression */
	EXPR_SUBSCRIPT,		/**< Subscript expression */
	EXPR_CALL,			/**< Call expression */
	EXPR_MEMBER,		/**< Member expression */
	EXPR_CAST,			/**< Cast expression */
	EXPR_UNARY,			/**< Unary expression */
	EXPR_BINARY,		/**< Binary expression */
	EXPR_TERNARY,		/**< Ternary expression */
	EXPR_ASSIGNMENT,	/**< Assignment expression */
	EXPR_INITIALIZER,	/**< Initializer */
	EXPR_EMPTY_BOUND,	/**< Empty array size expression */
	EXPR_INVALID,		/**< Invalid expression */
} expression_t;

/** Statement class */
typedef enum STATEMENT
{
	STMT_DECL,			/**< Declaration statement */
	STMT_CASE,			/**< Case statement */
	STMT_DEFAULT,		/**< Default statement */
	STMT_COMPOUND,		/**< Compound statement */
	STMT_EXPR,			/**< Expression statement */
	STMT_NULL,			/**< Null statement */
	STMT_IF,			/**< If statement */
	STMT_SWITCH,		/**< Switch statement */
	STMT_WHILE,			/**< While statement */
	STMT_DO,			/**< Do statement */
	STMT_FOR,			/**< For statement */
	STMT_CONTINUE,		/**< Continue statement */
	STMT_BREAK,			/**< Break statement */
	STMT_RETURN,		/**< Return statement */
} statement_t;

/** Declaration class */
typedef enum DECLARATION
{
	DECL_VAR,			/**< Variable declaration */
	DECL_FUNC,			/**< Function declaration */
	DECL_MEMBER,		/**< Member declaration */
	DECL_STRUCT,		/**< Struct declaration */
	DECL_INVALID,		/**< Invalid declaration */
} declaration_t;


/**
 *	Build a broken node
 *
 *	@return	Broken node
 */
inline node node_broken(void)
{
	return node_load(NULL, SIZE_MAX);
}

/**
 *	Get node location
 *
 *	@param	nd				Node
 *
 *	@return	Node location
 */
range_location node_get_location(const node *const nd);


/**
 *	Get expression class
 *
 *	@param	nd				Expression
 *
 *	@return	Expression class
 */
expression_t expression_get_class(const node *const nd);

/**
 *	Get expression type
 *
 *	@param	nd				Expression
 *
 *	@return	Expression type
 */
inline item_t expression_get_type(const node *const nd)
{
	return node_get_arg(nd, 0);
}

/**
 *	Check if expression is lvalue
 *
 *	@param	nd				Expression
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool expression_is_lvalue(const node *const nd)
{
	return node_get_arg(nd, 1) == LVALUE;
}


/**
 *	Create new identifier expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	id				Index in identifiers table
 *	@param	loc				Identifier location
 *
 *	@return	Identifier expression
 */
node expression_identifier(node *const context, const item_t type, const size_t id, const range_location loc);

/**
 *	Get index in indentifiers table of identifier expression
 *
 *	@param	nd				Identifier expression
 *
 *	@return	Index in identifiers table
 */
size_t expression_identifier_get_id(const node *const nd);


/**
 *	Create new null literal expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	loc				Literal location
 *
 *	@return	Null literal expression
 */
node expression_null_literal(node *const context, const item_t type, const range_location loc);


/**
 *	Create new boolean literal expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	value			Literal value
 *	@param	loc				Literal location
 *
 *	@return	Boolean literal expression
 */
node expression_boolean_literal(node *const context, const item_t type, const bool value, const range_location loc);

/**
 *	Get value of boolean literal expression
 *
 *	@param	nd				Literal expression
 *
 *	@return	Boolean value
 */
bool expression_literal_get_boolean(const node *const nd);


/**
 *	Create new character literal expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	value			Literal value
 *	@param	loc				Literal location
 *
 *	@return	Character literal expression
 */
node expression_character_literal(node *const context, const item_t type, const char32_t value, const range_location loc);

/**
 *	Get value of character literal expression
 *
 *	@param	nd				Literal expression
 *
 *	@return	Character value
 */
char32_t expression_literal_get_character(const node *const nd);


/**
 *	Create new integer literal expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	value			Literal value
 *	@param	loc				Literal location
 *
 *	@return	Integer literal expression
 */
node expression_integer_literal(node *const context, const item_t type, const item_t value, const range_location loc);

/**
 *	Get value of integer literal expression
 *
 *	@param	nd				Literal expression
 *
 *	@return	Integer value
 */
item_t expression_literal_get_integer(const node *const nd);


/**
 *	Create new floating literal expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	value			Literal value
 *	@param	loc				Literal location
 *
 *	@return	Floating literal expression
 */
node expression_floating_literal(node *const context, const item_t type, const double value, const range_location loc);

/**
 *	Get value of floating literal expression
 *
 *	@param	nd				Literal expression
 *
 *	@return	Floating value
 */
double expression_literal_get_floating(const node *const nd);


/**
 *	Create new string literal expression
 *
 *	@param	context			Context node
 *	@param	type			Value type
 *	@param	index			String index
 *	@param	loc				Literal location
 *
 *	@return	String literal expression
 */
node expression_string_literal(node *const context, const item_t type, const size_t index, const range_location loc);

/**
 *	Get string index of literal expression
 *
 *	@param	nd				Literal expression
 *
 *	@return	String index
 */
size_t expression_literal_get_string(const node *const nd);


/**
 *	Create new subscript expression
 *
 *	@param	type			Value type
 *	@param	base			Base expression
 *	@param	index			Index expression
 *	@param	loc				Expression location
 *
 *	@return	Subscript expression
 */
node expression_subscript(const item_t type, node *const base, node *const index, const range_location loc);

/**
 *	Get base expression of subscript expression
 *
 *	@param	nd				Subscript expression
 *
 *	@return	Base expression
 */
node expression_subscript_get_base(const node *const nd);

/**
 *	Get index expression of subscript expression
 *
 *	@param	nd				Subscript expression
 *
 *	@return	Index expression
 */
node expression_subscript_get_index(const node *const nd);


/**
 *	Create new call expression
 *
 *	@param	type			Value type
 *	@param	callee			Called expression
 *	@param	args			Arguments of call
 *	@param	loc				Expression location
 *
 *	@return	Call expression
 */
node expression_call(const item_t type, node *const callee, node_vector *const args, const range_location loc);

/**
 *	Get called expression of call expression
 *
 *	@param	nd				Call expression
 *
 *	@return	Called expression
 */
node expression_call_get_callee(const node *const nd);

/**
 *	Get arguments amount of call expression
 *
 *	@param	nd				Call expression
 *
 *	@return	Arguments amount
 */
size_t expression_call_get_arguments_amount(const node *const nd);

/**
 *	Get argument of call expression by index
 *
 *	@param	nd				Call expression
 *	@param	index			Argument index
 *
 *	@return	Argument
 */
node expression_call_get_argument(const node *const nd, const size_t index);


/**
 *	Create new member expression
 *
 *	@param	type			Value type
 *	@param	ctg				Value category
 *	@param	index			Member index
 *	@param	is_arrow		Set if operator is '->'
 *	@param	loc				Expression location
 *
 *	@return	Member expression
 */
node expression_member(const item_t type, const category_t ctg
	, const size_t index, bool is_arrow, node *const base, const range_location loc);

/**
 *	Get base expression of member expression
 *
 *	@param	nd				Member expression
 *
 *	@return	Base expression
 */
node expression_member_get_base(const node *const nd);

/**
 *	Get member index of member expression
 *
 *	@param	nd				Member expression
 *
 *	@return	Member index
 */
size_t expression_member_get_member_index(const node *const nd);

/**
 *	Check if operator of member expression is '->'
 *
 *	@param	nd				Member expression
 *
 *	@return	@c 1 on true, @c on false
 */
bool expression_member_is_arrow(const node *const nd);


/**
 *	Create new cast expression
 *
 *	@param	target_type		Value target type
 *	@param	source_type		Source type
 *	@param	expr			Operand
 *	@param	loc				Expression location
 *
 *	@return	Cast expression
 */
node expression_cast(const item_t target_type, const item_t source_type, node *const expr, const range_location loc);

/**
 *	Get source type of cast expression
 *
 *	@param	nd				Cast expression
 *
 *	@return	Source type
 */
item_t expression_cast_get_source_type(const node *const nd);

/**
 *	Get operand of cast expression
 *
 *	@param	nd				Cast expression
 *
 *	@return	Operand
 */
node expression_cast_get_operand(const node *const nd);


/**
 *	Create new unary expression
 *
 *	@param	type			Value type
 *	@param	ctg				Value category
 *	@param	expr			Operand
 *	@param	op				Operator kind
 *	@param	loc				Expression location
 *
 *	@return	Unary expression
 */
node expression_unary(const item_t type, const category_t ctg, node *const expr, const unary_t op, const range_location loc);

/**
 *	Get operator of unary expression
 *
 *	@param	nd				Unary expression
 *
 *	@return	Operator
 */
unary_t expression_unary_get_operator(const node *const nd);

/**
 *	Get operand of unary expression
 *
 *	@param	nd				Unary expression
 *
 *	@return	Operand
 */
node expression_unary_get_operand(const node *const nd);


/**
 *	Create new binary expression
 *
 *	@param	type			Value type
 *	@param	LHS				Left operand
 *	@param	RHS				Right operand
 *	@param	op				Operator kind
 *	@param	loc				Expression location
 *
 *	@return	Binary expression
 */
node expression_binary(const item_t type, node *const LHS, node *const RHS, const binary_t op, const range_location loc);

/**
 *	Get operator of binary expression
 *
 *	@param	nd				Binary expression
 *
 *	@return	Operator
 */
binary_t expression_binary_get_operator(const node *const nd);

/**
 *	Get LHS of binary expression
 *
 *	@param	nd				Binary expression
 *
 *	@return	LHS of binary expression
 */
node expression_binary_get_LHS(const node *const nd);

/**
 *	Get RHS of binary expression
 *
 *	@param	nd				Binary expression
 *
 *	@return	RHS of binary expression
 */
node expression_binary_get_RHS(const node *const nd);


/**
 *	Create new ternary expression
 *
 *	@param	type			Value type
 *	@param	cond			First operand
 *	@param	LHS				Second operand
 *	@param	RHS				Third operand
 *	@param	loc				Expression location
 *
 *	@return	Ternary expression
 */
node expression_ternary(const item_t type, node *const cond, node *const LHS, node *const RHS, const range_location loc);

/**
 *	Get condition of ternary expression
 *
 *	@param	nd				Ternary expression
 *
 *	@return	Condition
 */
node expression_ternary_get_condition(const node *const nd);

/**
 *	Get LHS of ternary expression
 *
 *	@param	nd				Ternary expression
 *
 *	@return	LHS of ternary expression
 */
node expression_ternary_get_LHS(const node *const nd);

/**
 *	Get RHS of ternary expression
 *
 *	@param	nd				Ternary expression
 *
 *	@return	RHS of ternary expression
 */
node expression_ternary_get_RHS(const node *const nd);


/**
 *	Create new assignment expression
 *
 *	@param	type			Value type
 *	@param	LHS				Left operand
 *	@param	RHS				Right operand
 *	@param	op				Operator kind
 *	@param	loc				Expression location
 *
 *	@return	Assignment expression
 */
node expression_assignment(const item_t type, node *const LHS, node *const RHS, const binary_t op, const range_location loc);

/**
 *	Get operator of assignment expression
 *
 *	@param	nd				Assignment expression
 *
 *	@return	Operator
 */
binary_t expression_assignment_get_operator(const node *const nd);

/**
 *	Get LHS of assignment expression
 *
 *	@param	nd				Assignment expression
 *
 *	@return	LHS of assignment expression
 */
node expression_assignment_get_LHS(const node *const nd);

/**
 *	Get RHS of assignment expression
 *
 *	@param	nd				Assignment expression
 *
 *	@return	RHS of assignment expression
 */
node expression_assignment_get_RHS(const node *const nd);


/**
 *	Create new initializer
 *
 *	@param	exprs			Subexpressions
 *	@param	loc				Expression location
 *
 *	@return	Initializer
 */
node expression_initializer(node_vector *const exprs, const range_location loc);

/**
 *	Set type of initializer
 *
 *	@param	nd				Initializer
 *	@param	type			Type
 */
void expression_initializer_set_type(const node *const nd, const item_t type);

/**
 *	Get size of initializer
 *
 *	@param	nd				Initializer
 *
 *	@return	Size
 */
size_t expression_initializer_get_size(const node *const nd);

/**
 *	Get subexpression of initializer by index
 *
 *	@param	nd				Initializer
 *	@param	index			Subexpression index
 *
 *	@return	Expression
 */
node expression_initializer_get_subexpr(const node *const nd, const size_t index);


/**
 *	Create new empty bound expression
 *
 *	@param	context			Context node
 *	@param	loc				Expression location
 *
 *	@return	Empty bound expression
 */
node expression_empty_bound(node *const context, const range_location loc);


/**
 *	Get statement class
 *
 *	@param	nd				Statement
 *
 *	@return	Statement class
 */
statement_t statement_get_class(const node *const nd);


/**
 *	Create new case statement
 *
 *	@param	expr			Case expression
 *	@param	substmt			Substatement
 *	@param	loc				Statement location
 *
 *	@return	Case statement
 */
node statement_case(node *const expr, node *const substmt, const range_location loc);

/**
 *	Get expression of case statement
 *
 *	@param	nd				Case statement
 *
 *	@return	Expression
 */
node statement_case_get_expression(const node *const nd);

/**
 *	Get substatement of case statement
 *
 *	@param	nd				Case statement
 *
 *	@return	Substatement
 */
node statement_case_get_substmt(const node *const nd);


/**
 *	Create new default statement
 *
 *	@param	substmt			Substatement
 *	@param	loc				Statement location
 *
 *	@return	Default statement
 */
node statement_default(node *const substmt, const range_location loc);

/**
 *	Get substatement of default statement
 *
 *	@param	nd				Default statement
 *
 *	@return	Substatement
 */
node statement_default_get_substmt(const node *const nd);


/**
 *	Create new compound statement
 *
 *	@param	context			Context node
 *	@param	stmts			Substatements
 *	@param	loc				Statement location
 *
 *	@return	Compound statement
 */
node statement_compound(node *const context, node_vector *const stmts, const range_location loc);

/**
 *	Get size of compound statement
 *
 *	@param	nd				Compound statement
 *
 *	@return	Size
 */
size_t statement_compound_get_size(const node *const nd);

/**
 *	Get substatement of compound statement by index
 *
 *	@param	nd				Compound statement
 *	@param	index			Substatement index
 *
 *	@return	Substatement
 */
node statement_compound_get_substmt(const node *const nd, const size_t index);


/**
 *	Create new null statement
 *
 *	@param	context			Context node
 *	@param	loc				Statement location
 *
 *	@return	Null statement
 */
node statement_null(node *const context, const range_location loc);


/**
 *	Create new if statement
 *
 *	@param	cond			Contidion
 *	@param	then_stmt		Then-substatement
 *	@param	else_stmt		Else-substatement
 *	@param	loc				Statement location
 *
 *	@return	If statement
 */
node statement_if(node *const cond, node *const then_stmt, node *const else_stmt, const range_location loc);

/**
 *	Check if if statement has else-substatement
 *
 *	@param	nd				If statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool statement_if_has_else_substmt(const node *const nd);

/**
 *	Get condition of if statement
 *
 *	@param	nd				If statement
 *
 *	@return	Condition
 */
node statement_if_get_condition(const node *const nd);

/**
 *	Get then-substatement of if statement
 *
 *	@param	nd				If statement
 *
 *	@return	Then-substatement
 */
node statement_if_get_then_substmt(const node *const nd);

/**
 *	Get else-substatement of if statement
 *
 *	@param	nd				If statement
 *
 *	@return	Else-substatement
 */
node statement_if_get_else_substmt(const node *const nd);


/**
 *	Create new switch statement
 *
 *	@param	cond			Contidion
 *	@param	body			Substatement
 *	@param	loc				Statement location
 *
 *	@return	Switch statement
 */
node statement_switch(node *const cond, node *const body, const range_location loc);

/**
 *	Get condition of switch statement
 *
 *	@param	nd				Switch statement
 *
 *	@return	Condition
 */
node statement_switch_get_condition(const node *const nd);

/**
 *	Get substatement of switch statement
 *
 *	@param	nd				Switch statement
 *
 *	@return	Substatement
 */
node statement_switch_get_body(const node *const nd);


/**
 *	Create new while statement
 *
 *	@param	cond			Contidion
 *	@param	body			Substatement
 *	@param	loc				Statement location
 *
 *	@return	While statement
 */
node statement_while(node *const cond, node *const body, const range_location loc);

/**
 *	Get condition of while statement
 *
 *	@param	nd				While statement
 *
 *	@return	Condition
 */
node statement_while_get_condition(const node *const nd);

/**
 *	Get substatement of while statement
 *
 *	@param	nd				While statement
 *
 *	@return	Substatement
 */
node statement_while_get_body(const node *const nd);


/**
 *	Create new do statement
 *
 *	@param	body			Substatement
 *	@param	cond			Contidion
 *	@param	loc				Statement location
 *
 *	@return	Do statement
 */
node statement_do(node *const body, node *const cond, const range_location loc);

/**
 *	Get condition of do statement
 *
 *	@param	nd				Do statement
 *
 *	@return	Condition
 */
node statement_do_get_condition(const node *const nd);

/**
 *	Get substatement of do statement
 *
 *	@param	nd				Do statement
 *
 *	@return	Substatement
 */
node statement_do_get_body(const node *const nd);


/**
 *	Create new for statement
 *
 *	@param	init			Inition
 *	@param	cond			Contidion
 *	@param	incr			Increment
 *	@param	body			Substatement
 *	@param	loc				Statement location
 *
 *	@return	For statement
 */
node statement_for(node *const init, node *const cond, node *const incr, node *const body, const range_location loc);

/**
 *	Check if for statement has inition
 *
 *	@param	nd				For statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool statement_for_has_inition(const node *const nd);

/**
 *	Check if for statement has condition
 *
 *	@param	nd				For statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool statement_for_has_condition(const node *const nd);

/**
 *	Check if for statement has increment
 *
 *	@param	nd				For statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool statement_for_has_increment(const node *const nd);

/**
 *	Get inition of for statement
 *
 *	@param	nd				For statement
 *
 *	@return	Inition
 */
node statement_for_get_inition(const node *const nd);

/**
 *	Get condition of for statement
 *
 *	@param	nd				For statement
 *
 *	@return	condition
 */
node statement_for_get_condition(const node *const nd);

/**
 *	Get increment of for statement
 *
 *	@param	nd				For statement
 *
 *	@return	Increment
 */
node statement_for_get_increment(const node *const nd);

/**
 *	Get substatement of for statement
 *
 *	@param	nd				For statement
 *
 *	@return	Substatement
 */
node statement_for_get_body(const node *const nd);


/**
 *	Create new continue statement
 *
 *	@param	context			Context node
 *	@param	loc				Statement location
 *
 *	@return	Continue statement
 */
node statement_continue(node *const context, const range_location loc);


/**
 *	Create new break statement
 *
 *	@param	context			Context node
 *	@param	loc				Statement location
 *
 *	@return	Break statement
 */
node statement_break(node *const context, const range_location loc);


/**
 *	Create new return statement
 *
 *	@param	context			Context node
 *	@param	expr			Expression
 *	@param	loc				Statement location
 *
 *	@return	Return statement
 */
node statement_return(node *const context, node *const expr, const range_location loc);

/**
 *	Check if return statement has expression
 *
 *	@param	nd				Return statement
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool statement_return_has_expression(const node *const nd);

/**
 *	Get expression of return statement
 *
 *	@param	nd				Return statement
 *
 *	@return	Expression
 */
node statement_return_get_expression(const node *const nd);


/**
 *	Create new empty declaration statement
 *
 *	@param	context			Context node
 *
 *	@return	Declaration statement
 */
node statement_declaration(node *const context);

/**
 *	Add declarator to a declaration statement
 *
 *	@param	nd				Declaration statement
 *	@param	declarator		Declarator
 */
void statement_declaration_add_declarator(const node *const nd, node *const declarator);

/**
 *	Set location of a declaration statement
 *
 *	@param	nd				Declaration statement
 *	@param	loc				Statement location
 *
 *	@return	Declatation statement
 */
node statement_declaration_set_location(const node *const nd, const range_location loc);

/**
 *	Get size of declaration statement
 *
 *	@param	nd				Declaration statement
 *
 *	@return	Size
 */
size_t statement_declaration_get_size(const node *const nd);

/**
 *	Get declarator of declaration statement by index
 *
 *	@param	nd				Declaration statement
 *	@param	index			Declarator index
 *
 *	@return	Declarator
 */
node statement_declaration_get_declarator(const node *const nd, const size_t index);


/**
 *	Get declaration class
 *
 *	@param	nd				Declaration
 *
 *	@return	Declaration class
 */
declaration_t declaration_get_class(const node *const nd);


/**
 *	Create new member declaration
 *
 *	@param	context			Context node
 *	@param	type			Member type
 *	@param	name			Member name
 *	@param	bounds			Array member bound expressions
 *	@param	loc				Declaration location
 *
 *	@return	Member declaration
 */
node declaration_member(node *const context, const item_t type, const size_t name
	, node_vector *const bounds, const range_location loc);

/**
 *	Get name of member in member declaration
 *
 *	@param	nd				Member declaration
 *
 *	@return	Member name
 */
size_t declaration_member_get_name(const node *const nd);

/**
 *	Get type of member in member declaration
 *
 *	@param	nd				Member declaration
 *
 *	@return	Member type
 */
item_t declaration_member_get_type(const node *const nd);

/**
 *	Get amount of bounds in member declaration
 *
 *	@param	nd				Member declaration
 *
 *	@return	Amount of bounds
 */
size_t declaration_member_get_bounds_amount(const node *const nd);

/**
 *	Get bound expression in variable declaration by index
 *
 *	@param	nd				Variable declaration
 *	@param	index			Bound index
 *
 *	@return	Bound expression
 */
node declaration_member_get_bound(const node *const nd, const size_t index);


/**
 *	Create new empty struct declaration
 *
 *	@param	context			Context node
 *	@param	name			Struct name
 *	@param	loc				Declaration location
 *
 *	@return	Struct declaration
 */
node declaration_struct(node *const context, const size_t name, const range_location loc);

/**
 *	Add member declaration to a struct declaration
 *
 *	@param	nd				Struct declaration
 *	@param	member			Member declaration
 */
void declaration_struct_add_declarator(node *const nd, node *const member);

/**
 *	Set type of a struct declaration
 *
 *	@param	nd				Struct declaration
 *	@param	type			Struct type
 */
void declaration_struct_set_type(node *const nd, const item_t type);

/**
 *	Set location of a struct declaration
 *
 *	@param	nd				Struct declaration
 *	@param	loc				Declaration location
 *
 *	@return	Struct declaration
 */
node declaration_struct_set_location(node *const nd, const range_location loc);

/**
 *	Get struct name of struct declaration
 *
 *	@param	nd				Struct declaration
 *
 *	@return	Struct name
 */
size_t declaration_struct_get_name(const node *const nd);

/**
 *	Get struct type of struct declaration
 *
 *	@param	nd				Struct declaration
 *
 *	@return	Struct type
 */
item_t declaration_struct_get_type(const node *const nd);

/**
 *	Get size of declaration statement
 *
 *	@param	nd				Declaration statement
 *
 *	@return	Size
 */
size_t declaration_struct_get_size(const node *const nd);

/**
 *	Get member declaration of struct declaration by index
 *
 *	@param	nd				Struct declaration
 *	@param	index			Member index
 *
 *	@return	Member declaration
 */
node declaration_struct_get_member(const node *const nd, const size_t index);


/**
 *	Create new variable declaration
 *
 *	@param	context			Context node
 *	@param	id				Variable identifier
 *	@param	bounds			Array bound expressions
 *	@param	initializer		Initializer
 *	@param	loc				Declaration location
 *
 *	@return	Variable declaration
 */
node declaration_variable(node *const context, const size_t id, node_vector *const bounds
	, node *const initializer, const range_location loc);

/**
 *	Get variable identifier in variable declaration
 *
 *	@param	nd				Variable declaration
 *
 *	@return	Variable identifier
 */
size_t declaration_variable_get_id(const node *const nd);

/**
 *	Check if variable declaration has initializer
 *
 *	@param	nd				Variable declaration
 *
 *	@return	@c 1 on true, @c 0 on false
 */
bool declaration_variable_has_initializer(const node *const nd);

/**
 *	Get initializer of variable declaration
 *
 *	@param	nd				Variable declaration
 *
 *	@return	Initializer
 */
node declaration_variable_get_initializer(const node *const nd);

/**
 *	Get amount of bounds in variable declaration
 *
 *	@param	nd				Variable declaration
 *
 *	@return	Amount of bounds
 */
size_t declaration_variable_get_bounds_amount(const node *const nd);

/**
 *	Get bound expression in variable declaration by index
 *
 *	@param	nd				Variable declaration
 *	@param	index			Bound index
 *
 *	@return	Bound expression
 */
node declaration_variable_get_bound(const node *const nd, const size_t index);


/**
 *	Get id of function in function declaration
 *
 *	@param	nd				Function declaration
 *
 *	@return Function id
 */
size_t declaration_function_get_id(const node *const nd);

/**
 *	Get parameters amount in function declaration
 *
 *	@param	nd				Function declaration
 *
 *	@return Parameters amount
 */
size_t declaration_function_get_parameters_amount(const node *const nd);

/**
 *	Get parameter id in function declaration by index
 *
 *	@param	nd				Function declaration
 *	@param	index			Parameter index
 *
 *	@return Parameter id
 */
size_t declaration_function_get_parameter(const node *const nd, const size_t index);

/**
 *	Get body of function declaration
 *
 *	@param	nd				Function declaration
 *
 *	@return Function body
 */
node declaration_function_get_body(const node *const nd);


/**
 *	Get number of global declarations in translation unit
 *
 *	@param	nd				Translation unit
 *
 *	@return	Number of global declarations
 */
size_t translation_unit_get_size(const node *const nd);

/**
 *	Get global declaration of translation unit by index
 *
 *	@param	nd				Translation unit
 *	@param	index			Global declaration index
 *
 *	@return	Global declaration
 */
node translation_unit_get_declaration(const node *const nd, const size_t index);

#ifdef __cplusplus
} /* extern "C" */
#endif
