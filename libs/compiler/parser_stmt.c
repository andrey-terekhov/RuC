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

#include "parser.h"


#define MAX_PRINTF_ARGS 20


/** Check if current token is part of a declaration specifier */
static bool is_declaration_specifier(parser *const prs)
{
	switch (prs->token)
	{
		case TK_VOID:
		case TK_CHAR:
		case TK_INT:
		case TK_LONG:
		case TK_FLOAT:
		case TK_DOUBLE:
		case TK_STRUCT:
		case TK_FILE:
		case TK_TYPEDEF:
			return true;

		case TK_IDENTIFIER:
		{
			const item_t id = repr_get_reference(prs->sx, prs->lxr.repr);
			if (id == ITEM_MAX)
			{
				return false;
			}

			return ident_get_displ(prs->sx, (size_t)id) >= 1000;
		}

		default:
			return false;
	}
}


/**
 *	Parse labeled statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		identifier ':' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_labeled_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // identifier
	node nd = node_add_child(parent, OP_LABEL);
	const size_t repr = prs->lxr.repr;
	// Не проверяем, что это ':', так как по нему узнали, что это labeled statement
	token_consume(prs);
	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i)))
		{
			const item_t id = vector_get(&prs->labels, i);
			node_add_arg(&nd, id);

			if (vector_get(&prs->labels, i + 1) < 0)
			{
				parser_error(prs, repeated_label, repr_get_name(prs->sx, repr));
			}
			else
			{
				vector_set(&prs->labels, i + 1, -1);	// TODO: здесь должен быть номер строки
			}

			ident_set_type(prs->sx, (size_t)id, 1);
			parse_statement(prs, &nd);
			return;
		}
	}

	// Это определение метки, если она встретилась до переходов на нее
	const item_t id = (item_t)to_identab(prs, repr, 1, 0);
	node_add_arg(&nd, id);
	vector_add(&prs->labels, id);
	vector_add(&prs->labels, -1);	// TODO: здесь должен быть номер строки

	ident_set_type(prs->sx, (size_t)id, 1);
	parse_statement(prs, &nd);
}

/**
 *	Parse case statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_case_statement(parser *const prs, node *const parent)
{
	if (!prs->is_in_switch)
	{
		parser_error(prs, case_not_in_switch);
	}

	token_consume(prs); // kw_case
	node nd = node_add_child(parent, OP_CASE);
	node_copy(&prs->sx->nd, &nd);
	const node condition = parse_constant_expression(prs);
	const item_t condition_type = expression_get_type(&condition);
	if (node_is_correct(&condition) && !type_is_integer(condition_type))
	{
		parser_error(prs, float_in_switch);
	}

	token_expect_and_consume(prs, TK_COLON, expected_colon_after_case);
	parse_statement(prs, &nd);
}

/**
 *	Parse default statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'default' ':' statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_default_statement(parser *const prs, node *const parent)
{
	if (!prs->is_in_switch)
	{
		parser_error(prs, default_not_in_switch);
	}

	token_consume(prs); // kw_default
	node nd = node_add_child(parent, OP_DEFAULT);
	token_expect_and_consume(prs, TK_COLON, expected_colon_after_default);
	parse_statement(prs, &nd);
}

/**
 *	Parse expression statement [C99 6.8.3]
 *
 *	expression-statement:
 *		expression ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_expression_statement(parser *const prs, node *const parent)
{
	node_copy(&prs->sx->nd, parent);
	const node nd_expr = parse_expression(prs);
	if (!node_is_correct(&nd_expr))
	{
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse if statement [C99 6.8.4.1]
 *
 *	if-statement:
 *		'if' '(' expression ')' statement
 *		'if' '(' expression ')' statement 'else' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_if_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_if
	node nd = node_add_child(parent, OP_IF);
	node_add_arg(&nd, 0); // ref_else

	node_copy(&prs->sx->nd, &nd);
	token_expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	parse_expression(prs);
	token_expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	parse_statement(prs, &nd);

	if (token_try_consume(prs, TK_ELSE))
	{
		node_set_arg(&nd, 0, 1);
		parse_statement(prs, &nd);
	}
}

/**
 *	Parse switch statement [C99 6.8.4.2]
 *
 *	switch-statement:
 *		'switch' '(' expression ')' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_switch_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_switch
	node nd = node_add_child(parent, OP_SWITCH);

	node_copy(&prs->sx->nd, &nd);
	token_expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	const node condition = parse_expression(prs);
	token_expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	const item_t condition_type = expression_get_type(&condition);
	if (node_is_correct(&condition) && !type_is_integer(condition_type))
	{
		parser_error(prs, float_in_switch);
	}

	const bool old_in_switch = prs->is_in_switch;
	prs->is_in_switch = true;
	parse_statement(prs, &nd);
	prs->is_in_switch = old_in_switch;
}

/**
 *	Parse while statement [C99 6.8.5.1]
 *
 *	while-statement:
 *		'while' '(' expression ')' statement
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_while_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_while
	node nd = node_add_child(parent, OP_WHILE);

	node_copy(&prs->sx->nd, &nd);
	token_expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	parse_expression(prs);
	token_expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &nd);
	prs->is_in_loop = old_in_loop;
}

/**
 *	Parse do statement [C99 6.8.5.2]
 *
 *	do-statement:
 *		'do' statement 'while' '(' expression ')' ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_do_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_do
	node nd = node_add_child(parent, OP_DO);

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &nd);
	prs->is_in_loop = old_in_loop;

	if (token_try_consume(prs, TK_WHILE))
	{
		node_copy(&prs->sx->nd, &nd);
		token_expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
		parse_expression(prs);
		token_expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	}
	else
	{
		parser_error(prs, expected_while);
		token_skip_until(prs, TK_SEMICOLON);
	}

	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_for_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_for
	node nd = node_add_child(parent, OP_FOR);

	node_add_arg(&nd, 0); // ref_inition
	node_add_arg(&nd, 0); // ref_condition
	node_add_arg(&nd, 0); // ref_increment
	node_add_arg(&nd, 1); // ref_statement
	token_expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_for);

	item_t old_displ;
	item_t old_lg;
	scope_block_enter(prs->sx, &old_displ, &old_lg);
	if (!token_try_consume(prs, TK_SEMICOLON))
	{
		node_set_arg(&nd, 0, 1); // ref_inition
		if (is_declaration_specifier(prs))
		{
			parse_declaration_inner(prs, &nd);
		}
		else
		{
			node_copy(&prs->sx->nd, &nd);
			parse_expression(prs);
			token_expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_for);
		}
	}

	if (!token_try_consume(prs, TK_SEMICOLON))
	{
		node_set_arg(&nd, 1, 1); // ref_condition
		node_copy(&prs->sx->nd, &nd);
		parse_expression(prs);
		token_expect_and_consume(prs, TK_SEMICOLON, no_semicolon_in_for);
	}

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		node_set_arg(&nd, 2, 1); // ref_increment
		node_copy(&prs->sx->nd, &nd);
		parse_expression(prs);
		token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_for);
	}

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &nd);

	prs->is_in_loop = old_in_loop;
	scope_block_exit(prs->sx, old_displ, old_lg);
}

/**
 *	Parse goto statement [C99 6.8.6.1]
 *
 *	jump-statement:
 *		'goto' identifier ';'
 *
 *	@param	prs			Parser structure
 *	@param	parent		Parent node in AST
 */
