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
 *	@param	parser		Parser structure
 */
void parse_labeled_statement(parser *const parser)
{
	totree(parser, TLabel);

	const size_t repr = parser->lexer->repr;
	// Не проверяем, что это ':', так как по ней узнали,
	// что это labeled statement
	consume_token(parser);
	for (size_t i = 0; i < parser->pgotost; i += 2)
	{
		if (repr == (size_t)ident_get_repr(parser->sx, parser->gotost[i]))
		{
			const size_t id = (size_t)parser->gotost[i];
			const size_t repr = ident_get_repr(parser->sx, id);
			totree(parser, id);

			if (parser->gotost[i - 1] < 0)
			{
				parser_error(parser, repeated_label, parser->sx->reprtab, repr);
			}
			else
			{
				parser->gotost[i - 1] = -1;
			}

			ident_set_mode(parser->sx, id, 1);
			parse_statement(parser);
			return;
		}
	}

	// Это определение метки, если она встретилась до
	// переходов на нее
	const size_t id = to_identab(parser, repr, 1, 0);
	totree(parser, id);
	parser->gotost[parser->pgotost++] = id;
	parser->gotost[parser->pgotost++] = -1;	// TODO: здесь должен быть номер строки

	ident_set_mode(parser->sx, id, 1);
	parse_statement(parser);
}

/**
 *	Parse case statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 *
 *	@param	parser		Parser structure
 */
void parse_case_statement(parser *const parser)
{
	if (!parser->flag_in_switch)
	{
		parser_error(parser, case_not_in_switch);
	}

	totree(parser, TCase);
	const int condition_type = parse_constant_expression(parser);
	if (!is_int(condition_type) && !is_undefined(condition_type))
	{
		parser_error(parser, float_in_switch);
	}

	expect_and_consume_token(parser, colon, expected_colon_after_case);
	parse_statement(parser);
}

/**
 *	Parse default statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'default' ':' statement
 *
 *	@param	parser		Parser structure
 */
void parse_default_statement(parser *const parser)
{
	if (!parser->flag_in_switch)
	{
		parser_error(parser, default_not_in_switch);
	}

	totree(parser, TDefault);
	expect_and_consume_token(parser, colon, expected_colon_after_default);
	parse_statement(parser);
}

/**
 *	Parse expression statement [C99 6.8.3]
 *
 *	expression-statement:
 *		expression ';'
 *
 *	@param	parser		Parser structure
 */
