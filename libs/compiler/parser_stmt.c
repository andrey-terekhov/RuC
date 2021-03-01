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


/**
 *	Parse labeled statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		identifier ':' statement
 *
 *	@param	prs			Parser structure
 */
void parse_labeled_statement(parser *const prs)
{
	token_consume(prs); // identifier
	tree_add(prs->sx, TLabel);

	const size_t repr = prs->lxr->repr;
	// Не проверяем, что это ':', так как по нему узнали, что это labeled statement
	token_consume(prs);
	for (size_t i = 0; i < prs->pgotost; i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)prs->gotost[i]))
		{
			const item_t id = prs->gotost[i];
			tree_add(prs->sx, id);

			if (prs->gotost[i + 1] < 0)
			{
				parser_error(prs, repeated_label, repr_get_name(prs->sx, repr));
			}
			else
			{
				prs->gotost[i + 1] = -1;	// TODO: здесь должен быть номер строки
			}

			ident_set_mode(prs->sx, (size_t)id, 1);
			parse_statement(prs);
			return;
		}
	}

	// Это определение метки, если она встретилась до переходов на нее
	const item_t id = (size_t)to_identab(prs, repr, 1, 0);
	tree_add(prs->sx, id);
	prs->gotost[prs->pgotost++] = id;
	prs->gotost[prs->pgotost++] = -1;	// TODO: здесь должен быть номер строки

	ident_set_mode(prs->sx, (size_t)id, 1);
	parse_statement(prs);
}

/**
 *	Parse case statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 *
 *	@param	prs			Parser structure
 */
void parse_case_statement(parser *const prs)
{
	token_consume(prs); // kw_case
	tree_add(prs->sx, TCase);

	if (!prs->flag_in_switch)
	{
		parser_error(prs, case_not_in_switch);
	}

	const expr_result expression = parse_constant_expression(prs);
	if (expression.was_error)
	{
		token_skip_until(prs, colon | semicolon | r_brace);
	}
	else if (!mode_is_int(expression.type) && !mode_is_undefined(expression.type))
	{
		parser_error(prs, float_in_switch);
	}

	token_expect_and_consume(prs, colon, expected_colon_after_case);

	if (prs->token == r_brace)
	{
		// switch (X) {... case 4: }
		parser_error(prs, label_end_of_compound_statement);
		return;
	}

	parse_statement(prs);
}

/**
 *	Parse default statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'default' ':' statement
 *
 *	@param	prs			Parser structure
 */
void parse_default_statement(parser *const prs)
{
	token_consume(prs); // kw_default
	tree_add(prs->sx, TDefault);

	if (!prs->flag_in_switch)
	{
		parser_error(prs, default_not_in_switch);
	}

	token_expect_and_consume(prs, colon, expected_colon_after_default);

	if (prs->token == r_brace)
	{
		// switch (X) {... default: }
		parser_error(prs, label_end_of_compound_statement);
		return;
	}

	parse_statement(prs);
}

/**
 *	Parse expression statement [C99 6.8.3]
 *
 *	expression-statement:
 *		expression ';'
 *
 *	@param	prs			Parser structure
 */
void parse_expression_statement(parser *const prs)
{
	const expr_result expression = parse_expression(prs);

	if (expression.was_error)
	{
		token_skip_until(prs, semicolon | r_brace);
	}
	else
	{
		token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
	}
}

/**
 *	Parse if statement [C99 6.8.4.1]
 *
 *	if-statement:
 *		'if' parenthesized-expression statement
 *		'if' parenthesized-expression statement 'else' statement
 *
 *	@param	prs			Parser structure
 */
void parse_if_statement(parser *const prs)
{
	token_consume(prs); // kw_if
	tree_add(prs->sx, TIf);
	const size_t ref_else = tree_reserve(prs->sx);

	if (!token_try_consume(prs, l_paren))
	{
		parser_error(prs, expected_lparen_after, kw_if);
		token_skip_until(prs, semicolon);
		return;
	}

	const expr_result condition = parse_condition(prs);

	if (condition.was_error)
	{
		token_skip_until(prs, semicolon);
		return;
	}
	else if (!token_try_consume(prs, r_paren))
	{
		parser_error(prs, expected_rparen_after, kw_if);
		prs->token == l_brace ? token_skip_until(prs, r_brace) : token_skip_until(prs, semicolon);
		return;
	}

	parse_statement(prs);
	if (token_try_consume(prs, kw_else))
	{
		tree_set(prs->sx, ref_else, (item_t)tree_size(prs->sx));
		parse_statement(prs);
	}
}

