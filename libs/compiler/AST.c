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

extern size_t expression_call_get_arguments_amount(const node *const nd);
extern node expression_call_get_callee(const node *const nd);
extern size_t expression_call_get_arguments_amount(const node *const nd);
extern node expression_call_get_argument(const node *const nd, const size_t index);

extern node expression_member_get_base(const node *const nd);
extern size_t expression_member_get_member_index(const node *const nd);
extern bool expression_member_is_arrow(const node *const nd);

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

extern node statement_case_get_expression(const node *const nd);
extern node statement_case_get_substmt(const node *const nd);

extern node statement_default_get_substmt(const node *const nd);

extern size_t statement_compound_get_size(const node *const nd);
extern node statement_compound_get_substmt(const node *const nd, const size_t index);

extern bool statement_if_has_else_substmt(const node *const nd);
extern node statement_if_get_condition(const node *const nd);
extern node statement_if_get_then_substmt(const node *const nd);
extern node statement_if_get_else_substmt(const node *const nd);

extern node statement_switch_get_condition(const node *const nd);
extern node statement_switch_get_body(const node *const nd);

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

static inline node node_create(syntax *const sx, operation_t type)
 {
	return node_add_child(&sx->nd, type);
 }

 static inline void node_set_child(const node *const parent, const node *const child)
 {
	node temp = node_add_child(parent, OP_NOP);
	node_swap(child, &temp);
	node_remove(&temp);
 }

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

node expression_identifier(syntax *const sx, const item_t type
	, const category_t ctg, const size_t id, const location loc)
{
	node nd = node_create(sx, OP_IDENTIFIER);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, ctg);							// Категория значения выражения
	node_add_arg(&nd, (item_t)id);					// Индекс в таблице идентификаторов
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения

	return nd;
}

node expression_subscript(syntax *const sx, const item_t type
	, const node *const base, const node *const index, const location loc)
{
	node nd = node_create(sx, OP_SLICE);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, LVALUE);						// Категория значения выражения
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения
	node_set_child(&nd, base);						// Выражение-операнд
	node_set_child(&nd, index);						// Выражение-индекс

	return nd;
}

node expression_call(syntax *const sx, const item_t type
	, const node *const callee, node_vector *const args, const location loc)
{
	node nd = node_create(sx, OP_CALL);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, RVALUE);						// Категория значения выражения
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения
	node_set_child(&nd, callee);					// Вызываемое выражение

	if (args)
	{
		size_t amount = node_vector_size(args);
		for (size_t i = 0; i < amount; i++)
		{
			node arg = node_vector_get(args, i);
			node_set_child(&nd, &arg);					// i-ый аргумент вызова
		}

		node_vector_clear(args);
	}
	
	return nd;
}

node expression_member(syntax *const sx, const item_t type, const category_t ctg
	, const size_t i, const bool is_arrow, const node *const base, const location loc)
{
	node nd = node_create(sx, OP_SELECT);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, ctg);							// Категория значения выражения
	node_add_arg(&nd, (item_t)i);					// Индекс поля выборки
	node_add_arg(&nd, is_arrow);					// Является ли оператор '->'
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения
	node_set_child(&nd, base);						// Операнд выражения

	return nd;
}

node expression_unary(syntax *const sx, const item_t type, const category_t ctg
	, const node *const expr, const unary_t op, const location loc)
{
	node nd = node_create(sx, OP_UNARY);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, ctg);							// Категория значения выражения
	node_add_arg(&nd, op);							// Вид унарного оператора
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения
	node_set_child(&nd, expr);						// Операнд выражения

	return nd;
}

node expression_binary(syntax *const sx, const item_t type
	, const node *const LHS, const node *const RHS, const binary_t op, const location loc)
{
	node nd = node_create(sx, OP_BINARY);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, RVALUE);						// Категория значения выражения
	node_add_arg(&nd, op);							// Вид бинарного оператора
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения
	node_set_child(&nd, LHS);						// Первый операнд выражения
	node_set_child(&nd, RHS);						// Второй операнд выражения

	return nd;
}

node expression_ternary(syntax *const sx, const item_t type, const node *const cond
	, const node *const LHS, const node *const RHS, const location loc)
{
	node nd = node_create(sx, OP_TERNARY);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, RVALUE);						// Категория значения выражения
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.begin);			// Конечная позиция выражения
	node_set_child(&nd, cond);						// Первый операнд выражения
	node_set_child(&nd, LHS);						// Второй операнд выражения
	node_set_child(&nd, RHS);						// Третий операнд выражения

	return nd;
}

node expression_list(syntax *const sx, const item_t type, node_vector *const exprs, const location loc)
{
	node nd = node_create(sx, OP_LIST);
	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, RVALUE);						// Категория значения выражения
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения

	size_t amount = node_vector_size(exprs);
	for (size_t i = 0; i < amount; i++)
	{
		node subexpr = node_vector_get(exprs, i);
		node_set_child(&nd, &subexpr);				// i-ое подвыражение списка
	}

	node_vector_clear(exprs);
	return nd;
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
		case OP_PRINT:
			return STMT_PRINT;
		case OP_PRINTID:
			return STMT_PRINTID;
		case OP_GETID:
			return STMT_GETID;
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