void parse_expression_statement(parser *const parser)
{
	parse_expression(parser);
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse if statement [C99 6.8.4.1]
 *
 *	if-statement:
 *		'if' parenthesized-expression statement
 *		'if' parenthesized-expression statement 'else' statement
 *
 *	@param	parser		Parser structure
 */
void parse_if_statement(parser *const parser)
{
	totree(parser, TIf);
	const size_t ref_else = parser->sx->tc++;

	parse_parenthesized_expression(parser);
	parse_statement(parser);

	if (try_consume_token(parser, kw_else))
	{
		parser->sx->tree[ref_else] = (int)parser->sx->tc;
		parse_statement(parser);
	}
	else
	{
		parser->sx->tree[ref_else] = 0;
	}
}

/**
 *	Parse switch statement [C99 6.8.4.2]
 *
 *	switch-statement:
 *		'switch' parenthesized-expression statement
 *
 *	@param	parser		Parser structure
 */
void parse_switch_statement(parser *const parser)
{
	totree(parser, TSwitch);

	const int condition_type = parse_parenthesized_expression(parser);
	if (!is_int(condition_type) && !is_undefined(condition_type))
	{
		parser_error(parser, float_in_switch);
	}

	const int old_in_switch = parser->flag_in_switch;
	parser->flag_in_switch = 1;
	parse_statement(parser);
	parser->flag_in_switch = old_in_switch;
}

/**
 *	Parse while statement [C99 6.8.5.1]
 *
 *	while-statement:
 *		'while' parenthesized-expression statement
 *
 *	@param	parser		Parser structure
 */
void parse_while_statement(parser *const parser)
{
	totree(parser, TWhile);

	parse_parenthesized_expression(parser);

	const int old_in_loop = parser->flag_in_loop;
	parser->flag_in_loop = 1;
	parse_statement(parser);
	parser->flag_in_loop = old_in_loop;
}

/**
 *	Parse do statement [C99 6.8.5.2]
 *
 *	do-statement:
 *		'do' statement 'while' parenthesized-expression ';'
 *
 *	@param	parser		Parser structure
 */
void parse_do_statement(parser *const parser)
{
	totree(parser, TDo);

	const int old_in_loop = parser->flag_in_loop;
	parser->flag_in_loop = 1;
	parse_statement(parser);
	parser->flag_in_loop = old_in_loop;

	if (try_consume_token(parser, kw_while))
	{
		parse_parenthesized_expression(parser);
	}
	else
	{
		parser_error(parser, expected_while);
		skip_until(parser, semicolon);
	}

	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 *
 *	@param	parser		Parser structure
 */
void parse_for_statement(parser *const parser)
{
	totree(parser, TFor);

	const size_t ref_inition = parser->sx->tc++;
	const size_t ref_condition = parser->sx->tc++;
	const size_t ref_increment = parser->sx->tc++;
	const size_t ref_statement = parser->sx->tc++;
	expect_and_consume_token(parser, l_paren, no_leftbr_in_for);

	if (try_consume_token(parser, semicolon))
	{
		parser->sx->tree[ref_inition] = 0;
	}
	else
	{
		parser->sx->tree[ref_inition] = (int)parser->sx->tc;
		consume_token(parser);
		parse_expression(parser);
		expect_and_consume_token(parser, semicolon, no_semicolon_in_for);
	}

	if (try_consume_token(parser, semicolon))
	{
		parser->sx->tree[ref_condition] = 0;
	}
	else
	{
		parser->sx->tree[ref_condition] = (int)parser->sx->tc;
		parse_condition(parser);
		expect_and_consume_token(parser, semicolon, no_semicolon_in_for);
		parser->sopnd--;
	}

	if (try_consume_token(parser, r_paren))
	{
		parser->sx->tree[ref_increment] = 0;
	}
	else
	{
		parser->sx->tree[ref_increment] = (int)parser->sx->tc;
		consume_token(parser);
		parse_expression(parser);
		expect_and_consume_token(parser, r_paren, no_rightbr_in_for);
	}

	parser->sx->tree[ref_statement] = (int)parser->sx->tc;
	const int old_in_loop = parser->flag_in_loop;
	parser->flag_in_loop = 1;
	parse_statement(parser);
	parser->flag_in_loop = old_in_loop;
}

/**
 *	Parse goto statement [C99 6.8.6.1]
 *
 *	jump-statement:
 *		'goto' identifier ';'
 *
 *	@param	parser		Parser structure
 */
void parse_goto_statement(parser *const parser)
{
	totree(parser, TGoto);
	expect_and_consume_token(parser, identifier, no_ident_after_goto);
	const size_t repr = parser->lexer->repr;

	for (size_t i = 0; i < parser->pgotost; i += 2)
	{
		if (repr == (size_t)ident_get_repr(parser->sx, parser->gotost[i]))
		{
			const size_t id = (size_t)parser->gotost[i];
			totree(parser, id);
			if (parser->gotost[id + 1] >= 0) // Перехода на метку еще не было
			{
				parser->gotost[parser->pgotost++] = id;
				parser->gotost[parser->pgotost++] = 1; // TODO: здесь должен быть номер строки
			}

			expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
			return;
		}
	}

	// Первый раз встретился переход на метку, которой не было,
	// в этом случае ссылка на identtab, стоящая после TGoto,
	// будет отрицательной
	const size_t id = to_identab(parser, repr, 1, 0);
	totree(parser, -id);
	parser->gotost[parser->pgotost++] = id;
	parser->gotost[parser->pgotost++] = 1;	// TODO: здесь должен быть номер строки
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse continue statement [C99 6.8.6.2]
 *
 *	jump-statement:
 *		'continue' ';'
 *
 *	@param	parser		Parser structure
 */
void parse_continue_statement(parser *const parser)
{
	if (!parser->flag_in_loop)
	{
		parser_error(parser, continue_not_in_loop);
	}

	totree(parser, TContinue);
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse break statement [C99 6.8.6.3]
 *
 *	jump-statement:
 *		'break' ';'
 *
 *	@param	parser		Parser structure
 */
void parse_break_statement(parser *const parser)
{
	if (!(parser->flag_in_loop || parser->flag_in_switch))
	{
		parser_error(parser, break_not_in_loop_or_switch);
	}

	totree(parser, TBreak);
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse return statement [C99 6.8.6.4]
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *
 *	@param	parser		Parser structure
 */
void parse_return_statement(parser *const parser)
{
	const int return_type = mode_get(parser->sx, parser->function_type + 1);
	parser->flag_was_return = 1;
	
	if (try_consume_token(parser, semicolon))
	{
		totree(parser, TReturnvoid);
		if (!is_void(return_type))
		{
			parser_error(parser, no_ret_in_func);
		}
	}
	else
	{
		if (return_type != mode_void_pointer)
		{
			if (is_void(return_type))
			{
				parser_error(parser, notvoidret_in_void_func);
			}

			totree(parser, TReturnval);
			totree(parser, size_of(parser->sx, return_type));

			consume_token(parser);
			const int expr_type = parse_assignment_expression(parser);
			if (!is_undefined(expr_type) && !is_undefined(return_type))
			{
				if (is_float(return_type) && is_int(expr_type))
				{
					insertwiden(parser);
				}
				else if (return_type != expr_type)
				{
					parser_error(parser, bad_type_in_ret);
				}
			}
			expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
		}
	}
}

/**	Parse t_create_direct statement [RuC] */
void parse_create_direct_statement(parser *const parser)
{
	totree(parser, CREATEDIRECTC);
	parse_compound_statement(parser, THREAD);
	totree(parser, EXITC);
}

/**	Parse printid statement [RuC] */
void parse_printid_statement(parser *const parser)
{
	expect_and_consume_token(parser, l_paren, no_leftbr_in_printid);

	do
	{
		if (try_consume_token(parser, identifier))
		{
			const size_t id = repr_get_reference(parser->sx, parser->lexer->repr);
			if (id == 1)
			{
				parser_error(parser, ident_is_not_declared);
			}

			totree(parser, TPrintid);
			totree(parser, id);
		}
		else
		{
			parser_error(parser, no_ident_in_printid);
			skip_until(parser, comma | r_paren | semicolon);
		}
	} while (try_consume_token(parser, comma));

	expect_and_consume_token(parser, r_paren, no_rightbr_in_printid);
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);

}

/**	Parse print statement [RuC] */
void parse_print_statement(parser *const parser)
{
	expect_and_consume_token(parser, l_paren, print_without_br);
	consume_token(parser);
	const int type = parse_assignment_expression(parser);
	expect_and_consume_token(parser, r_paren, print_without_br);
	
	parser->sx->tc--;
	totree(parser, TPrint);
	totree(parser, type);
	totree(parser, TExprend);

	if (is_pointer(parser->sx, type))
	{
		parser_error(parser, pointer_in_print);
	}
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**	Parse getid statement [RuC] */
void parse_getid_statement(parser *const parser)
{
	expect_and_consume_token(parser, l_paren, no_leftbr_in_getid);

	do
	{
		if (try_consume_token(parser, identifier))
		{
			const size_t id = repr_get_reference(parser->sx, parser->lexer->repr);
			if (id == 1)
			{
				parser_error(parser, ident_is_not_declared);
			}

			totree(parser, TGetid);
			totree(parser, id);
		}
		else
		{
			parser_error(parser, no_ident_in_getid);
			skip_until(parser, comma | r_paren | semicolon);
		}
	} while (try_consume_token(parser, comma));

	expect_and_consume_token(parser, r_paren, no_rightbr_in_printid);
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);
}

size_t evaluate_params(parser *const parser, const size_t length
	, const char32_t *const formatstr, int *const formattypes, char32_t *const placeholders)
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
					parser_error(parser, too_many_printf_params);
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
					formattypes[param_number++] = to_modetab(parser->sx, mode_array, mode_character);
					break;

				case '%':
					break;

				case '\0':
					parser_error(parser, printf_no_format_placeholder);
					return 0;

				default:
					parser_error(parser, printf_unknown_format_placeholder, placeholder);
					return 0;
			}
		}
	}

	return param_number;
}