/**
 *	Parse switch statement [C99 6.8.4.2]
 *
 *	switch-statement:
 *		'switch' parenthesized-expression statement
 *
 *	@param	prs			Parser structure
 */
void parse_switch_statement(parser *const prs)
{
	token_consume(prs); // kw_switch
	tree_add(prs->sx, TSwitch);

	if (!token_try_consume(prs, l_paren))
	{
		parser_error(prs, expected_lparen_after, kw_switch);
		token_skip_until(prs, semicolon);
		return;
	}

	const expr_result condition = parse_condition(prs);

	if (condition.was_error)
	{
		token_skip_until(prs, semicolon);
		return;
	}
	else if (!token_try_consume(prs, r_paren))
	{
		parser_error(prs, expected_rparen_after, kw_switch);
		prs->token == l_brace ? token_skip_until(prs, r_brace) : token_skip_until(prs, semicolon);
		return;
	}
	else if (!mode_is_int(condition.type) && !mode_is_undefined(condition.type))
	{
		parser_error(prs, float_in_switch);
	}

	const int old_in_switch = prs->flag_in_switch;
	prs->flag_in_switch = 1;
	parse_statement(prs);
	prs->flag_in_switch = old_in_switch;
}

/**
 *	Parse while statement [C99 6.8.5.1]
 *
 *	while-statement:
 *		'while' parenthesized-expression statement
 *
 *	@param	prs			Parser structure
 */
void parse_while_statement(parser *const prs)
{
	token_consume(prs); // kw_while
	tree_add(prs->sx, TWhile);

	if (!token_try_consume(prs, l_paren))
	{
		parser_error(prs, expected_lparen_after, kw_while);
		token_skip_until(prs, semicolon);
		return;
	}

	const expr_result condition = parse_condition(prs);

	if (condition.was_error)
	{
		token_skip_until(prs, semicolon);
		return;
	}
	else if (!token_try_consume(prs, r_paren))
	{
		parser_error(prs, expected_rparen_after, kw_while);
		prs->token == l_brace ? token_skip_until(prs, r_brace) : token_skip_until(prs, semicolon);
		return;
	}

	const int old_in_loop = prs->flag_in_loop;
	prs->flag_in_loop = 1;
	parse_statement(prs);
	prs->flag_in_loop = old_in_loop;
}

/**
 *	Parse do statement [C99 6.8.5.2]
 *
 *	do-statement:
 *		'do' statement 'while' parenthesized-expression ';'
 *
 *	@param	prs			Parser structure
 */
