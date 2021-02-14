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
	tree_add(prs->sx, TLabel);

	const size_t repr = prs->lxr->repr;
	// Не проверяем, что это ':', так как по нему узнали,
	// что это labeled statement
	consume_token(prs);
	for (size_t i = 0; i < prs->pgotost; i += 2)
	{
		if (repr == (size_t)ident_get_repr(prs->sx, (size_t)prs->gotost[i]))
		{
			const item_t id = prs->gotost[i];
			tree_add(prs->sx, id);

			if (prs->gotost[i + 1] < 0)
			{
				char buffer[MAXSTRINGL];
				repr_get_ident(prs->sx, repr, buffer);
				parser_error(prs, repeated_label, buffer);
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

	// Это определение метки, если она встретилась до
	// переходов на нее
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
	if (!prs->flag_in_switch)
	{
		parser_error(prs, case_not_in_switch);
	}

	tree_add(prs->sx, TCase);
	const item_t condition_type = parse_constant_expression(prs);
	if (!is_int(condition_type) && !is_undefined(condition_type))
	{
		parser_error(prs, float_in_switch);
	}

	expect_and_consume_token(prs, colon, expected_colon_after_case);
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
	if (!prs->flag_in_switch)
	{
		parser_error(prs, default_not_in_switch);
	}

	tree_add(prs->sx, TDefault);
	expect_and_consume_token(prs, colon, expected_colon_after_default);
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
	parse_expression(prs);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
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
	tree_add(prs->sx, TIf);
	const size_t ref_else = tree_reserve(prs->sx);

	parse_parenthesized_expression(prs);
	parse_statement(prs);

	if (try_consume_token(prs, kw_else))
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
	tree_add(prs->sx, TSwitch);

	const item_t condition_type = parse_parenthesized_expression(prs);
	if (!is_int(condition_type) && !is_undefined(condition_type))
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
	tree_add(prs->sx, TWhile);

	parse_parenthesized_expression(prs);

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
	tree_add(prs->sx, TDo);

	const int old_in_loop = prs->flag_in_loop;
	prs->flag_in_loop = 1;
	parse_statement(prs);
	prs->flag_in_loop = old_in_loop;

	if (try_consume_token(prs, kw_while))
	{
		parse_parenthesized_expression(prs);
	}
	else
	{
		parser_error(prs, expected_while);
		skip_until(prs, semicolon);
	}

	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 *
 *	@param	prs			Parser structure
 */
void parse_for_statement(parser *const prs)
{
	tree_add(prs->sx, TFor);

	const size_t ref_inition = tree_reserve(prs->sx);
	const size_t ref_condition = tree_reserve(prs->sx);
	const size_t ref_increment = tree_reserve(prs->sx);
	const size_t ref_statement = tree_reserve(prs->sx);
	expect_and_consume_token(prs, l_paren, no_leftbr_in_for);

	if (!try_consume_token(prs, semicolon))
	{
		tree_set(prs->sx, ref_inition, (item_t)tree_size(prs->sx));
		consume_token(prs);
		parse_expression(prs);
		expect_and_consume_token(prs, semicolon, no_semicolon_in_for);
	}

	if (!try_consume_token(prs, semicolon))
	{
		tree_set(prs->sx, ref_condition, (item_t)tree_size(prs->sx));
		parse_condition(prs);
		expect_and_consume_token(prs, semicolon, no_semicolon_in_for);
	}

	if (!try_consume_token(prs, r_paren))
	{
		tree_set(prs->sx, ref_increment, (item_t)tree_size(prs->sx));
		consume_token(prs);
		parse_expression(prs);
		expect_and_consume_token(prs, r_paren, no_rightbr_in_for);
	}

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
	tree_add(prs->sx, TGoto);
	expect_and_consume_token(prs, identifier, no_ident_after_goto);
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

			expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
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
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
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
	if (!prs->flag_in_loop)
	{
		parser_error(prs, continue_not_in_loop);
	}

	tree_add(prs->sx, TContinue);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
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
	if (!(prs->flag_in_loop || prs->flag_in_switch))
	{
		parser_error(prs, break_not_in_loop_or_switch);
	}

	tree_add(prs->sx, TBreak);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
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
	const item_t return_type = mode_get(prs->sx, prs->function_mode + 1);
	prs->flag_was_return = 1;

	if (try_consume_token(prs, semicolon))
	{
		tree_add(prs->sx, TReturnvoid);
		if (!is_void(return_type))
		{
			parser_error(prs, no_ret_in_func);
		}
	}
	else if (return_type != mode_void_pointer)
	{
		if (is_void(return_type))
		{
			parser_error(prs, notvoidret_in_void_func);
		}

		tree_add(prs->sx, TReturnval);
		tree_add(prs->sx, (item_t)size_of(prs->sx, return_type));

		consume_token(prs);
		const item_t expr_type = parse_assignment_expression(prs);
		if (!is_undefined(expr_type) && !is_undefined(return_type))
		{
			if (is_float(return_type) && is_int(expr_type))
			{
				insert_widen(prs);
			}
			else if (return_type != expr_type)
			{
				parser_error(prs, bad_type_in_ret);
			}
		}

		expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
	}
}

/**	Parse t_create_direct statement [RuC] */
void parse_create_direct_statement(parser *const prs)
{
	tree_add(prs->sx, CREATEDIRECTC);
	parse_compound_statement(prs, THREAD);
	tree_add(prs->sx, EXITDIRECTC);
}

/**	Parse printid statement [RuC] */
void parse_printid_statement(parser *const prs)
{
	expect_and_consume_token(prs, l_paren, no_leftbr_in_printid);

	do
	{
		if (try_consume_token(prs, identifier))
		{
			const size_t repr = prs->lxr->repr;
			const size_t id = (size_t)repr_get_reference(prs->sx, repr);
			if (id == 1)
			{
				char buffer[MAXSTRINGL];
				repr_get_ident(prs->sx, repr, buffer);
				parser_error(prs, ident_is_not_declared, buffer);
			}

			tree_add(prs->sx, TPrintid);
			tree_add(prs->sx, (item_t)id);
		}
		else
		{
			parser_error(prs, no_ident_in_printid);
			skip_until(prs, comma | r_paren | semicolon);
		}
	} while (try_consume_token(prs, comma));

	expect_and_consume_token(prs, r_paren, no_rightbr_in_printid);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);

}

/**	Parse print statement [RuC] */
void parse_print_statement(parser *const prs)
{
	expect_and_consume_token(prs, l_paren, print_without_br);
	consume_token(prs);

	const item_t type = parse_assignment_expression(prs);
	if (is_pointer(prs->sx, type))
	{
		parser_error(prs, pointer_in_print);
	}

	vector_remove(&prs->sx->tree);
	tree_add(prs->sx, TPrint);
	tree_add(prs->sx, type);
	tree_add(prs->sx, TExprend);

	expect_and_consume_token(prs, r_paren, print_without_br);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
}

/**	Parse getid statement [RuC] */
void parse_getid_statement(parser *const prs)
{
	expect_and_consume_token(prs, l_paren, no_leftbr_in_getid);

	do
	{
		if (try_consume_token(prs, identifier))
		{
			const size_t repr = prs->lxr->repr;
			const size_t id = (size_t)repr_get_reference(prs->sx, repr);
			if (id == 1)
			{
				char buffer[MAXSTRINGL];
				repr_get_ident(prs->sx, repr, buffer);
				parser_error(prs, ident_is_not_declared, buffer);
			}

			tree_add(prs->sx, TGetid);
			tree_add(prs->sx, (item_t)id);
		}
		else
		{
			parser_error(prs, no_ident_in_getid);
			skip_until(prs, comma | r_paren | semicolon);
		}
	} while (try_consume_token(prs, comma));

	expect_and_consume_token(prs, r_paren, no_rightbr_in_getid);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);
}

size_t evaluate_params(parser *const prs, const size_t length
					   , const char32_t *const formatstr, item_t *const formattypes, char32_t *const placeholders)
{
	size_t param_number = 0;
	for (size_t i = 0; i < length; i++)
	{
		if (formatstr[i] == '%')
		{
			const char32_t placeholder = formatstr[++i];
			if (placeholder != '%')
			{
				if (param_number == MAXPRINTFPARAMS)
				{
					parser_error(prs, too_many_printf_params);
					return 0;
				}

				placeholders[param_number] = placeholder;
			}
			switch (placeholder)
			{
				case 'i':
				case U'ц':
					formattypes[param_number++] = mode_integer;
					break;

				case 'c':
				case U'л':
					formattypes[param_number++] = mode_character;
					break;

				case 'f':
				case U'в':
					formattypes[param_number++] = mode_float;
					break;

				case 's':
				case U'с':
					formattypes[param_number++] = to_modetab(prs, mode_array, mode_character);
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

	return param_number;
}

/**	Parse scanf statement [RuC] */
void parse_scanf_statement(parser *const prs);

/**	Parse printf statement [RuC] */
void parse_printf_statement(parser *const prs)
{
	char32_t placeholders[MAXPRINTFPARAMS];
	char32_t format_str[MAXSTRINGL + 1];
	item_t formattypes[MAXPRINTFPARAMS];
	size_t sumsize = 0;

	expect_and_consume_token(prs, l_paren, no_leftbr_in_printf);

	if (prs->next_token != STRING)
	{
		parser_error(prs, wrong_first_printf_param);
		skip_until(prs, SEMICOLON);
		return;
	}

	const size_t format_str_length = (size_t)prs->lxr->num;
	for (size_t i = 0; i < format_str_length; i++)
	{
		format_str[i] = prs->lxr->lexstr[i];
	}
	format_str[format_str_length] = 0;
	consume_token(prs);	// Для форматирующей строки

	size_t actual_param_number = 0;
	const size_t expected_param_number = evaluate_params(prs, format_str_length, format_str, formattypes, placeholders);
	while (try_consume_token(prs, comma) && actual_param_number != expected_param_number)
	{
		consume_token(prs);
		const item_t type = parse_assignment_expression(prs);
		if (is_float(formattypes[actual_param_number]) && is_int(type))
		{
			insert_widen(prs);
		}
		else if (formattypes[actual_param_number] != type)
		{
			parser_error(prs, wrong_printf_param_type, placeholders[actual_param_number]);
		}

		sumsize += size_of(prs->sx, type);
		actual_param_number++;
	}

	expect_and_consume_token(prs, r_paren, no_rightbr_in_printf);
	expect_and_consume_token(prs, semicolon, expected_semi_after_stmt);

	if (actual_param_number != expected_param_number)
	{
		parser_error(prs, wrong_printf_param_number);
	}

	tree_add(prs->sx, TString);
	tree_add(prs->sx, (item_t)format_str_length);

	for (size_t i = 0; i < format_str_length; i++)
	{
		tree_add(prs->sx, format_str[i]);
	}
	tree_add(prs->sx, TExprend);

	tree_add(prs->sx, TPrintf);
	tree_add(prs->sx, (item_t)sumsize);
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
	switch (prs->next_token)
	{
		case kw_void:
		case kw_char:
			//case kw_short:
		case kw_int:
		case kw_long:
		case kw_float:
		case kw_double:
		case kw_struct:
			//case kw_union:
			//case kw_enum:
			//case kw_typedef:
			parse_inner_declaration(prs);
			return;

		case identifier:
		{
			const size_t id = (size_t)repr_get_reference(prs->sx, prs->lxr->repr);
			if (ident_get_displ(prs->sx, id) >= 1000)
			{
				parse_inner_declaration(prs);
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
	consume_token(prs);

	switch (prs->curr_token)
	{
		case semicolon:
			tree_add(prs->sx, NOP);
			break;

		case kw_case:
			parse_case_statement(prs);
			break;

		case kw_default:
			parse_default_statement(prs);
			break;

		case l_brace:
			parse_compound_statement(prs, REGBLOCK);
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
			if (prs->next_token == colon)
			{
				parse_labeled_statement(prs);
				break;
			}

		default:
			parse_expression_statement(prs);
			break;
	}
}

void parse_compound_statement(parser *const prs, const block_type type)
{
	tree_add(prs->sx, TBegin);

	item_t old_displ = 0;
	item_t old_lg = 0;

	if (type != FUNCBODY)
	{
		scope_block_enter(prs->sx, &old_displ, &old_lg);
	}

	const token_t end_token = (type == THREAD) ? kw_exit : r_brace;
	if (try_consume_token(prs, end_token))
	{
		// Если это пустой блок
		tree_add(prs->sx, NOP);
	}
	else
	{
		do
		{
			parse_block_item(prs);
			/* Почему не ловилась ошибка, если в блоке нити встретилась '}'? */
		} while (prs->next_token != eof && prs->next_token != end_token);

		expect_and_consume_token(prs, end_token, expected_end);
	}

	if (type != FUNCBODY)
	{
		scope_block_exit(prs->sx, old_displ, old_lg);
	}

	tree_add(prs->sx, TEnd);
}