/**	Parse scanf statement [RuC] */
void parse_scanf_statement(parser *const parser);

/**	Parse printf statement [RuC] */
void parse_printf_statement(parser *const parser)
{
	char32_t placeholders[MAXPRINTFPARAMS];
	char32_t format_str[MAXSTRINGL + 1];
	int formattypes[MAXPRINTFPARAMS];
	int sumsize = 0;

	expect_and_consume_token(parser, l_paren, no_leftbr_in_printf);

	if (parser->next_token != STRING)
	{
		parser_error(parser, wrong_first_printf_param);
		skip_until(parser, SEMICOLON);
		return;
	}

	const size_t format_str_length = (size_t)parser->lexer->num;
	for (size_t i = 0; i < format_str_length; i++)
	{
		format_str[i] = parser->lexer->lexstr[i];
	}
	format_str[format_str_length] = 0;
	consume_token(parser);	// Для форматирующей строки

	size_t actual_param_number = 0;
	const size_t expected_param_number = evaluate_params(parser, format_str_length, format_str, formattypes, placeholders);
	while (try_consume_token(parser, comma) && actual_param_number != expected_param_number)
	{
		consume_token(parser);
		const int type = parse_assignment_expression(parser);
		if (is_float(formattypes[actual_param_number]) && is_int(type))
		{
			insertwiden(parser);
		}
		else if (formattypes[actual_param_number] != type)
		{
			parser_error(parser, wrong_printf_param_type, placeholders[actual_param_number]);
		}

		sumsize += size_of(parser->sx, type);
		actual_param_number++;
	}

	expect_and_consume_token(parser, r_paren, no_rightbr_in_printf);
	expect_and_consume_token(parser, semicolon, expected_semi_after_stmt);

	if (actual_param_number != expected_param_number)
	{
		parser_error(parser, wrong_printf_param_number);
	}

	totree(parser, TString);
	totree(parser, format_str_length);

	for (size_t i = 0; i < format_str_length; i++)
	{
		totree(parser, format_str[i]);
	}
	totree(parser, TExprend);

	totree(parser, TPrintf);
	totree(parser, sumsize);
}