void parse_do_statement(parser *const prs)
{
	token_consume(prs); // kw_do
	tree_add(prs->sx, TDo);

	const int old_in_loop = prs->flag_in_loop;
	prs->flag_in_loop = 1;
	parse_statement(prs);
	prs->flag_in_loop = old_in_loop;

	if (token_try_consume(prs, kw_while))
	{
		if (!token_try_consume(prs, l_paren))
		{
			parser_error(prs, expected_lparen_after, kw_while);
			token_skip_until(prs, semicolon);
			return;
		}

		const expr_result condition = parse_condition(prs);

		if (condition.was_error)
		{
			token_skip_until(prs, semicolon);
			return;
		}
		else if (!token_try_consume(prs, r_paren))
		{
			parser_error(prs, expected_rparen_after, kw_while);
			token_skip_until(prs, semicolon);
			return;
		}
	}
	else
	{
		parser_error(prs, expected_while);
		token_skip_until(prs, semicolon);
	}

	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement [TODO]
 *
 *	@param	prs			Parser structure
 */
void parse_for_statement(parser *const prs)
{
	token_consume(prs); // kw_for
	tree_add(prs->sx, TFor);

	if (!token_try_consume(prs, l_paren))
	{
		parser_error(prs, expected_lparen_after, kw_for);
		token_skip_until(prs, semicolon);
		return;
	}

	const size_t ref_inition = tree_reserve(prs->sx);
	const size_t ref_condition = tree_reserve(prs->sx);
	const size_t ref_increment = tree_reserve(prs->sx);
	const size_t ref_statement = tree_reserve(prs->sx);

	if (!token_try_consume(prs, semicolon))
	{
		const expr_result inition = parse_expression(prs);
		tree_set(prs->sx, ref_inition, (item_t)tree_size(prs->sx));
		if (inition.was_error)
		{
			token_skip_until(prs, r_paren | semicolon);
			token_try_consume(prs, semicolon);
		}
		else if (!token_try_consume(prs, semicolon))
		{
			parser_error(prs, expected_semi_for);
			token_skip_until(prs, r_paren | semicolon);
			token_try_consume(prs, semicolon);
		}
	}

	if (!token_try_consume(prs, semicolon))
	{
		const expr_result condition = parse_condition(prs);
		tree_set(prs->sx, ref_condition, (item_t)tree_size(prs->sx));
		if (condition.was_error)
		{
			token_skip_until(prs, r_paren | semicolon);
			token_try_consume(prs, semicolon);
		}
		else if (!token_try_consume(prs, semicolon))
		{
			parser_error(prs, expected_semi_for);
			token_skip_until(prs, r_paren | semicolon);
			token_try_consume(prs, semicolon);
		}
	}

	if (!token_try_consume(prs, r_paren))
	{
		const expr_result increment = parse_expression(prs);
		tree_set(prs->sx, ref_increment, (item_t)tree_size(prs->sx));
		if (increment.was_error)
		{
			token_skip_until(prs, r_paren | semicolon);
			token_try_consume(prs, semicolon);
		}
		else if (!token_try_consume(prs, r_paren))
		{
			parser_error(prs, no_rightbr_in_for);
			token_skip_until(prs, r_paren | semicolon);
			token_try_consume(prs, semicolon);
		}
	}

	// На тот случай, если при восстановлении остановились на ')'
	token_try_consume(prs, r_paren);

	tree_set(prs->sx, ref_statement, (item_t)tree_size(prs->sx));
	const int old_in_loop = prs->flag_in_loop;
	prs->flag_in_loop = 1;
	parse_statement(prs);
	prs->flag_in_loop = old_in_loop;
}

/**
 *	Parse goto statement [C99 6.8.6.1]
 *
 *	jump-statement:
 *		'goto' identifier ';'
 *
 *	@param	prs			Parser structure
 */
void parse_goto_statement(parser *const prs)
{
	token_consume(prs); // kw_goto
	tree_add(prs->sx, TGoto);
	token_expect_and_consume(prs, identifier, no_ident_after_goto);
	const size_t repr = prs->lxr->repr;

	for (size_t i = 0; i < prs->pgotost; i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)prs->gotost[i]))
		{
			const item_t id = prs->gotost[i];
			tree_add(prs->sx, id);
			if (prs->gotost[id + 1] >= 0) // Перехода на метку еще не было
			{
				prs->gotost[prs->pgotost++] = id;
				prs->gotost[prs->pgotost++] = 1; // TODO: здесь должен быть номер строки
			}

			token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
			return;
		}
	}

	// Первый раз встретился переход на метку, которой не было,
	// в этом случае ссылка на identtab, стоящая после TGoto,
	// будет отрицательной
	const item_t id = (item_t)to_identab(prs, repr, 1, 0);
	tree_add(prs->sx, -id);
	prs->gotost[prs->pgotost++] = id;
	prs->gotost[prs->pgotost++] = 1;	// TODO: здесь должен быть номер строки
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse continue statement [C99 6.8.6.2]
 *
 *	jump-statement:
 *		'continue' ';'
 *
 *	@param	prs			Parser structure
 */
void parse_continue_statement(parser *const prs)
{
	token_consume(prs); // kw_continue
	tree_add(prs->sx, TContinue);
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);

	if (!prs->flag_in_loop)
	{
		parser_error(prs, continue_not_in_loop);
	}
}

/**
 *	Parse break statement [C99 6.8.6.3]
 *
 *	jump-statement:
 *		'break' ';'
 *
 *	@param	prs			Parser structure
 */
void parse_break_statement(parser *const prs)
{
	token_consume(prs); // kw_break
	tree_add(prs->sx, TBreak);
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);

	if (!(prs->flag_in_loop || prs->flag_in_switch))
	{
		parser_error(prs, break_not_in_loop_or_switch);
	}
}

/**
 *	Parse return statement [C99 6.8.6.4]
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *
 *	@param	prs			Parser structure
 */
void parse_return_statement(parser *const prs)
{
	token_consume(prs); // kw_return
	const item_t return_type = mode_get(prs->sx, prs->function_mode + 1);
	prs->flag_was_return = 1;

	if (token_try_consume(prs, semicolon))
	{
		tree_add(prs->sx, TReturnvoid);
		if (!mode_is_void(return_type))
		{
			parser_error(prs, no_ret_in_func);
		}
	}
	else if (return_type != mode_void_pointer)
	{
		if (mode_is_void(return_type))
		{
			parser_error(prs, notvoidret_in_void_func);
		}

		tree_add(prs->sx, TReturnval);
		tree_add(prs->sx, (item_t)size_of(prs->sx, return_type));

		const expr_result expression = parse_assignment_expression(prs);
		if (expression.was_error)
		{
			token_skip_until(prs, semicolon | r_brace);
			return;
		}
		if (!mode_is_undefined(expression.type) && !mode_is_undefined(return_type))
		{
			if (mode_is_float(return_type) && mode_is_int(expression.type))
			{
				parse_insert_widen(prs);
			}
			else if (return_type != expression.type)
			{
				parser_error(prs, bad_type_in_ret);
			}
		}

		token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
	}
}