static void parse_goto_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_goto
	node nd = node_add_child(parent, OP_GOTO);
	token_expect_and_consume(prs, TK_IDENTIFIER, no_ident_after_goto);
	const size_t repr = prs->lxr.repr;

	for (size_t i = 0; i < vector_size(&prs->labels); i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)vector_get(&prs->labels, i)))
		{
			const item_t id = vector_get(&prs->labels, i);
			node_add_arg(&nd, id);
			if (vector_get(&prs->labels, (size_t)id + 1) >= 0) // Перехода на метку еще не было
			{
				vector_add(&prs->labels, id);
				vector_add(&prs->labels, 1);	// TODO: здесь должен быть номер строки
			}

			token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
			return;
		}
	}

	// Первый раз встретился переход на метку, которой не было,
	// в этом случае ссылка на identtab, стоящая после TGoto,
	// будет отрицательной
	const item_t id = (item_t)to_identab(prs, repr, 1, 0);
	node_add_arg(&nd, -id);
	vector_add(&prs->labels, id);
	vector_add(&prs->labels, 1);	// TODO: здесь должен быть номер строки
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse continue statement [C99 6.8.6.2]
 *
 *	jump-statement:
 *		'continue' ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_continue_statement(parser *const prs, node *const parent)
{
	if (!prs->is_in_loop)
	{
		parser_error(prs, continue_not_in_loop);
	}

	token_consume(prs); // kw_continue
	node_add_child(parent, OP_CONTINUE);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse break statement [C99 6.8.6.3]
 *
 *	jump-statement:
 *		'break' ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_break_statement(parser *const prs, node *const parent)
{
	if (!(prs->is_in_loop || prs->is_in_switch))
	{
		parser_error(prs, break_not_in_loop_or_switch);
	}

	token_consume(prs); // kw_break
	node_add_child(parent, OP_BREAK);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**
 *	Parse return statement [C99 6.8.6.4]
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *
 *	@param	prs			Parser
 *	@param	parent		Parent node in AST
 */
static void parse_return_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_return
	const item_t return_type = type_get(prs->sx, prs->function_mode + 1);
	prs->was_return = true;

	node nd = node_add_child(parent, OP_RETURN);

	if (token_try_consume(prs, TK_SEMICOLON))
	{
		if (!type_is_void(return_type))
		{
			parser_error(prs, no_ret_in_func);
		}
	}
	else if (return_type != TYPE_VOID_POINTER)
	{
		if (type_is_void(return_type))
		{
			parser_error(prs, notvoidret_in_void_func);
		}

		node_copy(&prs->sx->nd, &nd);
		const node nd_expr = parse_assignment_expression(prs);
		const item_t expr_type = expression_get_type(&nd_expr);
		if (!type_is_undefined(expr_type) && !type_is_undefined(return_type))
		{
			if (return_type != expr_type && (!type_is_floating(return_type) || !type_is_integer(expr_type)))
			{
				parser_error(prs, bad_type_in_ret);
			}
		}

		token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
	}
}

/**	Parse printid statement [RuC] */
static void parse_printid_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_printid
	token_expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_printid);

	do
	{
		if (token_try_consume(prs, TK_IDENTIFIER))
		{
			const size_t repr = prs->lxr.repr;
			const item_t id = repr_get_reference(prs->sx, repr);
			if (id == ITEM_MAX)
			{
				parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
			}

			node nd = node_add_child(parent, OP_PRINTID);
			node_add_arg(&nd, id);
		}
		else
		{
			parser_error(prs, no_ident_in_printid);
			token_skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
		}
	} while (token_try_consume(prs, TK_COMMA));

	token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_printid);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**	Parse print statement [RuC] */
