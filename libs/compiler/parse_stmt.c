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
 */
void parse_labeled_statement(parser *const context)
{
	int id;
	int i;
	int flag = 1;
	totree(context, TLabel);
	for (i = 0; flag && i < context->pgotost - 1; i += 2)
	{
		flag = context->sx->identab[context->gotost[i] + 1] != REPRTAB_POS;
	}
	if (flag)
	{
		totree(context, id = toidentab(context, 1, 0));
		if (context->was_error == 5)
		{
			context->was_error = 2;
		}
		else
		{
			context->gotost[context->pgotost++] = id; // это определение метки, если она встретилась до
													  // переходов на нее
			context->gotost[context->pgotost++] = -context->line;
		}
	}
	else
	{
		id = context->gotost[i - 2];
		REPRTAB_POS = context->sx->identab[id + 1];
		if (context->gotost[i - 1] < 0)
		{
			parser_error(context, repeated_label);
			context->was_error = 2;
		}
		else
		{
			context->gotost[i - 1] = -context->line;
		}
		totree(context, id);
	}

	if (context->was_error == 2)
	{
		context->was_error = 1;
	}
	else
	{
		context->sx->identab[id + 2] = 1;

		scanner(context);
		parse_statement(context);
	}
}

/**
 *	Parse case statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'case' constant-expression ':' statement
 */
void parse_case_statement(parser *const parser)
{
	totree(parser, TCase);

	if (!parser->flag_in_switch)
	{
		parser_error(parser, case_not_in_switch);
	}

	parse_constant_expression(parser);

	if (parser->ansttype == LFLOAT)
	{
		parser_error(parser, float_in_switch);
	}
	parser->sopnd--;

	try_consume_token(parser, colon, expected_colon_after_case);
	parse_statement(parser);
}

/**
 *	Parse default statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'default' ':' statement
 */
void parse_default_statement(parser *const parser)
{
	totree(parser, TDefault);

	if (!parser->flag_in_switch)
	{
		parser_error(parser, default_not_in_switch);
	}

	try_consume_token(parser, colon, expected_colon_after_default);
	parse_statement(parser);
}

/**
 *	Parse expression statement [C99 6.8.3]
 *
 *	expression-statement:
 *		expression ';'
 */