/**	Parse t_create_direct statement [RuC] */
void parse_create_direct_statement(parser *const prs)
{
	tree_add(prs->sx, CREATEDIRECTC);
	parse_statement_compound(prs, THREAD);
	tree_add(prs->sx, EXITDIRECTC);
}

/**	Parse printid statement [RuC] */
void parse_printid_statement(parser *const prs)
{
	token_consume(prs); // kw_printid
	token_expect_and_consume(prs, l_paren, no_leftbr_in_printid);

	do
	{
		if (token_try_consume(prs, identifier))
		{
			const size_t repr = prs->lxr->repr;
			const item_t id = repr_get_reference(prs->sx, repr);
			if (id == ITEM_MAX)
			{
				parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
			}

			tree_add(prs->sx, TPrintid);
			tree_add(prs->sx, id);
		}
		else
		{
			parser_error(prs, no_ident_in_printid);
			token_skip_until(prs, comma | r_paren | semicolon);
		}
	} while (token_try_consume(prs, comma));

	token_expect_and_consume(prs, r_paren, no_rightbr_in_printid);
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
}

/**	Parse print statement [RuC] */
void parse_print_statement(parser *const prs)
{
	token_consume(prs); // kw_print
	token_expect_and_consume(prs, l_paren, print_without_br);

	const expr_result expression = parse_assignment_expression(prs);
	if (mode_is_pointer(prs->sx, expression.type))
	{
		parser_error(prs, pointer_in_print);
	}

	vector_remove(&prs->sx->tree);
	tree_add(prs->sx, TPrint);
	tree_add(prs->sx, expression.type);
	tree_add(prs->sx, TExprend);

	token_expect_and_consume(prs, r_paren, print_without_br);
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
}

/**	Parse getid statement [RuC] */
void parse_getid_statement(parser *const prs)
{
	token_consume(prs); // kw_getid
	token_expect_and_consume(prs, l_paren, no_leftbr_in_getid);

	do
	{
		if (token_try_consume(prs, identifier))
		{
			const size_t repr = prs->lxr->repr;
			const item_t id = repr_get_reference(prs->sx, repr);
			if (id == ITEM_MAX)
			{
				parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, repr));
			}

			tree_add(prs->sx, TGetid);
			tree_add(prs->sx, id);
		}
		else
		{
			parser_error(prs, no_ident_in_getid);
			token_skip_until(prs, comma | r_paren | semicolon);
		}
	} while (token_try_consume(prs, comma));

	token_expect_and_consume(prs, r_paren, no_rightbr_in_getid);
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);
}