static void parse_print_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_print
	token_expect_and_consume(prs, TK_L_PAREN, print_without_br);

	node nd_print = node_add_child(parent, OP_PRINT);

	node_copy(&prs->sx->nd, &nd_print);
	const node nd_expr = parse_assignment_expression(prs);
	if (!node_is_correct(&nd_expr))
	{
		token_skip_until(prs, TK_SEMICOLON);
		token_try_consume(prs, TK_SEMICOLON);
		return;
	}
	
	const item_t type = expression_get_type(&nd_expr);
	if (type_is_pointer(prs->sx, type))
	{
		parser_error(prs, pointer_in_print);
	}

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		parser_error(prs, print_without_br);
		token_skip_until(prs, TK_SEMICOLON);
	}
	
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**	Parse getid statement [RuC] */
static void parse_getid_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_getid
	token_expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_getid);

	do
	{
		if (token_try_consume(prs, TK_IDENTIFIER))
		{
			const size_t repr = prs->lxr.repr;
			const item_t id = repr_get_reference(prs->sx, repr);
			if (id == ITEM_MAX)
			{
				parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
			}

			node nd = node_add_child(parent, OP_GETID);
			node_add_arg(&nd, id);
		}
		else
		{
			parser_error(prs, no_ident_in_getid);
			token_skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
		}
	} while (token_try_consume(prs, TK_COMMA));

	token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_getid);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

static size_t evaluate_args(parser *const prs, const vector *const format_str
	, item_t *const format_types, char32_t *const placeholders)
{
	size_t args = 0;
	const size_t length = vector_size(format_str);
	for (size_t i = 0; i < length; i++)
	{
		if (vector_get(format_str, i) == '%')
		{
			i++;
			const char32_t placeholder = (char32_t)vector_get(format_str, i);
			if (placeholder != '%')
			{
				if (args == MAX_PRINTF_ARGS)
				{
					parser_error(prs, too_many_printf_args, (size_t)MAX_PRINTF_ARGS);
					return 0;
				}

				placeholders[args] = placeholder;
			}
			switch (placeholder)
			{
				case 'i':
				case U'ц':
				case 'c':
				case U'л':
					format_types[args++] = TYPE_INTEGER;
					break;

				case 'f':
				case U'в':
					format_types[args++] = TYPE_FLOATING;
					break;

				case 's':
				case U'с':
					format_types[args++] = type_array(prs->sx, TYPE_INTEGER);
					break;

				case '%':
					break;

				case '\0':
					parser_error(prs, printf_no_format_placeholder);
					return 0;

				default:
					parser_error(prs, printf_unknown_format_placeholder, placeholder);
					return 0;
			}
		}
	}

	return args;
}