/**
 *	Parse statement or declaration
 *	
 *	block-item:
 *		statement
 *		declaration
 *
 *	@param	parser		Parser structure
 */
void parse_block_item(parser *const parser)
{
	switch (parser->next_token)
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
			parse_inner_declaration(parser);
			return;

		case identifier:
		{
			const size_t id = repr_get_reference(parser->sx, parser->lexer->repr);
			if (ident_get_displ(parser->sx, id) >= 1000)
			{
				parse_inner_declaration(parser);
			}
			else
			{
				parse_statement(parser);
			}
			return;
		}

		default:
			parse_statement(parser);
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


void parse_statement(parser *const parser)
{
	consume_token(parser);
	
	switch (parser->curr_token)
	{
		case semicolon:
			totree(parser, NOP);
			break;

		case kw_case:
			parse_case_statement(parser);
			break;

		case kw_default:
			parse_default_statement(parser);
			break;

		case l_brace:
			parse_compound_statement(parser, REGBLOCK);
			break;

		case kw_if:
			parse_if_statement(parser);
			break;

		case kw_switch:
			parse_switch_statement(parser);
			break;

		case kw_while:
			parse_while_statement(parser);
			break;

		case kw_do:
			parse_do_statement(parser);
			break;

		case kw_for:
			parse_for_statement(parser);
			break;

		case kw_goto:
			parse_goto_statement(parser);
			break;

		case kw_continue:
			parse_continue_statement(parser);
			break;

		case kw_break:
			parse_break_statement(parser);
			break;

		case kw_return:
			parse_return_statement(parser);
			break;

		case kw_t_create_direct:
			parse_create_direct_statement(parser);
			break;

		case kw_printid:
			parse_printid_statement(parser);
			break;

		case kw_printf:
			parse_printf_statement(parser);
			break;

		case kw_print:
			parse_print_statement(parser);
			break;

		case kw_getid:
			parse_getid_statement(parser);
			break;

		case identifier:
			if (parser->next_token == colon)
			{
				parse_labeled_statement(parser);
				break;
			}

		default:
			parse_expression_statement(parser);
			break;
	}
}

void parse_compound_statement(parser *const parser, const block_type type)
{
	totree(parser, TBegin);

	int old_displ = 0;
	int old_lg = 0;

	if (type != FUNCBODY)
	{
		scope_block_enter(parser->sx, &old_displ, &old_lg);
	}

	const token end_token = (type == THREAD) ? kw_exit : r_brace;
	if (try_consume_token(parser, end_token))
	{
		// Если это пустой блок
		totree(parser, NOP);
	}
	else
	{
		do
		{
			parse_block_item(parser);
			/* Почему не ловилась ошибка, если в блоке нити встретилась '}'? */
		} while (parser->next_token != eof && parser->next_token != end_token);

		expect_and_consume_token(parser, end_token, expected_end);
	}

	if (type != FUNCBODY)
	{
		scope_block_exit(parser->sx, old_displ, old_lg);
	}
	totree(parser, TEnd);
}