void parse_expression_statement(parser *const parser)
{
	parse_expression(parser);
	try_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse if statement [C99 6.8.4.1]
 *
 *	if-statement:
 *		'if' '(' expression ')' statement
 *		'if' '(' expression ')' statement 'else' statement
 */
void parse_if_statement(parser *const context)
{
	totree(context, TIf);
	size_t else_ref = context->sx->tc++;

	exprinbrkts(context, cond_must_be_in_brkts);
	context->sopnd--;
	parse_statement(context);

	if (context->next_token == LELSE)
	{
		scanner(context);
		context->sx->tree[else_ref] = (int)context->sx->tc;
		parse_statement(context);
	}
	else
	{
		context->sx->tree[else_ref] = 0;
	}
}

/**
 *	Parse switch statement [C99 6.8.4.2]
 *
 *	switch-statement:
 *		'switch' '(' expression ')' statement
 */
void parse_switch_statement(parser *const context)
{
	totree(context, TSwitch);

	exprinbrkts(context, cond_must_be_in_brkts);
	if (!is_int(context->ansttype))
	{
		parser_error(context, float_in_switch);
	}

	context->sopnd--;
	scanner(context);
	context->flag_in_switch = 1;
	parse_compound_statement(context, SWITCH);
}

/**
 *	Parse while statement [C99 6.8.5.1]
 *
 *	while-statement:
 *		'while' '(' expression ')' statement
 */
void parse_while_statement(parser *const parser)
{
	int old_in_loop = parser->flag_in_loop;
	totree(parser, TWhile);

	parser->flag_in_loop = 1;
	exprinbrkts(parser, cond_must_be_in_brkts);
	parse_statement(parser);
	parser->flag_in_loop = old_in_loop;
}

/**
 *	Parse do statement [C99 6.8.5.2]
 *
 *	do-statement:
 *		'do' statement 'while' '(' expression ')' ';'
 */
void parse_do_statement(parser *const parser)
{
	int old_in_loop = parser->flag_in_loop;
	totree(parser, TDo);

	parser->flag_in_loop = 1;
	parse_statement(parser);

	if (parser->next_token == LWHILE)
	{
		scanner(parser);
		exprinbrkts(parser, cond_must_be_in_brkts);
		parser->sopnd--;
	}
	else
	{
		parser_error(parser, expected_while);
		skip_until(parser, semicolon);
	}

	try_consume_token(parser, semicolon, expected_semi_after_stmt);
	parser->flag_in_loop = old_in_loop;
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 */
void parse_for_statement(parser *const context)
{
	int oldinloop = context->flag_in_loop;

	totree(context, TFor);
	size_t inition_ref = context->sx->tc++;
	size_t condition_ref = context->sx->tc++;
	size_t increment_ref = context->sx->tc++;
	size_t statement_ref = context->sx->tc++;

	mustbe(context, LEFTBR, no_leftbr_in_for);

	if (scanner(context) == SEMICOLON)
	{
		context->sx->tree[inition_ref] = 0;
	}
	else
	{
		context->sx->tree[inition_ref] = (int)context->sx->tc;
		// TODO: в С99 тут может быть объявление
		expr(context, 0);
		exprassnvoid(context);
		mustbe(context, SEMICOLON, no_semicolon_in_for);
	}

	if (scanner(context) == SEMICOLON)
	{
		context->sx->tree[condition_ref] = 0;
	}
	else
	{
		context->sx->tree[condition_ref] = (int)context->sx->tc;
		exprval(context);
		context->sopnd--;
		mustbe(context, SEMICOLON, no_semicolon_in_for);
		context->sopnd--;
	}

	if (scanner(context) == RIGHTBR)
	{
		context->sx->tree[increment_ref] = 0;
	}
	else
	{
		context->sx->tree[increment_ref] = (int)context->sx->tc;
		expr(context, 0);
		exprassnvoid(context);
		mustbe(context, RIGHTBR, no_rightbr_in_for);
	}

	context->sx->tree[statement_ref] = (int)context->sx->tc;
	context->flag_in_loop = 1;
	parse_statement(context);
	context->flag_in_loop = oldinloop;
}

/**
 *	Parse goto statement [C99 6.8.6.1]
 *
 *	jump-statement:
 *		'goto' identifier ';'
 */
void parse_goto_statement(parser *const context)
{
	int i;
	int flag = 1;
	totree(context, TGoto);

	mustbe(context, IDENT, no_ident_after_goto);
	for (i = 0; flag && i < context->pgotost - 1; i += 2)
	{
		flag = context->sx->identab[context->gotost[i] + 1] != REPRTAB_POS;
	}
	if (flag)
	{
		// первый раз встретился переход на метку, которой не было,
		// в этом случае ссылка на identtab, стоящая после TGoto,
		// будет отрицательной
		totree(context, -toidentab(context, 1, 0));
		context->gotost[context->pgotost++] = context->lastid;
	}
	else
	{
		int id = context->gotost[i - 2];
		if (context->gotost[id + 1] < 0) // метка уже была
		{
			totree(context, id);
			return;
		}
		totree(context, context->gotost[context->pgotost++] = id);
	}
	context->gotost[context->pgotost++] = context->line;
}

/**
 *	Parse continue statement [C99 6.8.6.2]
 *
 *	jump-statement:
 *		'continue' ';'
 */
void parse_continue_statement(parser *const parser)
{
	totree(parser, TContinue);

	if (!parser->flag_in_loop)
	{
		parser_error(parser, continue_not_in_loop);
	}

	try_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse break statement [C99 6.8.6.3]
 *
 *	jump-statement:
 *		'break' ';'
 */
void parse_break_statement(parser *const parser)
{
	totree(parser, TBreak);

	if (!(parser->flag_in_loop || parser->flag_in_switch))
	{
		parser_error(parser, break_not_in_loop_or_switch);
	}

	try_consume_token(parser, semicolon, expected_semi_after_stmt);
}

/**
 *	Parse return statement [C99 6.8.6.4]
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *		'return' braced-init-list ';'
 */
void parse_return_statement(parser *const context)
{
	int return_type = mode_get(context->sx, context->functype + 1);
	context->wasret = 1;
	if (context->next_token == SEMICOLON)
	{
		totree(context, TReturnvoid);
		scanner(context);
		if (return_type != LVOID)
			parser_error(context, no_ret_in_func);
	}
	else
	{
		if (return_type != LVOIDASTER)
		{
			if (return_type == LVOID)
			{
				parser_error(context, notvoidret_in_void_func);
			}
			totree(context, TReturnval);
			totree(context, szof(context, return_type));
			scanner(context);
			expr(context, 1);
			toval(context);
			context->sopnd--;

			if (return_type == LFLOAT && context->ansttype == LINT)
			{
				totree(context, WIDEN);
			}
			else if (return_type != context->ansttype)
			{
				parser_error(context, bad_type_in_ret);
			}
			totree(context, TExprend);
			mustbe(context, SEMICOLON, expected_semi_after_stmt);
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
void parse_printid_statement(parser *const context)
{
	mustbe(context, LEFTBR, no_leftbr_in_printid);

	while (context->next_token != RIGHTBR)
	{
		if (context->next_token == IDENT)
		{
			scanner(context);
			applid(context);
			totree(context, TPrintid);
			totree(context, context->lastid);
		}
		else
		{
			parser_error(context, no_ident_in_printid);
			skip_until(context, COMMA | RIGHTBR | SEMICOLON);
		}
		if (context->next_token == COMMA)
		{
			scanner(context);
		}
		else
		{
			break;
		}
	}

	mustbe(context, RIGHTBR, no_rightbr_in_printid);
	mustbe(context, SEMICOLON, expected_semi_after_stmt);

}

/**	Parse print statement [RuC] */
void parse_print_statement(parser *const context)
{
	exprassninbrkts(context, print_without_br);
	context->sx->tc--;
	totree(context, TPrint);
	totree(context, context->ansttype);
	totree(context, TExprend);

	if (is_pointer(context->sx, context->ansttype))
	{
		parser_error(context, pointer_in_print);
	}
	context->sopnd--;
	mustbe(context, SEMICOLON, expected_semi_after_stmt);
}

/**	Parse getid statement [RuC] */
void parse_getid_statement(parser *const context)
{
	mustbe(context, LEFTBR, no_leftbr_in_getid);

	while (context->next_token != RIGHTBR)
	{
		if (context->next_token == IDENT)
		{
			scanner(context);
			applid(context);
			totree(context, TGetid);
			totree(context, context->lastid);
		}
		else
		{
			parser_error(context, no_ident_in_getid);
			skip_until(context, COMMA | RIGHTBR | SEMICOLON);
		}
		if (context->next_token == COMMA)
		{
			scanner(context);
		}
		else
		{
			break;
		}
	}

	mustbe(context, RIGHTBR, no_rightbr_in_getid);
	mustbe(context, SEMICOLON, expected_semi_after_stmt);
}

int evaluate_params(parser *context, int num, char32_t formatstr[], int formattypes[], char32_t placeholders[])
{
	int num_of_params = 0;
	char32_t fsi;

	for (int i = 0; i < num; i++)
	{
		if (formatstr[i] == '%')
		{
			if (fsi = formatstr[++i], fsi != '%')
			{
				if (num_of_params == MAXPRINTFPARAMS)
				{
					parser_error(context, too_many_printf_params);
					return 0;
				}

				placeholders[num_of_params] = fsi;
			}
			switch (fsi) // Если добавляется новый спецификатор -- не забыть
						 // внести его в switch в bad_printf_placeholder
			{
				case 'i':
				case 1094: // 'ц'
					formattypes[num_of_params++] = LINT;
					break;

				case 'c':
				case 1083: // л
					formattypes[num_of_params++] = LCHAR;
					break;

				case 'f':
				case 1074: // в
					formattypes[num_of_params++] = LFLOAT;
					break;

				case 's':
				case 1089: // с
					formattypes[num_of_params++] = newdecl(context->sx, MARRAY, LCHAR);
					break;

				case '%':
					break;

				case 0:
					parser_error(context, printf_no_format_placeholder);
					return 0;

				default:
					context->bad_printf_placeholder = fsi;
					parser_error(context, printf_unknown_format_placeholder);
					return 0;
			}
		}
	}

	return num_of_params;
}

/**	Parse scanf statement [RuC] */
void parse_scanf_statement(parser *const context);

/**	Parse printf statement [RuC] */
void parse_printf_statement(parser *const context)
{
	char32_t formatstr[MAXSTRINGL + 1];
	int formattypes[MAXPRINTFPARAMS];
	char32_t placeholders[MAXPRINTFPARAMS];
	int sumsize = 0;

	mustbe(context, LEFTBR, no_leftbr_in_printf);

	if (context->next_token != STRING)
	{
		parser_error(context, wrong_first_printf_param);
		skip_until(context, SEMICOLON);
		return;
	}
	scanner(context);	// Для форматирующей строки

	for (int i = 0; i < context->lxr->num; i++)
	{
		formatstr[i] = context->lxr->lexstr[i];
	}
	formatstr[context->lxr->num] = 0;

	int expected_param_number = evaluate_params(context, context->lxr->num, formatstr, formattypes, placeholders);
	int actual_param_number = 0;
	//for (int i = 0; scanner(context) == COMMA; i++)
	while (context->next_token != RIGHTBR && actual_param_number != expected_param_number)
	{
		scanner(context);
		scanner(context);

		exprassn(context, 1);
		toval(context);
		totree(context, TExprend);

		if (formattypes[actual_param_number] == LFLOAT && context->ansttype == LINT)
		{
			insertwiden(context);
		}
		else if (formattypes[actual_param_number] != context->ansttype)
		{
			// FIXME: для этого заводится отдельное поле в analyzer
			context->bad_printf_placeholder = placeholders[actual_param_number];
			parser_error(context, wrong_printf_param_type);
		}

		sumsize += szof(context, formattypes[actual_param_number]);
		--context->sopnd;
		actual_param_number++;
		if (context->next_token != COMMA)
		{
			break;
		}
	}

	mustbe(context, RIGHTBR, no_rightbr_in_printf);

	if (actual_param_number != expected_param_number)
	{
		parser_error(context, wrong_printf_param_number);
	}

	totree(context, TString);
	totree(context, context->lxr->num);

	for (int i = 0; i < context->lxr->num; i++)
	{
		totree(context, formatstr[i]);
	}
	totree(context, TExprend);

	totree(context, TPrintf);
	totree(context, sumsize);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_statement(parser *const context)
{
	int oldinswitch = context->flag_in_switch;
	int oldinloop = context->flag_in_loop;

	scanner(context);
	if ((is_int(context->curr_token) || is_float(context->curr_token) || context->curr_token == LVOID ||
		 context->curr_token == LSTRUCT) &&
		context->blockflag)
	{
		parser_error(context, decl_after_strmt);
	}
	else
	{
		context->blockflag = 1;

		switch (context->curr_token)
		{
			case semicolon:
				totree(context, NOP);
				break;

			case kw_case:
				parse_case_statement(context);
				break;

			case kw_default:
				parse_default_statement(context);
				break;

			case l_brace:
				parse_compound_statement(context, REGBLOCK);
				break;

			case kw_if:
				parse_if_statement(context);
				break;

			case kw_switch:
				parse_switch_statement(context);
				break;

			case kw_while:
				parse_while_statement(context);
				break;

			case kw_do:
				parse_do_statement(context);
				break;

			case kw_for:
				parse_for_statement(context);
				break;

			case kw_goto:
				parse_goto_statement(context);
				break;

			case kw_continue:
				parse_continue_statement(context);
				break;

			case kw_break:
				parse_break_statement(context);
				break;

			case kw_return:
				parse_return_statement(context);
				break;

			case kw_t_create_direct:
				parse_create_direct_statement(context);
				break;

			case kw_printid:
				parse_printid_statement(context);
				break;

			case kw_printf:
				parse_printf_statement(context);
				break;

			case kw_print:
				parse_print_statement(context);
				break;

			case kw_getid:
				parse_getid_statement(context);
				break;

			case identifier:
				if (context->next_token == colon)
				{
					parse_labeled_statement(context);
					break;
				}

			default:
				parse_expression_statement(context);
		}
	}

	context->flag_in_switch = oldinswitch;
	context->flag_in_loop = oldinloop;
}

void parse_compound_statement(parser *const context, const block_type b)
{
	int oldinswitch = context->flag_in_switch;
	int notended = 1;
	int olddispl = 0;
	int oldlg = 0;
	int firstdecl;

	context->flag_in_switch = b < 0;
	totree(context, TBegin);
	if (b)
	{
		scope_block_enter(context->sx, &olddispl, &oldlg);
	}
	context->blockflag = 0;

	while (is_int(context->next_token) || is_float(context->next_token) || context->next_token == LSTRUCT ||
		   context->next_token == LVOID)
	{
		int repeat = 1;
		scanner(context);
		firstdecl = parse_type_specifier(context);
		if (context->was_error == 3)
		{
			context->was_error = 1;
			continue;
		}
		if (context->wasstructdef && context->next_token == SEMICOLON)
		{
			scanner(context);
			continue;
		}
		do
		{
			int temp = idorpnt(context, after_type_must_be_ident, firstdecl);

			if (context->was_error == after_type_must_be_ident)
			{
				context->was_error = 1;
				break;
			}

			decl_id(context, temp);
			if (context->was_error == 4)
			{
				context->was_error = 1;
				break;
			}
			if (context->next_token == COMMA)
			{
				scanner(context);
			}
			else if (context->next_token == SEMICOLON)
			{
				scanner(context);
				repeat = 0;
			}
			else
			{
				parser_error(context, def_must_end_with_semicomma);
				context->curr_token = SEMICOLON;
				repeat = 0;
			}
		} while (repeat);
	}

	// кончились описания, пошли операторы до }

	do
	{
		if (context->next_token == LEOF)
		{
			parser_error(context, wait_end);
			return;
		}
		if (b == 2 ? context->next_token == TEXIT : context->next_token == END)
		{
			scanner(context);
			notended = 0;
		}
		else
		{
			parse_statement(context);
			if (context->curr_token == LEOF && context->was_error)
			{
				return;
			}
		}
	} while (notended);

	if (b)
	{
		scope_block_exit(context->sx, olddispl, oldlg);
	}
	context->flag_in_switch = oldinswitch;
	totree(context, TEnd);
}