/**	Parse scanf statement [RuC] */
//static void parse_scanf_statement(parser *const prs, node *const parent);

/**	Parse printf statement [RuC] */
static void parse_printf_statement(parser *const prs, node *const parent)
{
	token_consume(prs); // kw_printf
	char32_t placeholders[MAX_PRINTF_ARGS];
	item_t format_types[MAX_PRINTF_ARGS];

	token_expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_printf);
	node nd_printf = node_add_child(parent, OP_PRINTF);

	if (prs->token != TK_STRING)
	{
		parser_error(prs, wrong_first_printf_param);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const size_t expected_args = evaluate_args(prs, &prs->lxr.lexstr, format_types, placeholders);

	node_copy(&prs->sx->nd, &nd_printf);
	parse_assignment_expression(prs);

	size_t actual_args = 0;
	while (token_try_consume(prs, TK_COMMA) && actual_args != expected_args)
	{
		node_copy(&prs->sx->nd, &nd_printf);
		const node nd_expr = parse_assignment_expression(prs);
		const item_t type = expression_get_type(&nd_expr);
		if (format_types[actual_args] != type && !(type_is_floating(format_types[actual_args]) && type_is_integer(type)))
		{
			parser_error(prs, wrong_printf_param_type, placeholders[actual_args]);
		}

		actual_args++;
	}

	token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_printf);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);

	if (actual_args != expected_args)
	{
		parser_error(prs, wrong_printf_param_number);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_statement(parser *const prs, node *const parent)
{
	switch (prs->token)
	{
		case TK_SEMICOLON:
			token_consume(prs);
			node_add_child(parent, OP_NOP);
			break;

		case TK_CASE:
			parse_case_statement(prs, parent);
			break;
		case TK_DEFAULT:
			parse_default_statement(prs, parent);
			break;

		case TK_L_BRACE:
			parse_statement_compound(prs, parent, REGBLOCK);
			break;

		case TK_IF:
			parse_if_statement(prs, parent);
			break;
		case TK_SWITCH:
			parse_switch_statement(prs, parent);
			break;

		case TK_WHILE:
			parse_while_statement(prs, parent);
			break;
		case TK_DO:
			parse_do_statement(prs, parent);
			break;
		case TK_FOR:
			parse_for_statement(prs, parent);
			break;

		case TK_GOTO:
			parse_goto_statement(prs, parent);
			break;
		case TK_CONTINUE:
			parse_continue_statement(prs, parent);
			break;
		case TK_BREAK:
			parse_break_statement(prs, parent);
			break;
		case TK_RETURN:
			parse_return_statement(prs, parent);
			break;

		case TK_PRINTID:
			parse_printid_statement(prs, parent);
			break;
		case TK_PRINTF:
			parse_printf_statement(prs, parent);
			break;
		case TK_PRINT:
			parse_print_statement(prs, parent);
			break;
		case TK_GETID:
			parse_getid_statement(prs, parent);
			break;

		case TK_IDENTIFIER:
			if (peek(&prs->lxr) == TK_COLON)
			{
				parse_labeled_statement(prs, parent);
				break;
			}

		default:
			parse_expression_statement(prs, parent);
			break;
	}
}

void parse_statement_compound(parser *const prs, node *const parent, const block_t type)
{
	token_consume(prs); // '{' 
	node nd_block = node_add_child(parent, OP_BLOCK);

	item_t old_displ = 0;
	item_t old_lg = 0;

	if (type != FUNCBODY)
	{
		scope_block_enter(prs->sx, &old_displ, &old_lg);
	}

	if (!token_try_consume(prs, TK_R_BRACE))
	{
		while (prs->token != TK_EOF && prs->token != TK_R_BRACE)
		{
			if (is_declaration_specifier(prs))
			{
				parse_declaration_inner(prs, &nd_block);
			}
			else
			{
				parse_statement(prs, &nd_block);
			}
		}

		token_expect_and_consume(prs, TK_R_BRACE, expected_end);
	}

	if (type != FUNCBODY)
	{
		scope_block_exit(prs->sx, old_displ, old_lg);
	}
}
