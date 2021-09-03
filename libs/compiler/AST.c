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

#include "AST.h"

extern node node_broken();

extern item_t expression_get_type(const node *const nd);
extern bool expression_is_lvalue(const node *const nd);
extern location expression_get_location(const node *const nd);

extern size_t expression_identifier_get_id(const node *const nd);
extern int expression_literal_get_integer(const node *const nd);
extern double expression_literal_get_floating(const node *const nd);
extern size_t expression_literal_get_string(const node *const nd);

extern node expression_subscript_get_base(const node *const nd);
extern node expression_subscript_get_index(const node *const nd);

extern node expression_call_get_callee(const node *const nd);
extern node expression_call_get_argument(const node *const nd, const size_t index);

extern node expression_member_get_base(const node *const nd);
extern size_t expression_member_get_member_displ(const node *const nd);

extern unary_t expression_unary_get_operator(const node *const nd);
extern node expression_unary_get_operand(const node *const nd);

extern binary_t expression_binary_get_operator(const node *const nd);
extern node expression_binary_get_LHS(const node *const nd);
extern node expression_binary_get_RHS(const node *const nd);

extern node expression_ternary_get_condition(const node *const nd);
extern node expression_ternary_get_LHS(const node *const nd);
extern node expression_ternary_get_RHS(const node *const nd);

extern size_t expression_list_get_size(const node *const nd);
extern node expression_list_get_subexpr(const node *const nd, const size_t index);


extern size_t statement_labeled_get_label(const node *const nd);
extern node statement_labeled_get_substmt(const node *const nd);

extern size_t statement_compound_get_size(const node *const nd);
extern node statement_compound_get_substmt(const node *const nd, const size_t index);

extern bool statement_if_has_else_substmt(const node *const nd);
extern node statement_if_get_condition(const node *const nd);
extern node statement_if_get_then_substmt(const node *const nd);
extern node statement_if_get_else_substmt(const node *const nd);

extern node statement_while_get_condition(const node *const nd);
extern node statement_while_get_body(const node *const nd);

extern node statement_do_get_condition(const node *const nd);
extern node statement_do_get_body(const node *const nd);

extern bool statement_for_has_inition(const node *const nd);
extern bool statement_for_has_condition(const node *const nd);
extern bool statement_for_has_increment(const node *const nd);
extern node statement_for_get_inition(const node *const nd);
extern node statement_for_get_condition(const node *const nd);
extern node statement_for_get_increment(const node *const nd);
extern node statement_for_get_body(const node *const nd);

extern size_t statement_goto_get_label(const node *const nd);

extern bool statement_return_has_expression(const node *const nd);
extern node statement_return_get_expression(const node *const nd);

extern size_t statement_printf_get_argc(const node *const nd);
extern node statement_printf_get_format_str(const node *const nd);
extern node statement_printf_get_argument(const node *const nd, const size_t index);


extern size_t translation_unit_get_size(const node *const nd);
extern node translation_unit_get_declaration(const node *const nd, const size_t index);

extern size_t declaration_variable_get_id(const node *const nd);
extern bool declaration_variable_has_initializer(const node *const nd);
extern node declaration_variable_get_initializer(const node *const nd);
extern size_t declaration_variable_get_dim_amount(const node *const nd);
extern node declaration_variable_get_dim_expr(const node *const nd, const size_t index);

extern size_t declaration_function_get_id(const node *const nd);
extern node declaration_function_get_body(const node *const nd);


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


expression_t expression_get_class(const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_IDENTIFIER:
			return EXPR_IDENTIFIER;
		case OP_LITERAL:
			return EXPR_LITERAL;
		case OP_SLICE:
			return EXPR_SUBSCRIPT;
		case OP_CALL:
			return EXPR_CALL;
		case OP_SELECT:
			return EXPR_MEMBER;
		case OP_UNARY:
			return EXPR_UNARY;
		case OP_BINARY:
			return EXPR_BINARY;
		case OP_TERNARY:
			return EXPR_TERNARY;
		case OP_LIST:
			return EXPR_LIST;
		default:
			system_error(node_unexpected);
			return 0;
	}
}

statement_t statement_get_class(const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_DECL_VAR:
		case OP_DECL_TYPE:
		case OP_FUNC_DEF:
			return STMT_DECL;
		case OP_LABEL:
			return STMT_LABEL;
		case OP_CASE:
			return STMT_CASE;
		case OP_DEFAULT:
			return STMT_DEFAULT;
		case OP_BLOCK:
			return STMT_COMPOUND;
		case OP_NOP:
			return STMT_NULL;
		case OP_IF:
			return STMT_IF;
		case OP_SWITCH:
			return STMT_SWITCH;
		case OP_WHILE:
			return STMT_WHILE;
		case OP_DO:
			return STMT_DO;
		case OP_FOR:
			return STMT_FOR;
		case OP_GOTO:
			return STMT_GOTO;
		case OP_CONTINUE:
			return STMT_CONTINUE;
		case OP_BREAK:
			return STMT_BREAK;
		case OP_RETURN:
			return STMT_RETURN;
		case OP_PRINTF:
			return STMT_PRINTF;
		default:
			return STMT_EXPR;
	}
}

declaration_t declaration_get_class(const node *const nd)
{
	switch (node_get_type(nd))
	{
		case OP_DECL_VAR:
			return DECL_VAR;
		case OP_DECL_TYPE:
			return DECL_TYPE;
		case OP_FUNC_DEF:
			return DECL_FUNC;
		default:
			system_error(node_unexpected);
			return 0;
	}
}


size_t declaration_function_get_param(const node *const nd, const size_t index)
{
	if (node_get_type(nd) != OP_FUNC_DEF)
	{
		return SIZE_MAX;
	}

	const node nd_parameter = node_get_child(nd, index);
	return (size_t)node_get_arg(&nd_parameter, 0);
}
