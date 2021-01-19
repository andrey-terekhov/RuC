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
void parse_labeled_statement(analyzer *const context)
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
		if (context->error_flag == 5)
		{
			context->error_flag = 2;
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
			context_error(context, repeated_label);
			context->error_flag = 2;
		}
		else
		{
			context->gotost[i - 1] = -context->line;
		}
		totree(context, id);
	}

	if (context->error_flag == 2)
	{
		context->error_flag = 1;
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
void parse_case_statement(analyzer *const context)
{
	totree(context, TCase);

	if (!context->inswitch)
	{
		context_error(context, case_not_in_switch);
	}
	else if (context->wasdefault)
	{
		// FIXME: C99 такого не запрещает
		context_error(context, case_after_default);
	}

	scanner(context);
	unarexpr(context);
	condexpr(context);
	toval(context);
	totree(context, TExprend);

	if (context->ansttype == LFLOAT)
	{
		context_error(context, float_in_switch);
	}
	context->sopnd--;

	mustbe(context, COLON, no_colon_in_case);
	parse_statement(context);
}

/**
 *	Parse default statement [C99 6.8.1]
 *
 *	labeled-statement:
 *		'default' ':' statement
 */
void parse_default_statement(analyzer *const context)
{
	totree(context, TDefault);
	context->wasdefault = 1;

	mustbe(context, COLON, no_colon_in_default);
	parse_statement(context);

	if (!context->inswitch)
		context_error(context, default_not_in_switch);
}

/**
 *	Parse expression statement [C99 6.8.3]
 *
 *	expression-statement:
 *		expression ';'
 */
void parse_expression_statement(analyzer *const context)
{
	expr(context, 0);
	exprassnvoid(context);
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse if statement [C99 6.8.4.1]
 *
 *	if-statement:
 *		'if' '(' expression ')' statement
 *		'if' '(' expression ')' statement 'else' statement
 */
void parse_if_statement(analyzer *const context)
{
	totree(context, TIf);
	size_t else_ref = context->sx->tc++;

	exprinbrkts(context, cond_must_be_in_brkts);
	context->sopnd--;
	parse_statement(context);

	if (context->next == LELSE)
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
void parse_switch_statement(analyzer *const context)
{
	totree(context, TSwitch);

	exprinbrkts(context, cond_must_be_in_brkts);
	if (!is_int(context->ansttype))
	{
		context_error(context, float_in_switch);
	}

	context->sopnd--;
	scanner(context);
	context->inswitch = 1;
	parse_compound_statement(context, SWITCH);
	context->wasdefault = 0;
}

/**
 *	Parse while statement [C99 6.8.5.1]
 *
 *	while-statement:
 *		'while' '(' expression ')' statement
 */
void parse_while_statement(analyzer *const context)
{
	int oldinloop = context->inloop;
	totree(context, TWhile);

	context->inloop = 1;
	exprinbrkts(context, cond_must_be_in_brkts);
	parse_statement(context);
	context->inloop = oldinloop;
}

/**
 *	Parse do statement [C99 6.8.5.2]
 *
 *	do-statement:
 *		'do' statement 'while' '(' expression ')' ';'
 */
void parse_do_statement(analyzer *const context)
{
	int oldinloop = context->inloop;
	totree(context, TDo);

	context->inloop = 1;
	parse_statement(context);
	if (context->next == LWHILE)
	{
		scanner(context);
		exprinbrkts(context, cond_must_be_in_brkts);
		context->sopnd--;
	}
	else
	{
		context_error(context, wait_while_in_do_stmt);
		skip_until(context, SEMICOLON);
	}

	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
	context->inloop = oldinloop;
}

/**
 *	Parse for statement [C99 6.8.5.3]
 *
 *	for-statement:
 *		'for' '(' expression[opt] ';' expression[opt] ';' expression[opt] ')' statement
 *		'for' '(' declaration expression[opt] ';' expression[opt] ')' statement
 */
void parse_for_statement(analyzer *const context)
{
	int oldinloop = context->inloop;

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
	context->inloop = 1;
	parse_statement(context);
	context->inloop = oldinloop;
}

/**
 *	Parse goto statement [C99 6.8.6.1]
 *
 *	jump-statement:
 *		'goto' identifier ';'
 */
void parse_goto_statement(analyzer *const context)
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
void parse_continue_statement(analyzer *const context)
{
	totree(context, TContinue);

	if (!context->inloop)
	{
		context_error(context, continue_not_in_loop);
	}

	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse break statement [C99 6.8.6.3]
 *
 *	jump-statement:
 *		'break' ';'
 */
void parse_break_statement(analyzer *const context)
{
	totree(context, TBreak);

	if (!(context->inloop || context->inswitch))
	{
		context_error(context, break_not_in_loop_or_switch);
	}

	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**
 *	Parse return statement [C99 6.8.6.4]
 *
 *	jump-statement:
 *		'return' expression[opt] ';'
 *		'return' braced-init-list ';'
 */
void parse_return_statement(analyzer *const context)
{
	int return_type = mode_get(context->sx, context->functype + 1);
	context->wasret = 1;
	if (context->next == SEMICOLON)
	{
		totree(context, TReturnvoid);
		scanner(context);
		if (return_type != LVOID)
			context_error(context, no_ret_in_func);
	}
	else
	{
		if (return_type != LVOIDASTER)
		{
			if (return_type == LVOID)
			{
				context_error(context, notvoidret_in_void_func);
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
				context_error(context, bad_type_in_ret);
			}
			totree(context, TExprend);
			mustbe(context, SEMICOLON, no_semicolon_after_stmt);
		}
	}
}

/**	Parse t_create_direct statement [RuC] */
void parse_create_direct_statement(analyzer *const context)
{
	totree(context, CREATEDIRECTC);
	parse_compound_statement(context, THREAD);
	totree(context, EXITC);
}

/**	Parse printid statement [RuC] */
void parse_printid_statement(analyzer *const context)
{
	mustbe(context, LEFTBR, no_leftbr_in_printid);

	while (context->next != RIGHTBR)
	{
		if (context->next == IDENT)
		{
			scanner(context);
			applid(context);
			totree(context, TPrintid);
			totree(context, context->lastid);
		}
		else
		{
			context_error(context, no_ident_in_printid);
			skip_until(context, COMMA | RIGHTBR | SEMICOLON);
		}
		if (context->next == COMMA)
		{
			scanner(context);
		}
		else
		{
			break;
		}
	}

	mustbe(context, RIGHTBR, no_rightbr_in_printid);
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);

}

/**	Parse printf statement [RuC] */
void parse_printf_statement(analyzer *const context)
{
	char32_t formatstr[MAXSTRINGL + 1];
	int formattypes[MAXPRINTFPARAMS];
	char32_t placeholders[MAXPRINTFPARAMS];
	int sumsize = 0;

	mustbe(context, LEFTBR, no_leftbr_in_printf);

	if (context->next != STRING)
	{
		context_error(context, wrong_first_printf_param);
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
	while (context->next != RIGHTBR && actual_param_number != expected_param_number)
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
			context_error(context, wrong_printf_param_type);
		}

		sumsize += szof(context, formattypes[actual_param_number]);
		--context->sopnd;
		actual_param_number++;
		if (context->next != COMMA)
		{
			break;
		}
	}

	mustbe(context, RIGHTBR, no_rightbr_in_printf);

	if (actual_param_number != expected_param_number)
	{
		context_error(context, wrong_printf_param_number);
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

/**	Parse print statement [RuC] */
void parse_print_statement(analyzer *const context)
{
	exprassninbrkts(context, print_without_br);
	context->sx->tc--;
	totree(context, TPrint);
	totree(context, context->ansttype);
	totree(context, TExprend);

	if (is_pointer(context->sx, context->ansttype))
	{
		context_error(context, pointer_in_print);
	}
	context->sopnd--;
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}

/**	Parse scanf statement [RuC] */
void parse_scanf_statement(analyzer *const context);

/**	Parse getid statement [RuC] */
void parse_getid_statement(analyzer *const context)
{
	mustbe(context, LEFTBR, no_leftbr_in_getid);

	while (context->next != RIGHTBR)
	{
		if (context->next == IDENT)
		{
			scanner(context);
			applid(context);
			totree(context, TGetid);
			totree(context, context->lastid);
		}
		else
		{
			context_error(context, no_ident_in_getid);
			skip_until(context, COMMA | RIGHTBR | SEMICOLON);
		}
		if (context->next == COMMA)
		{
			scanner(context);
		}
		else
		{
			break;
		}
	}

	mustbe(context, RIGHTBR, no_rightbr_in_getid);
	mustbe(context, SEMICOLON, no_semicolon_after_stmt);
}



/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void parse_statement(analyzer *const context)
{
	int oldwasdefault = context->wasdefault;
	int oldinswitch = context->inswitch;
	int oldinloop = context->inloop;

	context->wasdefault = 0;
	scanner(context);
	if ((is_int(context->cur) || is_float(context->cur) || context->cur == LVOID ||
		 context->cur == LSTRUCT) &&
		context->blockflag)
	{
		context_error(context, decl_after_strmt);
	}
	else
	{
		context->blockflag = 1;

		switch (context->cur)
		{
			case SEMICOLON:
				totree(context, NOP);
				break;

			case LCASE:
				parse_case_statement(context);
				break;

			case LDEFAULT:
				parse_default_statement(context);
				break;

			case BEGIN:
				parse_compound_statement(context, REGBLOCK);
				break;

			case LIF:
				parse_if_statement(context);
				break;

			case LSWITCH:
				parse_switch_statement(context);
				break;

			case LWHILE:
				parse_while_statement(context);
				break;

			case LDO:
				parse_do_statement(context);
				break;

			case LFOR:
				parse_for_statement(context);
				break;

			case LGOTO:
				parse_goto_statement(context);
				break;

			case LCONTINUE:
				parse_continue_statement(context);
				break;

			case LBREAK:
				parse_break_statement(context);
				break;

			case LRETURN:
				parse_return_statement(context);
				break;

			case TCREATEDIRECT:
				parse_create_direct_statement(context);
				break;

			case PRINTID:
				parse_printid_statement(context);
				break;

			case PRINTF:
				parse_printf_statement(context);
				break;

			case PRINT:
				parse_print_statement(context);
				break;

			case GETID:
				parse_getid_statement(context);
				break;

			case IDENT:
				if (context->next == COLON)
				{
					parse_labeled_statement(context);
					break;
				}

			default:
				parse_expression_statement(context);
		}
	}

	context->wasdefault = oldwasdefault;
	context->inswitch = oldinswitch;
	context->inloop = oldinloop;
}

void parse_compound_statement(analyzer *context, const block_type b)
{
	// если b=1, то это просто блок,
	// b = 2 - блок нити,
	// b = -1 - блок в switch, иначе
	// b = 0 - это блок функции

	int oldinswitch = context->inswitch;
	int notended = 1;
	int olddispl = 0;
	int oldlg = 0;
	int firstdecl;

	context->inswitch = b < 0;
	totree(context, TBegin);
	if (b)
	{
		scope_block_enter(context->sx, &olddispl, &oldlg);
	}
	context->blockflag = 0;

	while (is_int(context->next) || is_float(context->next) || context->next == LSTRUCT ||
		   context->next == LVOID)
	{
		int repeat = 1;
		scanner(context);
		firstdecl = gettype(context);
		if (context->error_flag == 3)
		{
			context->error_flag = 1;
			continue;
		}
		if (context->wasstructdef && context->next == SEMICOLON)
		{
			scanner(context);
			continue;
		}
		do
		{
			int temp = idorpnt(context, after_type_must_be_ident, firstdecl);

			if (context->error_flag == after_type_must_be_ident)
			{
				context->error_flag = 1;
				break;
			}

			decl_id(context, temp);
			if (context->error_flag == 4)
			{
				context->error_flag = 1;
				break;
			}
			if (context->next == COMMA)
			{
				scanner(context);
			}
			else if (context->next == SEMICOLON)
			{
				scanner(context);
				repeat = 0;
			}
			else
			{
				context_error(context, def_must_end_with_semicomma);
				context->cur = SEMICOLON;
				repeat = 0;
			}
		} while (repeat);
	}

	// кончились описания, пошли операторы до }

	do
	{
		if (context->next == LEOF)
		{
			context_error(context, wait_end);
			return;
		}
		if (b == 2 ? context->next == TEXIT : context->next == END)
		{
			scanner(context);
			notended = 0;
		}
		else
		{
			parse_statement(context);
			if (context->cur == LEOF && context->error_flag)
			{
				return;
			}
		}
	} while (notended);

	if (b)
	{
		scope_block_exit(context->sx, olddispl, oldlg);
	}
	context->inswitch = oldinswitch;
	totree(context, TEnd);
}