size_t evaluate_args(parser *const prs, const size_t length, const char32_t *const format_str
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
				if (args == MAXPRINTFPARAMS)
				{
					parser_error(prs, too_many_printf_params);
					return 0;
				}

				placeholders[args] = placeholder;
			}
			switch (placeholder)
			{
				case 'i':
				case U'ц':
					format_types[args++] = mode_integer;
					break;

				case 'c':
				case U'л':
					format_types[args++] = mode_character;
					break;

				case 'f':
				case U'в':
					format_types[args++] = mode_float;
					break;

				case 's':
				case U'с':
					format_types[args++] = to_modetab(prs, mode_array, mode_character);
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
void parse_scanf_statement(parser *const prs);

/**	Parse printf statement [RuC] */
void parse_printf_statement(parser *const prs)
{
	token_consume(prs); // kw_printf
	char32_t placeholders[MAXPRINTFPARAMS];
	char32_t format_str[MAXSTRINGL + 1];
	item_t format_types[MAXPRINTFPARAMS];
	size_t sum_size = 0;

	token_expect_and_consume(prs, l_paren, no_leftbr_in_printf);

	if (prs->token != STRING)
	{
		parser_error(prs, wrong_first_printf_param);
		token_skip_until(prs, SEMICOLON);
		return;
	}

	const size_t format_length = (size_t)prs->lxr->num;
	for (size_t i = 0; i < format_length; i++)
	{
		format_str[i] = prs->lxr->lexstr[i];
	}
	format_str[format_length] = '\0';
	token_consume(prs);	// Для форматирующей строки

	size_t actual_args = 0;
	const size_t expected_args = evaluate_args(prs, format_length, format_str, format_types, placeholders);
	while (token_try_consume(prs, comma) && actual_args != expected_args)
	{
		const expr_result expression = parse_assignment_expression(prs);
		if (mode_is_float(format_types[actual_args]) && mode_is_int(expression.type))
		{
			parse_insert_widen(prs);
		}
		else if (format_types[actual_args] != expression.type)
		{
			parser_error(prs, wrong_printf_param_type, placeholders[actual_args]);
		}

		sum_size += size_of(prs->sx, expression.type);
		actual_args++;
	}

	token_expect_and_consume(prs, r_paren, no_rightbr_in_printf);
	token_expect_and_consume(prs, semicolon, expected_semi_after_stmt);

	if (actual_args != expected_args)
	{
		parser_error(prs, wrong_printf_param_number);
	}

	tree_add(prs->sx, TString);
	tree_add(prs->sx, (item_t)format_length);

	for (size_t i = 0; i < format_length; i++)
	{
		tree_add(prs->sx, format_str[i]);
	}
	tree_add(prs->sx, TExprend);

	tree_add(prs->sx, TPrintf);
	tree_add(prs->sx, (item_t)sum_size);
}

/**
 *	Parse statement or declaration
 *
 *	block-item:
 *		statement
 *		declaration
 *
 *	@param	prs			Parser structure
 */
void parse_block_item(parser *const prs)
{
	switch (prs->token)
	{
		case kw_void:
		case kw_char:
		// case kw_short:
		case kw_int:
		case kw_long:
		case kw_float:
		case kw_double:
		case kw_struct:
		// case kw_union:
		// case kw_enum:
		// case kw_typedef:
			parse_declaration_inner(prs);
			return;

		case identifier:
		{
			const item_t ref = repr_get_reference(prs->sx, prs->lxr->repr);
			const size_t id = ref == ITEM_MAX ? 1 : (size_t)ref;
			if (ident_get_displ(prs->sx, id) >= 1000)
			{
				parse_declaration_inner(prs);
			}
			else
			{
				parse_statement(prs);
			}
			return;
		}

		default:
			parse_statement(prs);
			return;
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_statement(parser *const prs)
{
	switch (prs->token)
	{
		case semicolon:
			token_consume(prs);
			tree_add(prs->sx, NOP);
			break;

		case kw_case:
			parse_case_statement(prs);
			break;
		case kw_default:
			parse_default_statement(prs);
			break;

		case l_brace:
			parse_statement_compound(prs, REGBLOCK);
			break;

		case kw_if:
			parse_if_statement(prs);
			break;
		case kw_switch:
			parse_switch_statement(prs);
			break;

		case kw_while:
			parse_while_statement(prs);
			break;
		case kw_do:
			parse_do_statement(prs);
			break;
		case kw_for:
			parse_for_statement(prs);
			break;

		case kw_goto:
			parse_goto_statement(prs);
			break;
		case kw_continue:
			parse_continue_statement(prs);
			break;
		case kw_break:
			parse_break_statement(prs);
			break;
		case kw_return:
			parse_return_statement(prs);
			break;

		case kw_t_create_direct:
			parse_create_direct_statement(prs);
			break;

		case kw_printid:
			parse_printid_statement(prs);
			break;
		case kw_printf:
			parse_printf_statement(prs);
			break;
		case kw_print:
			parse_print_statement(prs);
			break;
		case kw_getid:
			parse_getid_statement(prs);
			break;

		case identifier:
			if (token_peek(prs) == colon)
			{
				parse_labeled_statement(prs);
				break;
			}

		default:
			parse_expression_statement(prs);
			break;
	}
}

void parse_statement_compound(parser *const prs, const block_t type)
{
	token_consume(prs); // '{' or kw_create_direct
	tree_add(prs->sx, TBegin);

	item_t old_displ = 0;
	item_t old_lg = 0;

	if (type != FUNCBODY)
	{
		scope_block_enter(prs->sx, &old_displ, &old_lg);
	}

	const token_t end_token = (type == THREAD) ? kw_exit : r_brace;
	while (prs->token != eof && prs->token != end_token)
	{
		parse_block_item(prs);
		// Почему не ловилась ошибка, если в блоке нити встретилась '}'?
	}

	token_expect_and_consume(prs, end_token, expected_end);

	if (type != FUNCBODY)
	{
		scope_block_exit(prs->sx, old_displ, old_lg);
	}
	else
	{
		tree_add(prs->sx, TReturnvoid);
	}

	tree_add(prs->sx, TEnd);
}
