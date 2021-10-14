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
extern location node_get_location(const node *const nd);

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

extern item_t expression_cast_get_source_type(const node *const nd);
extern node expression_cast_get_operand(const node *const nd);

extern unary_t expression_unary_get_operator(const node *const nd);
extern node expression_unary_get_operand(const node *const nd);

extern binary_t expression_binary_get_operator(const node *const nd);
extern node expression_binary_get_LHS(const node *const nd);
extern node expression_binary_get_RHS(const node *const nd);

extern node expression_ternary_get_condition(const node *const nd);
extern node expression_ternary_get_LHS(const node *const nd);
extern node expression_ternary_get_RHS(const node *const nd);

extern int expression_list_set_type(const node *const nd, const item_t type);
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


static inline node node_create(node *const context, const operation_t type)
{
	return node_add_child(context, type);
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
		case OP_CAST:
		   return EXPR_CAST;
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

node expression_identifier(node *const context, const item_t type, const size_t id, const location loc)
{
	node nd = node_create(context, OP_IDENTIFIER);

	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, LVALUE);						// Категория значения выражения
	node_add_arg(&nd, (item_t)id);					// Индекс в таблице идентификаторов
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения

	return nd;
}

node expression_integer_literal(node *const context, const item_t type, const item_t value, const location loc)
{
	node nd = node_create(context, OP_LITERAL);

	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, RVALUE);						// Категория значения выражения
	node_add_arg(&nd, value);						// Значение литерала
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения

	return nd;
}

node expression_floating_literal(node *const context, const item_t type, const double value, const location loc)
{
	node nd = node_create(context, OP_LITERAL);

	node_add_arg(&nd, type);						// Тип значения выражения
	node_add_arg(&nd, RVALUE);						// Категория значения выражения
	node_add_arg_double(&nd, value);				// Значение литерала
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения

	return nd;
}

