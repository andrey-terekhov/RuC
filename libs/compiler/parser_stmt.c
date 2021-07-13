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
			return true;

		case TK_IDENTIFIER:
		{
			const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
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
	const size_t repr = prs->lxr->repr;
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
	const location_t case_location = token_consume(prs);
	node case_node = node_add_child(parent, OP_CASE);

	node_copy(&prs->nd, &case_node);
	const expression condition = parse_constant_expression(prs);
	if (!condition.is_valid)
	{
		token_skip_until(prs, TK_COLON | TK_R_BRACE);
		return;
	}

	token_expect_and_consume(prs, TK_COLON, expected_colon_after_case);
	parse_statement(prs, &case_node);

	if (!prs->is_in_switch)
	{
		semantics_error(prs, case_location, case_not_in_switch);
		return;
	}

	const item_t condition_type = node_get_arg(&condition.nd, 0);
	if (!type_is_integer(condition_type))
	{
		semantics_error(prs, condition.location, typecheck_switch_requires_integer);
	}

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
	const location_t default_location = token_consume(prs);
	token_expect_and_consume(prs, TK_COLON, expected_colon_after_default);

	node default_node = node_add_child(parent, OP_DEFAULT);
	parse_statement(prs, &default_node);

	if (!prs->is_in_switch)
	{
		semantics_error(prs, default_location, default_not_in_switch);
	}
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
	node_copy(&prs->nd, parent);
	expression expr = parse_expression(prs);

	if (!expr.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		return;
	}

	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_expr);
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
	token_consume(prs);
	node if_node = node_add_child(parent, OP_IF);
	node_add_arg(&if_node, false); // has_else

	if (prs->token != TK_L_PAREN)
	{
		parser_error(prs, expected_l_paren_after_if);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const location_t l_paren_location = token_consume(prs);

	node_copy(&prs->nd, &if_node);
	expression condition = parse_expression(prs);

	if (!condition.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		parser_error(prs, expected_r_paren, l_paren_location);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	parse_statement(prs, &if_node);

	const item_t condition_type = node_get_arg(&condition.nd, 0);
	if (!type_is_scalar(prs->sx, condition_type))
	{
		semantics_error(prs, condition.location, typecheck_statement_requires_scalar);
	}

	if (token_try_consume(prs, TK_ELSE))
	{
		node_set_arg(&if_node, 0, true);
		parse_statement(prs, &if_node);
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
	token_consume(prs);
	node switch_node = node_add_child(parent, OP_SWITCH);

	if (prs->token != TK_L_PAREN)
	{
		parser_error(prs, expected_l_paren_after_switch);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const location_t l_paren_location = token_consume(prs);

	node_copy(&prs->nd, &switch_node);
	expression condition = parse_expression(prs);

	if (!condition.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		parser_error(prs, expected_r_paren, l_paren_location);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const bool old_in_switch = prs->is_in_switch;
	prs->is_in_switch = true;
	parse_statement(prs, &switch_node);
	prs->is_in_switch = old_in_switch;

	const item_t condition_type = node_get_arg(&condition.nd, 0);
	if (!type_is_integer(condition_type))
	{
		semantics_error(prs, condition.location, typecheck_switch_requires_integer);
	}
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
	token_consume(prs);
	node while_node = node_add_child(parent, OP_WHILE);

	if (prs->token != TK_L_PAREN)
	{
		parser_error(prs, expected_l_paren_after_while);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const location_t l_paren_location = token_consume(prs);

	node_copy(&prs->nd, &while_node);
	expression condition = parse_expression(prs);

	if (!condition.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		parser_error(prs, expected_r_paren, l_paren_location);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &while_node);
	prs->is_in_loop = old_in_loop;

	const item_t condition_type = node_get_arg(&condition.nd, 0);
	if (!type_is_scalar(prs->sx, condition_type))
	{
		semantics_error(prs, condition.location, typecheck_statement_requires_scalar);
	}
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
	const location_t do_location = token_consume(prs);
	node do_node = node_add_child(parent, OP_DO);

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	parse_statement(prs, &do_node);
	prs->is_in_loop = old_in_loop;

	if (!token_try_consume(prs, TK_WHILE))
	{
		parser_error(prs, expected_while, do_location);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	if (prs->token != TK_L_PAREN)
	{
		parser_error(prs, expected_l_paren_after_while);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	node_copy(&prs->nd, &do_node);
	const location_t l_paren_location = token_consume(prs);
	expression condition = parse_expression(prs);

	if (!condition.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		parser_error(prs, expected_r_paren, l_paren_location);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);

	const item_t condition_type = node_get_arg(&condition.nd, 0);
	if (!type_is_scalar(prs->sx, condition_type))
	{
		semantics_error(prs, condition.location, typecheck_statement_requires_scalar);
	}
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
	token_consume(prs);
	node for_node = node_add_child(parent, OP_FOR);

	if (!token_try_consume(prs, TK_L_PAREN))
	{
		parser_error(prs, expected_l_paren_after_for);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	item_t old_displ;
	item_t old_lg;
	scope_block_enter(prs->sx, &old_displ, &old_lg);

	if (token_try_consume(prs, TK_SEMICOLON))
	{
		node_add_child(&for_node, OP_NOP);
	}
	else if (is_declaration_specifier(prs))
	{
		parse_declaration_inner(prs, &for_node);
	}
	else
	{
		node_copy(&prs->nd, &for_node);
		expression inition = parse_expression(prs);

		if (!inition.is_valid)
		{
			token_skip_until(prs, TK_SEMICOLON);
			return;
		}

		token_expect_and_consume(prs, TK_SEMICOLON, expected_semicolon_in_for);
	}

	if (token_try_consume(prs, TK_SEMICOLON))
	{
		node_add_child(&for_node, OP_NOP);
	}
	else
	{
		node_copy(&prs->nd, &for_node);
		expression condition = parse_expression(prs);

		if (!condition.is_valid)
		{
			token_skip_until(prs, TK_SEMICOLON);
			return;
		}

		const item_t condition_type = node_get_arg(&condition.nd, 0);
		if (!type_is_scalar(prs->sx, condition_type))
		{
			semantics_error(prs, condition.location, typecheck_statement_requires_scalar);
			token_skip_until(prs, TK_SEMICOLON);
			return;
		}

		token_expect_and_consume(prs, TK_SEMICOLON, expected_semicolon_in_for);
	}

	if (token_try_consume(prs, TK_R_PAREN))
	{
		node_add_child(&for_node, OP_NOP);
	}
	else
	{
		node_copy(&prs->nd, &for_node);
		expression increment = parse_expression(prs);

		if (!increment.is_valid)
		{
			token_skip_until(prs, TK_SEMICOLON);
			return;
		}

		if (!token_try_consume(prs, TK_R_PAREN))
		{
			parser_error(prs, expected_r_paren);
			return;
		}
	}

	const bool old_in_loop = prs->is_in_loop;
	prs->is_in_loop = true;
	if (prs->token == TK_L_BRACE)
	{
		parse_statement_compound(prs, &for_node, FORBLOCK);
	}
	else
	{
		parse_statement(prs, &for_node);
	}

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
	token_expect_and_consume(prs, TK_IDENTIFIER, expected_ident_after_goto);
	const size_t repr = prs->lxr->repr;

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
	const location_t continue_location = token_consume(prs);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);

	node_add_child(parent, OP_CONTINUE);

	if (!prs->is_in_loop)
	{
		semantics_error(prs, continue_location, continue_not_in_loop);
	}
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
	const location_t break_location = token_consume(prs);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);

	node_add_child(parent, OP_BREAK);

	if (!(prs->is_in_loop || prs->is_in_switch))
	{
		semantics_error(prs, break_location, break_not_in_loop_or_switch);
	}
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
	token_consume(prs);
	prs->nd = node_add_child(parent, OP_RETURN);
	prs->was_return = true;

	const item_t return_type = type_get(prs->sx, prs->function_mode + 1);

	if (token_try_consume(prs, TK_SEMICOLON))
	{
		if (!type_is_void(return_type))
		{
			parser_error(prs, no_ret_in_func);
		}

		return;
	}

	expression expr = parse_expression(prs);
	if (!expr.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON | TK_R_BRACE);
		return;
	}

	const item_t expr_type = node_get_arg(&expr.nd, 0);
	if (type_is_void(return_type))
	{
		parser_error(prs, notvoidret_in_void_func);
	}
	else if (return_type != expr_type
			 && !(type_is_floating(return_type) && type_is_integer(expr_type))
			 && return_type != TYPE_VOID_POINTER) // TODO: тут что-то не то
	{
		parser_error(prs, bad_type_in_ret);
	}

	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);
}

/**	Parse t_create_direct statement [RuC] */
static void parse_create_direct_statement(parser *const prs, node *const parent)
{
	node nd = node_add_child(parent, OP_THREAD);
	parse_statement_compound(prs, &nd, THREAD);
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
			const size_t repr = prs->lxr->repr;
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

	node node_print = node_add_child(parent, OP_PRINT);

	node_copy(&prs->nd, &node_print);
	const expression expr = parse_assignment_expression(prs);
	if (!expr.is_valid)
	{
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	token_expect_and_consume(prs, TK_R_PAREN, print_without_br);
	token_expect_and_consume(prs, TK_SEMICOLON, expected_semi_after_stmt);

	if (type_is_pointer(prs->sx, node_get_arg(&expr.nd, 0)))
	{
		semantics_error(prs, expr.location, pointer_in_print);
	}
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
			const size_t repr = prs->lxr->repr;
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

static size_t evaluate_args(parser *const prs, const size_t length, const char32_t *const format_str
	, item_t *const format_types, char32_t *const placeholders)
{
	size_t args = 0;
	for (size_t i = 0; i < length; i++)
	{
		if (format_str[i] == '%')
		{
			const char32_t placeholder = format_str[++i];
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
					format_types[args++] = TYPE_INTEGER;
					break;

				case 'c':
				case U'л':
					format_types[args++] = TYPE_CHARACTER;
					break;

				case 'f':
				case U'в':
					format_types[args++] = TYPE_FLOATING;
					break;

				case 's':
				case U'с':
					format_types[args++] = type_array(prs->sx, TYPE_CHARACTER);
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
	char32_t format_str[MAX_STRING_LENGTH + 1];
	item_t format_types[MAX_PRINTF_ARGS];

	token_expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_printf);
	node printf_node = node_add_child(parent, OP_PRINTF);

	if (prs->token != TK_STRING)
	{
		parser_error(prs, wrong_first_printf_param);
		token_skip_until(prs, TK_SEMICOLON);
		return;
	}

	const size_t format_length = (size_t)prs->lxr->num;
	for (size_t i = 0; i < format_length; i++)
	{
		format_str[i] = prs->lxr->lexstr[i];
	}
	format_str[format_length] = '\0';

	node_copy(&prs->nd, &printf_node);
	parse_assignment_expression(prs);

	size_t actual_args = 0;
	const size_t expected_args = evaluate_args(prs, format_length, format_str, format_types, placeholders);
	while (token_try_consume(prs, TK_COMMA) && actual_args != expected_args)
	{
		node_copy(&prs->nd, &printf_node);
		const expression expr = parse_assignment_expression(prs);
		const item_t type = node_get_arg(&expr.nd, 0);
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

		case TK_CREATE_DIRECT:
			parse_create_direct_statement(prs, parent);
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
			if (peek(prs->lxr) == TK_COLON)
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
	token_consume(prs); // '{' or kw_create_direct
	node nd_block = node_add_child(parent, OP_BLOCK);

	item_t old_displ = 0;
	item_t old_lg = 0;

	if (type != FUNCBODY && type != FORBLOCK)
	{
		scope_block_enter(prs->sx, &old_displ, &old_lg);
	}

	const token_t end_token = (type == THREAD) ? TK_EXIT_DIRECT : TK_R_BRACE;
	if (!token_try_consume(prs, end_token))
	{
		while (prs->token != TK_EOF && prs->token != end_token)
		{
			// Почему не ловилась ошибка, если в блоке нити встретилась '}'?
			if (is_declaration_specifier(prs))
			{
				parse_declaration_inner(prs, &nd_block);
			}
			else
			{
				parse_statement(prs, &nd_block);
			}
		}

		token_expect_and_consume(prs, end_token, expected_end);
	}

	if (type == FUNCBODY)
	{
		node_add_child(&nd_block, OP_RETURN);
	}
	else if (type != FORBLOCK)
	{
		scope_block_exit(prs->sx, old_displ, old_lg);
	}
}
