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

#include "extdecl.h"


void statement(analyzer *context)
{
	int flagsemicol = 1;
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
		flagsemicol = 0;
	}
	if (context->cur == BEGIN)
	{
		flagsemicol = 0;
		block(context, 1);
	}
	else if (context->cur == TCREATEDIRECT)
	{
		totree(context, CREATEDIRECTC);
		flagsemicol = 0;
		block(context, 2);
		totree(context, EXITDIRECTC);
	}
	else if (context->cur == SEMICOLON)
	{
		totree(context, NOP);
		flagsemicol = 0;
	}
	else if (context->cur == IDENT && context->next == COLON)
	{
		int id;
		int i;
		int flag = 1;
		flagsemicol = 0;
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
			ident_set_mode(context->sx, id, 1);

			scanner(context);
			statement(context);
		}
	}
	else
	{
		context->blockflag = 1;

		// And here too
		switch (context->cur)
		{
			case PRINT:
			{
				exprassninbrkts(context, print_without_br);
				if (context->error_flag == 3)
				{
					context->error_flag = 1;
					flagsemicol = 0;
					break;
				}

				if (context->sx->tc != 0)
				{
					context->sx->tc--;
				}
				totree(context, TPrint);
				totree(context, context->ansttype);
				totree(context, TExprend);
				if (is_pointer(context->sx, context->ansttype))
				{
					context_error(context, pointer_in_print);
					flagsemicol = 0;
				}
				context->sopnd--;
			}
				break;
			case PRINTID:
			{
				mustbe(context, LEFTBR, no_leftbr_in_printid);
				do
				{
					mustbe(context, IDENT, no_ident_in_printid);
					context->lastid = REPRTAB[REPRTAB_POS + 1];
					if (context->lastid == 1)
					{
						context_error(context, ident_is_not_declared);
						context->wasdefault = oldwasdefault;
						context->inswitch = oldinswitch;
						context->inloop = oldinloop;
						return;
					}
					totree(context, TPrintid);
					totree(context, context->lastid);
				} while (context->next == COMMA ? scanner(context), 1 : 0);
				mustbe(context, RIGHTBR, no_rightbr_in_printid);
			}
				break;

			case PRINTF:
			{
				char32_t formatstr[MAXSTRINGL + 1];
				int formattypes[MAXPRINTFPARAMS];
				char32_t placeholders[MAXPRINTFPARAMS];
				int paramnum = 0;
				int sumsize = 0;
				int i = 0;
				int fnum;

				mustbe(context, LEFTBR, no_leftbr_in_printf);
				if (scanner(context) != STRING) // выкушиваем форматную строку
				{
					context_error(context, wrong_first_printf_param);
					break;
				}

				for (i = 0; i < context->lxr->num; i++)
				{
					formatstr[i] = context->lxr->lexstr[i];
				}
				formatstr[context->lxr->num] = 0;

				paramnum = evaluate_params(context, fnum = context->lxr->num, formatstr, formattypes, placeholders);

				if (context->error_flag)
				{
					flagsemicol = 0;
					break;
				}

				for (i = 0; scanner(context) == COMMA; i++)
				{
					if (i >= paramnum)
					{
						context_error(context, wrong_printf_param_number);
						context->error_flag = 2;
						break;
					}

					scanner(context);

					exprassn(context, 1);
					if (context->error_flag == 6)
					{
						context->error_flag = 2;
						break;
					}
					toval(context);
					totree(context, TExprend);

					if (formattypes[i] == LFLOAT && context->ansttype == LINT)
					{
						insertwiden(context);
					}
					else if (formattypes[i] != context->ansttype)
					{
						context->bad_printf_placeholder = placeholders[i];
						context_error(context, wrong_printf_param_type);
						context->error_flag = 2;
						break;
					}

					sumsize += szof(context, formattypes[i]);
					--context->sopnd;
				}
				if (context->error_flag == 2)
				{
					flagsemicol = 0;
					break;
				}

				if (context->cur != RIGHTBR)
				{
					context_error(context, no_rightbr_in_printf);
					context->buf_cur = context->next;
					context->next = context->cur;
					context->cur = RIGHTBR;
					context->buf_flag++;
					break;
				}

				if (i != paramnum)
				{
					context_error(context, wrong_printf_param_number);
					flagsemicol = 0;
					break;
				}

				totree(context, TString);
				totree(context, fnum);

				for (i = 0; i < fnum; i++)
				{
					totree(context, (int)formatstr[i]);
				}
				totree(context, TExprend);

				totree(context, TPrintf);
				totree(context, sumsize);
			}
				break;

			case GETID:
			{

				mustbe(context, LEFTBR, no_leftbr_in_printid);
				do
				{
					mustbe_complex(context, IDENT, no_ident_in_printid);
					context->lastid = REPRTAB[REPRTAB_POS + 1];
					if (context->lastid == 1)
					{
						context_error(context, ident_is_not_declared);
						context->error_flag = 2;
						flagsemicol = 0;
						break;
					}
					if (context->error_flag == no_ident_in_printid)
					{
						context->error_flag = 2;
						flagsemicol = 0;
						break;
					}
					totree(context, TGetid);
					totree(context, context->lastid);
				} while (context->next == COMMA ? scanner(context), 1 : 0);
				if (context->error_flag == 2)
				{
					context->error_flag = 1;
					break;
				}
				mustbe(context, RIGHTBR, no_rightbr_in_printid);
			}
				break;
			case LBREAK:
			{
				if (!(context->inloop || context->inswitch))
				{
					context_error(context, break_not_in_loop_or_switch);
					flagsemicol = 0;
					break;
				}
				totree(context, TBreak);
			}
				break;
			case LCASE:
			{
				if (!context->inswitch)
				{
					context_error(context, case_or_default_not_in_switch);
					break;
				}
				if (context->wasdefault)
				{
					context_error(context, case_after_default);
					break;
				}
				totree(context, TCase);
				scanner(context);
				unarexpr(context);
				if (context->error_flag == 7)
				{
					context->error_flag = 1;
					break;
				}
				condexpr(context);
				if (context->error_flag == 4)
				{
					context->error_flag = 1;
					break;
				}
				toval(context);
				totree(context, TExprend);
				if (context->ansttype == LFLOAT)
				{
					context_error(context, float_in_switch);
					break;
				}
				context->sopnd--;
				mustbe(context, COLON, no_colon_in_case);
				flagsemicol = 0;
				statement(context);
			}
				break;
			case LCONTINUE:
			{
				if (!context->inloop)
				{
					context_error(context, continue_not_in_loop);
					flagsemicol = 0;
					break;
				}
				totree(context, TContinue);
			}
				break;
			case LDEFAULT:
			{
				if (!context->inswitch)
				{
					context_error(context, case_or_default_not_in_switch);
					break;
				}
				mustbe(context, COLON, no_colon_in_case);
				context->wasdefault = 1;
				flagsemicol = 0;
				totree(context, TDefault);
				statement(context);
			}
				break;
			case LDO:
			{
				context->inloop = 1;
				totree(context, TDo);
				statement(context);

				if (context->next == LWHILE)
				{
					scanner(context);
					exprinbrkts(context, cond_must_be_in_brkts);
					context->sopnd--;
				}
				else
				{
					context_error(context, wait_while_in_do_stmt);
					context->cur = LWHILE;
					exprinbrkts(context, cond_must_be_in_brkts);
					context->sopnd--;
				}
				if (context->error_flag == 3)
				{
					context->error_flag = 1;
					flagsemicol = 0;
				}
			}
				break;
			case LFOR:
			{
				mustbe(context, LEFTBR, no_leftbr_in_for);
				totree(context, TFor);
				size_t fromref = context->sx->tc++;
				size_t condref = context->sx->tc++;
				size_t incrref = context->sx->tc++;
				size_t stmtref = context->sx->tc++;

				if (scanner(context) == SEMICOLON) // init
				{
					context->sx->tree[fromref] = 0;
				}
				else
				{
					context->sx->tree[fromref] = (int)context->sx->tc;
					expr(context, 0);
					if (context->error_flag == 5)
					{
						context->error_flag = 1;
						flagsemicol = 0;
						break;
					}
					exprassnvoid(context);
					mustbe(context, SEMICOLON, no_semicolon_in_for);
				}
				if (scanner(context) == SEMICOLON) // cond
				{
					context->sx->tree[condref] = 0;
				}
				else
				{
					context->sx->tree[condref] = (int)context->sx->tc;
					exprval(context);
					if (context->error_flag == 4)
					{
						context->error_flag = 1;
						flagsemicol = 0;
						break;
					}
					context->sopnd--;
					mustbe(context, SEMICOLON, no_semicolon_in_for);
					context->sopnd--;
				}
				if (scanner(context) == RIGHTBR) // incr
				{
					context->sx->tree[incrref] = 0;
				}
				else
				{
					context->sx->tree[incrref] = (int)context->sx->tc;
					expr(context, 0);
					if (context->error_flag == 5)
					{
						context->error_flag = 1;
						flagsemicol = 0;
						break;
					}
					exprassnvoid(context);
					mustbe(context, RIGHTBR, no_rightbr_in_for);
				}
				flagsemicol = 0;
				context->sx->tree[stmtref] = (int)context->sx->tc;
				context->inloop = 1;
				statement(context);
			}
				break;
			case LGOTO:
			{
				int i;
				int flag = 1;
				mustbe_complex(context, IDENT, no_ident_after_goto);
				if (context->error_flag == no_ident_after_goto)
				{
					context->error_flag = 1;
					flagsemicol = 0;
					break;
				}
				totree(context, TGoto);
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
					if (context->error_flag == 5)
					{
						context->error_flag = 1;
						flagsemicol = 0;
					}
					else
					{
						context->gotost[context->pgotost++] = context->lastid;
					}
				}
				else
				{
					int id = context->gotost[i - 2];
					if (context->gotost[id + 1] < 0) // метка уже была
					{
						totree(context, id);
						break;
					}
					totree(context, context->gotost[context->pgotost++] = id);
				}
				context->gotost[context->pgotost++] = context->line;
			}
				break;
			case LIF:
			{
				totree(context, TIf);
				size_t elseref = context->sx->tc++;
				flagsemicol = 0;
				exprinbrkts(context, cond_must_be_in_brkts);
				if (context->error_flag == 3)
				{
					context->error_flag = 1;
					break;
				}
				context->sopnd--;
				statement(context);
				if (context->next == LELSE)
				{
					scanner(context);
					context->sx->tree[elseref] = (int)context->sx->tc;
					statement(context);
				}
				else
				{
					context->sx->tree[elseref] = 0;
				}
			}
				break;
			case LRETURN:
			{
				int ftype = mode_get(context->sx, context->functype + 1);
				context->wasret = 1;
				if (context->next == SEMICOLON)
				{
					if (ftype != LVOID)
					{
						context_error(context, no_ret_in_func);
						break;
					}
					totree(context, TReturnvoid);
				}
				else
				{
					if (ftype == LVOIDASTER)
					{
						flagsemicol = 0;
					}
					else
					{
						if (ftype == LVOID)
						{
							context_error(context, notvoidret_in_void_func);
							flagsemicol = 0;
							break;
						}
						totree(context, TReturnval);
						totree(context, szof(context, ftype));
						scanner(context);
						expr(context, 1);
						if (context->error_flag == 5)
						{
							context->error_flag = 1;
							flagsemicol = 0;
							break;
						}
						toval(context);
						context->sopnd--;
						if (ftype == LFLOAT && context->ansttype == LINT)
						{
							totree(context, WIDEN);
						}
						else if (ftype != context->ansttype)
						{
							context_error(context, bad_type_in_ret);
							flagsemicol = 0;
							break;
						}
						totree(context, TExprend);
					}
				}
			}
				break;
			case LSWITCH:
			{
				totree(context, TSwitch);
				exprinbrkts(context, cond_must_be_in_brkts);
				if (context->error_flag == 3)
				{
					context->error_flag = 1;
					break;
				}
				if (context->ansttype != LCHAR && context->ansttype != LINT)
				{
					context_error(context, float_in_switch);
					flagsemicol = 0;
					break;
				}
				context->sopnd--;
				scanner(context);
				context->inswitch = 1;
				block(context, -1);
				flagsemicol = 0;
				context->wasdefault = 0;
			}
				break;
			case LWHILE:
			{
				context->inloop = 1;
				totree(context, TWhile);
				flagsemicol = 0;
				exprinbrkts(context, cond_must_be_in_brkts);
				if (context->error_flag == 3)
				{
					context->error_flag = 1;
					break;
				}
				context->sopnd--;
				statement(context);
			}
				break;
			default:
				expr(context, 0);
				if (context->error_flag == 5)
				{
					context->error_flag = 1;
					flagsemicol = 0;
					break;
				}
				exprassnvoid(context);
		}
	}

	if (flagsemicol && scanner(context) != SEMICOLON)
	{
		context_error(context, no_semicolon_after_stmt);
		context->buf_cur = context->next;
		context->next = context->cur;
		context->cur = SEMICOLON;
		context->buf_flag++;
	}
	context->wasdefault = oldwasdefault;
	context->inswitch = oldinswitch;
	context->inloop = oldinloop;
}


/** Debug from here */
void block(analyzer *context, int b)
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
		if (b == 2 ? context->next == TEXIT : context->next == END)
		{
			scanner(context);
			notended = 0;
		}
		else
		{
			statement(context);
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