node expression_subscript(const item_t type, node *const base, node *const index, const location loc)
{
	node nd = node_insert(base, OP_SLICE, 4);		// Выражение-операнд
	node_set_child(&nd, index);						// Выражение-индекс

	node_set_arg(&nd, 0, type);						// Тип значения выражения
	node_set_arg(&nd, 1, LVALUE);					// Категория значения выражения
	node_set_arg(&nd, 2, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 3, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_call(const item_t type, node *const callee, node_vector *const args, const location loc)
{
	node nd = node_insert(callee, OP_CALL, 4);		// Операнд выражения

	if (args)
	{
		const size_t amount = node_vector_size(args);
		for (size_t i = 0; i < amount; i++)
		{
			node arg = node_vector_get(args, i);
			node_set_child(&nd, &arg);				// i-ый аргумент вызова
		}
	}

	node_set_arg(&nd, 0, type);						// Тип значения выражения
	node_set_arg(&nd, 1, RVALUE);					// Категория значения выражения
	node_set_arg(&nd, 2, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 3, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_member(const item_t type, const category_t ctg
	, const size_t index, bool is_arrow, node *const base, const location loc)
{
	node nd = node_insert(base, OP_SELECT, 6);		// Операнд выражения

	node_set_arg(&nd, 0, type);						// Тип значения выражения
	node_set_arg(&nd, 1, ctg);						// Категория значения выражения
	node_set_arg(&nd, 2, (item_t)index);			// Индекс поля выборки
	node_set_arg(&nd, 3, is_arrow);					// Является ли оператор '->'
	node_set_arg(&nd, 4, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 5, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_cast(const item_t target_type, const item_t source_type, node *const expr, const location loc)
{
	node nd = node_insert(expr, OP_CAST, 5);		// Операнд выражения

	node_set_arg(&nd, 0, target_type);				// Тип значения выражения
	node_set_arg(&nd, 1, RVALUE);					// Категория значения выражения
	node_set_arg(&nd, 2, source_type);				// Тип до преобразования
	node_set_arg(&nd, 3, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 4, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_unary(const item_t type, const category_t ctg, node *const expr, const unary_t op, const location loc)
{
	node nd = node_insert(expr, OP_UNARY, 5);		// Операнд выражения

	node_set_arg(&nd, 0, type);						// Тип значения выражения
	node_set_arg(&nd, 1, ctg);						// Категория значения выражения
	node_set_arg(&nd, 2, op);						// Вид унарного оператора
	node_set_arg(&nd, 3, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 4, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_binary(const item_t type, node *const LHS, node *const RHS, const binary_t op, const location loc)
{
	node nd = node_insert(LHS, OP_BINARY, 5);		// Первый операнд выражения
	node_set_child(&nd, RHS);						// Второй операнд выражения

	node_set_arg(&nd, 0, type);						// Тип значения выражения
	node_set_arg(&nd, 1, RVALUE);					// Категория значения выражения
	node_set_arg(&nd, 2, op);						// Вид бинарного оператора
	node_set_arg(&nd, 3, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 4, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_ternary(const item_t type, node *const cond, node *const LHS, node *const RHS, const location loc)
{
	node nd = node_insert(cond, OP_TERNARY, 4);		// Первый операнд выражения
	node_set_child(&nd, LHS);						// Второй операнд выражения
	node_set_child(&nd, RHS);						// Третий операнд выражения

	node_set_arg(&nd, 0, type);						// Тип значения выражения
	node_set_arg(&nd, 1, RVALUE);					// Категория значения выражения
	node_set_arg(&nd, 2, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 3, (item_t)loc.end);			// Конечная позиция выражения

	return nd;
}

node expression_list(node_vector *const exprs, const location loc)
{
	node fst = node_vector_get(exprs, 0);
	node nd = node_insert(&fst, OP_LIST, 4);

	node_set_arg(&nd, 0, TYPE_UNDEFINED);			// Тип значения выражения
	node_set_arg(&nd, 1, RVALUE);					// Категория значения выражения
	node_set_arg(&nd, 2, (item_t)loc.begin);		// Начальная позиция выражения
	node_set_arg(&nd, 3, (item_t)loc.end);			// Конечная позиция выражения

	const size_t amount = node_vector_size(exprs);
	for (size_t i = 1; i < amount; i++)
	{
		node subexpr = node_vector_get(exprs, i);
		node_set_child(&nd, &subexpr);				// i-ое подвыражение списка
	}

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

node statement_labeled(const size_t label, node *const substmt, const location loc)
{
	node nd = node_insert(substmt, OP_LABEL, 3);

	node_set_arg(&nd, 0, (item_t)label);			// ID метки
	node_set_arg(&nd, 1, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 2, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_case(node *const expr, node *const substmt, const location loc)
{
	node nd = node_insert(expr, OP_CASE, 2);
	node_set_child(&nd, substmt);

	node_set_arg(&nd, 0, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 1, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_default(node *const substmt, const location loc)
{
	node nd = node_insert(substmt, OP_DEFAULT, 2);

	node_set_arg(&nd, 0, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 1, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_compound(node *const context, node_vector *const stmts, const location loc)
{
	node nd = node_create(context, OP_BLOCK);

	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция оператора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция оператора

	if (stmts)
	{
		const size_t amount = node_vector_size(stmts);
		for (size_t i = 0; i < amount; i++)
		{
			node item = node_vector_get(stmts, i);
			node_set_child(&nd, &item);
		}
	}

	return nd;
}

node statement_null(node *const context, const location loc)
{
	node nd = node_create(context, OP_NOP);

	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция оператора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция оператора

	return nd;
}

node statement_if(node *const cond, node *const then_stmt, node *const else_stmt, const location loc)
{
	node nd = node_insert(cond, OP_IF, 3);
	node_set_child(&nd, then_stmt);

	node_set_arg(&nd, 0, 0);						// Флаг наличия else-части
	node_set_arg(&nd, 1, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 2, (item_t)loc.end);			// Конечная позиция оператора

	if (else_stmt)
	{
		node_set_arg(&nd, 0, 1);
		node_set_child(&nd, else_stmt);
	}

	return nd;
}

node statement_switch(node *const cond, node *const body, const location loc)
{
	node nd = node_insert(cond, OP_SWITCH, 2);
	node_set_child(&nd, body);

	node_set_arg(&nd, 0, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 1, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_while(node *const cond, node *const body, const location loc)
{
	node nd = node_insert(cond, OP_WHILE, 2);
	node_set_child(&nd, body);

	node_set_arg(&nd, 0, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 1, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_do(node *const body, node *const cond, const location loc)
{
	node nd = node_insert(body, OP_DO, 2);
	node_set_child(&nd, cond);

	node_set_arg(&nd, 0, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 1, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_for(node *const init, node *const cond, node *const incr, node *const body, const location loc)
{
	node nd = node_insert(body, OP_FOR, 5);

	node_set_arg(&nd, 0, 0);
	node_set_arg(&nd, 1, 0);
	node_set_arg(&nd, 2, 0);

	if (init)
	{
		node_set_arg(&nd, 0, 1);
		node_set_child(&nd, init);
	}

	if (cond)
	{
		node_set_arg(&nd, 1, 1);
		node_set_child(&nd, cond);
	}

	if (incr)
	{
		node_set_arg(&nd, 2, 1);
		node_set_child(&nd, incr);
	}

	node_set_arg(&nd, 3, (item_t)loc.begin);		// Начальная позиция оператора
	node_set_arg(&nd, 4, (item_t)loc.end);			// Конечная позиция оператора

	return nd;
}

node statement_goto(node *const context, const size_t label, const location loc)
{
	node nd = node_create(context, OP_GOTO);

	node_add_arg(&nd, (item_t)label);				// ID метки
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция оператора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция оператора

	return nd;
}

node statement_continue(node *const context, const location loc)
{
	node nd = node_create(context, OP_CONTINUE);

	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция оператора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция оператора

	return nd;
}

node statement_break(node *const context, const location loc)
{
	node nd = node_create(context, OP_BREAK);

	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция оператора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция оператора

	return nd;
}

node statement_return(node *const context, node *const expr, const location loc)
{
	node nd = node_create(context, OP_RETURN);

	node_add_arg(&nd, 0);							// Содержит ли выражение
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция оператора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция оператора
	
	if (expr)
	{
		node_set_arg(&nd, 0, 1);
		node_set_child(&nd, expr);
	}

	return nd;
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
