/*
 *	Copyright 2016 Andrey Terekhov
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
#include "errors.h"
#include "defs.h"
#include "lexer.h"
#include <string.h>

void binop(parser *context, int sp)
{
	int op = context->stackop[sp];
	int rtype = context->stackoperands[context->sopnd--];
	int ltype = context->stackoperands[context->sopnd];

	if (is_pointer(context->sx, ltype) || is_pointer(context->sx, rtype))
	{
		context_error(context, operand_is_pointer);
		context->error_flag = 5;
		return; // 1
	}
	if ((op == LOGOR || op == LOGAND || op == LOR || op == LEXOR || op == LAND || op == LSHL || op == LSHR ||
		 op == LREM) &&
		(is_float(ltype) || is_float(rtype)))
	{
		context_error(context, int_op_for_float);
		context->error_flag = 5;
		return; // 1
	}
	if (is_int(ltype) && is_float(rtype))
	{
		totree(context, WIDEN1);
	}
	if (is_int(rtype) && is_float(ltype))
	{
		totree(context, WIDEN);
	}
	if (is_float(ltype) || is_float(rtype))
	{
		context->ansttype = LFLOAT;
	}
	if (op == LOGOR || op == LOGAND)
	{
		totree(context, op);
		vector_set(&TREE, context->stacklog[sp], (item_t)vector_size(&TREE));
		vector_increase(&TREE, 1);
	}
	else
	{
		totreef(context, op);
	}
	if (op >= EQEQ && op <= LGE)
	{
		context->ansttype = LINT;
	}
	context->stackoperands[context->sopnd] = context->ansttype;
	// printf("binop context->sopnd=%i ltype=%i rtype=%i
	// context->ansttype=%i\n", context->sopnd, ltype, rtype,
	// context->ansttype);
	context->anst = VAL;
}

void toval(parser *context)
{
	// надо значение положить на стек,
	// например, чтобы передать параметром

	if (context->anst == VAL || context->anst == NUMBER)
	{
		;
	}
	else if (is_struct(context->sx, context->ansttype))
	{
		if (!context->inass)
		{
			if (context->anst == IDENT)
			{
				vector_remove(&TREE);
				vector_remove(&TREE);
				totree(context, COPY0ST);
				totree(context, context->anstdispl);
			}
			else // тут может быть только ADDR
			{
				totree(context, COPY1ST);
			}
			totree(context, mode_get(context->sx, context->ansttype + 1));
			context->anst = VAL;
		}
	}
	else
	{
		if (context->anst == IDENT)
		{
			vector_set(&TREE, vector_size(&TREE) - 2
				, is_float(context->ansttype)
					? TIdenttovald
					: TIdenttoval);
		}

		if (!(is_array(context->sx, context->ansttype) || is_pointer(context->sx, context->ansttype)))
		{
			if (context->anst == ADDR)
			{
				totree(context, is_float(context->ansttype) ? TAddrtovald : TAddrtoval);
			}
		}
		context->anst = VAL;
	}
}

void insertwiden(parser *context)
{
	vector_remove(&TREE);
	totree(context, WIDEN);
	totree(context, TExprend);
}

void applid(parser *context)
{
	context->lastid = REPRTAB[REPRTAB_POS + 1];
	if (context->lastid == 1)
	{
		context_error(context, ident_is_not_declared);
		context->error_flag = 5;
	}
}

void actstring(int type, parser *context)
{
	scanner(context);
	totree(context, type == LFLOAT ? TStringd : TString);
	size_t adn = vector_size(&TREE);
	vector_increase(&TREE, 1);
	
	int n = 0;
	do
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 1;
			return; // 1
		}
		const size_t size = vector_size(&TREE);
		if (vector_get(&TREE, size - 3) == TConstd)
		{
			vector_set(&TREE, size - 3, vector_get(&TREE, size - 2));
			vector_set(&TREE, size - 2, vector_get(&TREE, size - 1));
			vector_remove(&TREE);
		}
		else if (vector_get(&TREE, size - 2) == TConst)
		{
			vector_set(&TREE, size - 2, vector_get(&TREE, size - 1));
			vector_remove(&TREE);
		}
		else
		{
			context_error(context, wrong_init_in_actparam);
			context->error_flag = 1;
			return; // 1
		}
		++n;
	} while (scanner(context) == COMMA ? scanner(context), 1 : 0);

	vector_set(&TREE, adn, n);
	if (context->cur != END)
	{
		context_error(context, no_comma_or_end);
		context->error_flag = 1;
		return; // 1
	}
	context->ansttype = (int)newdecl(context->sx, mode_array, type);
	context->anst = VAL;
}

void mustbestring(parser *context)
{
	scanner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;
	if (!(is_string(context->sx, context->ansttype)))
	{
		context_error(context, not_string_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustbepointstring(parser *context)
{
	scanner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;
	if (!(is_pointer(context->sx, context->ansttype) &&
		  is_string(context->sx, (int)mode_get(context->sx, context->ansttype + 1))))
	{
		context_error(context, not_point_string_in_stanfunc);
		context->error_flag = 5;
		return; // 1
	}
}

void mustberow(parser *context)
{
	scanner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;

	if (!is_array(context->sx, context->ansttype))
	{
		context_error(context, not_array_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustbeint(parser *context)
{
	scanner(context);
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	toval(context);
	context->sopnd--;
	if (context->ansttype != LINT && context->ansttype != LCHAR)
	{
		context_error(context, not_int_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustberowofint(parser *context)
{
	if (scanner(context) == BEGIN)
	{
		actstring(LINT, context), totree(context, TExprend);
		if (context->error_flag == 2)
		{
			context->error_flag = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
		toval(context);
		context->sopnd--;
		if (context->ansttype == LINT || context->ansttype == LCHAR)
		{
			totree(context, ROWING);
			context->ansttype = (int)newdecl(context->sx, mode_array, LINT);
		}
	}
	if (!(is_array(context->sx, context->ansttype) &&
		  is_int((int)mode_get(context->sx, context->ansttype + 1))))
	{
		context_error(context, not_rowofint_in_stanfunc);
		context->error_flag = 5;
	}
}

void mustberowoffloat(parser *context)
{
	if (scanner(context) == BEGIN)
	{
		actstring(LFLOAT, context), totree(context, TExprend);
		if (context->error_flag == 2)
		{
			context->error_flag = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(context, 1);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
		toval(context);
		context->sopnd--;
		if (context->ansttype == LFLOAT)
		{
			totree(context, ROWINGD);
			context->ansttype = (int)newdecl(context->sx, mode_array, LFLOAT);
		}
	}

	if (!(is_array(context->sx, context->ansttype) &&
		  (int)mode_get(context->sx, context->ansttype + 1) == LFLOAT))
	{
		context_error(context, not_rowoffloat_in_stanfunc);
		context->error_flag = 5;
	}
}

void primaryexpr(parser *context)
{
	if (context->cur == CHAR_CONST)
	{
		totree(context, TConst);
		totree(context, context->lxr->num);
		context->stackoperands[++context->sopnd] = context->ansttype = LCHAR;
		context->anst = NUMBER;
	}
	else if (context->cur == INT_CONST)
	{
		totree(context, TConst);
		totree(context, context->lxr->num);
		context->stackoperands[++context->sopnd] = context->ansttype = LINT;
		context->anst = NUMBER;
	}
	else if (context->cur == FLOAT_CONST)
	{
		totree(context, TConstd);
		double_to_tree(&TREE, context->lxr->num_double);
		context->stackoperands[++context->sopnd] = context->ansttype = LFLOAT;
		context->anst = NUMBER;
	}
	else if (context->cur == STRING)
	{
		int i;

		totree(context, TString);
		totree(context, context->lxr->num);

		for (i = 0; i < context->lxr->num; i++)
		{
			totree(context, context->lxr->lexstr[i]);
		}

		context->ansttype = (int)newdecl(context->sx, mode_array, LCHAR);
		context->stackoperands[++context->sopnd] = context->ansttype;
		context->anst = VAL;
	}
	else if (context->cur == IDENT)
	{
		applid(context);
		if (context->error_flag == 5)
		{
			context->error_flag = 4;
			return; // 1
		}

		totree(context, TIdent);
		context->anstdispl = (int)ident_get_displ(context->sx, context->lastid);
		totree(context, context->anstdispl);
		context->ansttype = (int)ident_get_mode(context->sx, context->lastid);
		context->stackoperands[++context->sopnd] = context->ansttype;
		context->anst = IDENT;
	}
	else if (context->cur == LEFTBR)
	{
		if (context->next == LVOID)
		{
			scanner(context);
			mustbe(context, LMULT, no_mult_in_cast);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (!is_pointer(context->sx, context->ansttype))
			{
				context_error(context, not_pointer_in_cast);
				context->error_flag = 4;
				return; // 1
			}
			mustbe(context, RIGHTBR, no_rightbr_in_cast);
			toval(context);
			// totree(context, CASTC);
			totree(context, TExprend);
		}
		else
		{
			int oldsp = context->sp;
			scanner(context);
			expr(context, 1);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			mustbe(context, RIGHTBR, wait_rightbr_in_primary);
			while (context->sp > oldsp)
			{
				binop(context, --context->sp);
			}
		}
	}
	else if (context->cur <= STANDARD_FUNC_START) // стандартная функция
	{
		int func = context->cur;

		if (scanner(context) != LEFTBR)
		{
			context_error(context, no_leftbr_in_stand_func);
			context->buf_cur = context->next;
			context->next = context->cur;
			context->cur = LEFTBR;
			context->buf_flag++;
		}
		if (func == ASSERT)
		{
			mustbeint(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
			mustbestring(context);
		}
		else if (func <= STRCPY && func >= STRLEN) // функции работы со строками
		{
			if (func >= STRNCAT)
			{
				mustbepointstring(context);
			}
			else
			{
				mustbestring(context);
			}
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (func != STRLEN)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustbestring(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if (func == STRNCPY || func == STRNCAT || func == STRNCMP)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
				}
			}
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (func < STRNCAT)
			{
				context->stackoperands[++context->sopnd] = context->ansttype = LINT;
			}
		}
		else if (func >= RECEIVE_STRING && func <= SEND_INT)
		{
			// новые функции Фадеева
			mustbeint(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			if (func == SEND_INT || func == SEND_STRING)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				// scaner(context);
				mustberowofint(context);
			}
			else if (func == SEND_FLOAT)
			{
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				// scaner(context);
				mustberowoffloat(context);
			}
			else
			{
				context->stackoperands[++context->sopnd] = context->ansttype =
					func == RECEIVE_INT ? LINT : func == RECEIVE_FLOAT ? LFLOAT : (int)newdecl(context->sx, mode_array, LCHAR);
			}
		}
		else if (func >= ICON && func <= WIFI_CONNECT) // функции Фадеева
		{
			if (func <= PIXEL && func >= ICON)
			{
				// scaner(context);
				mustberowofint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if (func != CLEAR)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				}

				if (func == LINE || func == RECTANGLE || func == ELLIPS)
				{
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					if (func != LINE)
					{
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
					}
				}
				else if (func == ICON || func == PIXEL)
				{
					mustbeint(context);
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					if (func == ICON)
					{
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
					}
				}
				else if (func == DRAW_NUMBER || func == DRAW_STRING)
				{
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);

					if (func == DRAW_STRING)
					{
						mustbestring(context);
					}
					else // DRAW_NUMBER
					{
						scanner(context);
						exprassn(context, 1);
						if (context->error_flag == 6)
						{
							context->error_flag = 4;
							return; // 1
						}
						toval(context);
						context->sopnd--;
						if (is_int(context->ansttype))
						{
							totree(context, WIDEN);
						}
						else if (context->ansttype != LFLOAT)
						{
							context_error(context, not_float_in_stanfunc);
							context->error_flag = 4;
							return; // 1
						}
					}
				}
			}
			else if (func == SETSIGNAL)
			{
				mustbeint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustberowofint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
				mustberowofint(context);
			}
			else if (func == WIFI_CONNECT || func == BLYNK_AUTORIZATION || func == BLYNK_NOTIFICATION)
			{
				mustbestring(context);
				if (func == WIFI_CONNECT)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					mustbestring(context);
				}
			}
			else
			{
				mustbeint(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if (func != BLYNK_RECEIVE)
				{
					mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
					if (func == BLYNK_TERMINAL)
					{
						mustbestring(context);
					}
					else if (func == BLYNK_SEND)
					{
						mustbeint(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
					}
					else if (func == BLYNK_PROPERTY)
					{
						mustbestring(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbestring(context);
					}
					else // BLYNK_LCD
					{
						mustbeint(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbeint(context);
						if (context->error_flag == 5)
						{
							context->error_flag = 4;
							return; // 1
						}
						mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
						mustbestring(context);
					}
				}
				else
				{
					context->stackoperands[++context->sopnd] = context->ansttype = LINT;
				}
			}
		}
		else if (func == UPB) // UPB
		{
			mustbeint(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			mustbe(context, COMMA, no_comma_in_act_params_stanfunc);
			mustberow(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
			context->stackoperands[++context->sopnd] = context->ansttype = LINT;
		}
		else if (func <= TMSGSEND && func >= TGETNUM) // процедуры управления параллельными нитями
		{
			if (func == TINIT || func == TDESTROY || func == TEXIT)
			{
				; // void()
			}
			else if (func == TMSGRECEIVE || func == TGETNUM) // getnum int()   msgreceive msg_info()
			{
				context->anst = VAL;
				context->ansttype = context->stackoperands[++context->sopnd] =
					func == TGETNUM ? LINT : 2; // 2 - это ссылка на msg_info
												//не было параметра,  выдали 1 результат
			}
			else
			{
				// MSGSEND void(msg_info)  CREATE int(void*(*func)(void*))
				// SEMCREATE int(int)  JOIN,  SLEEP,  SEMWAIT,  SEMPOST void(int)
				// у этих процедур 1 параметр
				scanner(context);

				if (func == TCREATE)
				{
					item_t dn;

					if (context->cur != IDENT)
					{
						context_error(context, act_param_not_ident);
						context->error_flag = 4;
						return; // 1
					}
					applid(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					if (ident_get_mode(context->sx, context->lastid) != 15 ||
						context->error_flag == 5) // 15 - это аргумент типа void* (void*)
					{
						context_error(context, wrong_arg_in_create);
						context->error_flag = 4;
						return; // 1
					}

					context->stackoperands[context->sopnd] = context->ansttype = LINT;
					dn = ident_get_displ(context->sx, context->lastid);
					if (dn < 0)
					{
						totree(context, TIdenttoval);
						totree(context, -dn);
					}
					else
					{
						totree(context, TConst);
						totree(context, dn);
					}
					context->anst = VAL;
				}
				else
				{
					context->leftansttype = 2;
					exprassn(context, 1);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
					toval(context);

					if (func == TMSGSEND)
					{
						if (context->ansttype != 2) // 2 - это аргумент типа msg_info (struct{int numTh; int data;})
						{
							context_error(context, wrong_arg_in_send);
							context->error_flag = 4;
							return; // 1
						}
						--context->sopnd;
					}
					else
					{
						if (!is_int(context->ansttype))
						{
							context_error(context, param_threads_not_int);
							context->error_flag = 4;
							return; // 1
						}
						if (func == TSEMCREATE)
						{
							context->anst = VAL,
							context->ansttype = context->stackoperands[context->sopnd] =
								LINT; // съели 1 параметр, выдали int
						}
						else
						{
							--context->sopnd; // съели 1 параметр, не выдали
						}
						// результата
					}
				}
			}
		}
		else if (func == RAND)
		{
			context->ansttype = context->stackoperands[++context->sopnd] = LFLOAT;
		}
		else if (func == ROUND)
		{
			scanner(context);
			exprassn(context, 1);
			if (context->error_flag == 6)
			{
				context->error_flag = 4;
				return; // 1
			}
			toval(context);
			context->ansttype = context->stackoperands[context->sopnd] = LINT;
		}
		else
		{
			scanner(context);
			exprassn(context, 1);
			if (context->error_flag == 6)
			{
				context->error_flag = 4;
				return; // 1
			}
			toval(context);

			// GETDIGSENSOR int(int port, int pins[]),
			// GETANSENSOR int (int port, int pin),
			// SETMOTOR и VOLTAGE void (int port, int volt)
			if (func == GETDIGSENSOR || func == GETANSENSOR || func == SETMOTOR || func == VOLTAGE)
			{
				if (!is_int(context->ansttype))
				{
					context_error(context, param_setmotor_not_int);
					context->error_flag = 4;
					return; // 1
				}
				mustbe(context, COMMA, no_comma_in_setmotor);
				if (func == GETDIGSENSOR)
				{
					mustberowofint(context);
					if (context->error_flag == 5)
					{
						context->error_flag = 4;
						return; // 1
					}
					context->ansttype = context->stackoperands[++context->sopnd] = LINT;
				}
				else
				{
					scanner(context);
					exprassn(context, 1);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
					toval(context);
					if (!is_int(context->ansttype))
					{
						context_error(context, param_setmotor_not_int);
						context->error_flag = 4;
						return; // 1
					}
					if (func == SETMOTOR || func == VOLTAGE)
					{
						context->sopnd -= 2;
					}
					else
					{
						--context->sopnd, context->anst = VAL;
					}
				}
			}
			else if (func == ABS && is_int(context->ansttype))
			{
				func = ABSI;
			}
			else
			{
				if (is_int(context->ansttype))
				{
					totree(context, WIDEN);
					context->ansttype = context->stackoperands[context->sopnd] = LFLOAT;
				}
				if (!is_float(context->ansttype))
				{
					context_error(context, bad_param_in_stand_func);
					context->error_flag = 4;
					return; // 1
				}
			}
		}
		if (context->error_flag == 5)
		{
			context->error_flag = 4;
			return; // 1
		}
		totree(context, 9500 - func);
		mustbe(context, RIGHTBR, no_rightbr_in_stand_func);
	}
	else
	{
		context_error(context, not_primary);
		context->error_flag = 4;
		return; // 1
	}
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
}

void index_check(parser *context)
{
	if (!is_int(context->ansttype))
	{
		context_error(context, index_must_be_int);
		context->error_flag = 5;
	}
}

int find_field(parser *context, int stype)
{
	// выдает смещение до найденного поля или ошибку

	int flag = 1;
	int select_displ = 0;
	const item_t record_length = mode_get(context->sx, stype + 2);

	scanner(context);
	mustbe(context, IDENT, after_dot_must_be_ident);

	for (item_t i = 0; i < record_length; i += 2) // тут хранится удвоенное n
	{
		int field_type = (int)mode_get(context->sx, stype + 3 + (int)i);

		if (mode_get(context->sx, stype + 4 + (int)i) == (item_t)REPRTAB_POS)
		{
			context->stackoperands[context->sopnd] = context->ansttype = field_type;
			flag = 0;
			break;
		}
		else
		{
			select_displ += szof(context, field_type);
		}
		// прибавляем к суммарному смещению длину поля
	}
	if (flag)
	{
		context_error(context, no_field);
		context->error_flag = 5;
		return 0; // 1
	}
	return select_displ;
}

void selectend(parser *context)
{
	while (context->next == DOT)
	{
		context->anstdispl += find_field(context, context->ansttype);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
	}

	totree(context, context->anstdispl);
	if (is_array(context->sx, context->ansttype) || is_pointer(context->sx, context->ansttype))
	{
		totree(context, TAddrtoval);
	}
}

void array_init(parser *context, int t);

void postexpr(parser *context)
{
	int lid;
	int leftansttyp;
	int was_func = 0;

	lid = context->lastid;
	leftansttyp = context->ansttype;

	if (context->next == LEFTBR) // вызов функции
	{
		int j;
		item_t n;
		item_t dn;
		int oldinass = context->inass;

		was_func = 1;
		scanner(context);
		if (!is_function(context->sx, leftansttyp))
		{
			context_error(context, call_not_from_function);
			context->error_flag = 4;
			return; // 1
		}

		n = mode_get(context->sx, leftansttyp + 2); // берем количество аргументов функции

		totree(context, TCall1);
		totree(context, n);
		j = leftansttyp + 3;
		for (item_t i = 0; i < n; i++) // фактические параметры
		{
			int mdj = context->leftansttype = (int)mode_get(context->sx, j); // это вид формального параметра, в
																   // context->ansttype будет вид фактического
																   // параметра
			scanner(context);
			if (is_function(context->sx, mdj))
			{
				// фактическим параметром должна быть функция, в С - это только идентификатор

				if (context->cur != IDENT)
				{
					context_error(context, act_param_not_ident);
					context->error_flag = 4;
					return; // 1
				}
				applid(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
				if ((int)ident_get_mode(context->sx, context->lastid) != mdj)
				{
					context_error(context, diff_formal_param_type_and_actual);
					context->error_flag = 4;
					return; // 1
				}
				dn = ident_get_displ(context->sx, context->lastid);
				if (dn < 0)
				{
					totree(context, TIdenttoval);
					totree(context, -dn);
				}
				else
				{
					totree(context, TConst);
					totree(context, dn);
				}
				totree(context, TExprend);
			}
			else
			{
				if (context->cur == BEGIN && is_array(context->sx, mdj))
				{
					actstring((int)mode_get(context->sx, mdj + 1), context), totree(context, TExprend);
					if (context->error_flag == 2)
					{
						context->error_flag = 4;
						return; // 1
					}
				}
				else
				{
					context->inass = 0;
					exprassn(context, 1);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
					toval(context);
					totree(context, TExprend);

					if (mdj > 0 && mdj != context->ansttype)
					{
						context_error(context, diff_formal_param_type_and_actual);
						context->error_flag = 4;
						return; // 1
					}

					if (is_int(mdj) && is_float(context->ansttype))
					{
						context_error(context, float_instead_int);
						context->error_flag = 4;
						return; // 1
					}

					if (is_float(mdj) && is_int(context->ansttype))
					{
						insertwiden(context);
					}
					--context->sopnd;
				}
			}
			if (i < n - 1 && scanner(context) != COMMA)
			{
				context_error(context, no_comma_in_act_params);
				context->error_flag = 4;
				return; // 1
			}
			j++;
		}
		context->inass = oldinass;
		mustbe(context, RIGHTBR, wrong_number_of_params);
		totree(context, TCall2);
		totree(context, lid);
		context->stackoperands[context->sopnd] = context->ansttype = (int)mode_get(context->sx, leftansttyp + 1);
		context->anst = VAL;
		if (is_struct(context->sx, context->ansttype))
		{
			context->x -= (int)mode_get(context->sx, context->ansttype + 1) - 1;
		}
	}

	while (context->next == LEFTSQBR || context->next == ARROW || context->next == DOT)
	{
		while (context->next == LEFTSQBR) // вырезка из массива (возможно, многомерного)
		{
			if (was_func)
			{
				context_error(context, slice_from_func);
				context->error_flag = 4;
				return; // 1
			}
			if (!is_array(context->sx, context->ansttype)) // вырезка не из массива
			{
				context_error(context, slice_not_from_array);
				context->error_flag = 4;
				return; // 1
			}

			item_t elem_type = mode_get(context->sx, context->ansttype + 1);

			scanner(context);
			scanner(context);

			if (context->anst == IDENT) // a[i]
			{
				const size_t size = vector_size(&TREE);
				vector_set(&TREE, size - 2, TSliceident);
				vector_set(&TREE, size - 1, context->anstdispl);
			}
			else // a[i][j]
			{
				totree(context, TSlice);
			}

			totree(context, elem_type);
			exprval(context);
			if (context->error_flag == 4)
			{
				return; // 1
			}
			index_check(context); // проверка, что индекс int или char
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}

			mustbe(context, RIGHTSQBR, no_rightsqbr_in_slice);

			context->stackoperands[--context->sopnd] = context->ansttype = (int)elem_type;
			context->anst = ADDR;
		}

		while (context->next == ARROW)
		{
			// это выборка поля из указателя на структуру, если после
			// -> больше одной точки подряд, схлопываем в 1 select
			// перед выборкой мог быть вызов функции или вырезка элемента массива

			if (!is_pointer(context->sx, context->ansttype) ||
				!is_struct(context->sx, (int)mode_get(context->sx, context->ansttype + 1)))
			{
				context_error(context, get_field_not_from_struct_pointer);
				context->error_flag = 4;
				return; // 1
			}

			if (context->anst == IDENT)
			{
				vector_set(&TREE, vector_size(&TREE) - 2, TIdenttoval);
			}
			context->anst = ADDR;
			// pointer  мог быть значением функции (VAL) или, может быть,
			totree(context, TSelect); // context->anst уже был ADDR, т.е. адрес
									  // теперь уже всегда на верхушке стека

			context->ansttype = (int)mode_get(context->sx, context->ansttype + 1);
			context->anstdispl = find_field(context, context->ansttype);
			if (context->error_flag == 6)
			{
				context->error_flag = 4;
				return; // 1
			}
			selectend(context);
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
		}
		if (context->next == DOT)

		{
			if (!is_struct(context->sx, context->ansttype))
			{
				context_error(context, select_not_from_struct);
				context->error_flag = 4;
				return; // 1
			}
			if (context->anst == VAL) // структура - значение функции
			{
				int len1 = szof(context, context->ansttype);
				context->anstdispl = 0;
				while (context->next == DOT)
				{
					context->anstdispl += find_field(context, context->ansttype);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
				}
				totree(context, COPYST);
				totree(context, context->anstdispl);
				totree(context, szof(context, context->ansttype));
				totree(context, len1);
			}
			else if (context->anst == IDENT)
			{
				int globid = context->anstdispl < 0 ? -1 : 1;
				while (context->next == DOT)
				{
					context->anstdispl += globid * find_field(context, context->ansttype);
					if (context->error_flag == 6)
					{
						context->error_flag = 4;
						return; // 1
					}
				}
				vector_set(&TREE, vector_size(&TREE) - 1, context->anstdispl);
			}
			else // ADDR
			{
				totree(context, TSelect);
				context->anstdispl = 0;
				selectend(context);
				if (context->error_flag == 5)
				{
					context->error_flag = 4;
					return; // 1
				}
			}
		}
	}
	if (context->next == INC || context->next == DEC) // a++, a--
	{
		int op;

		if (!is_int(context->ansttype) && !is_float(context->ansttype))
		{
			context_error(context, wrong_operand);
			context->error_flag = 4;
			return; // 1
		}

		if (context->anst != IDENT && context->anst != ADDR)
		{
			context_error(context, unassignable_inc);
			context->error_flag = 4;
			return; // 1
		}
		op = (context->next == INC) ? POSTINC : POSTDEC;
		if (context->anst == ADDR)
		{
			op += 4;
		}
		scanner(context);
		totreef(context, op);
		if (context->anst == IDENT)
		{
			totree(context, ident_get_displ(context->sx, lid));
		}
		context->anst = VAL;
	}
}

void unarexpr(parser *context)
{
	int op = context->cur;
	if (context->cur == LNOT || context->cur == LOGNOT || context->cur == LPLUS || context->cur == LMINUS ||
		context->cur == LAND || context->cur == LMULT || context->cur == INC || context->cur == DEC)
	{
		if (context->cur == INC || context->cur == DEC)
		{
			scanner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				return; // 1
			}
			if (context->anst != IDENT && context->anst != ADDR)
			{
				context_error(context, unassignable_inc);
				context->error_flag = 7;
				return; // 1
			}
			if (context->anst == ADDR)
			{
				op += 4;
			}
			totreef(context, op);
			if (context->anst == IDENT)
			{
				totree(context, ident_get_displ(context->sx, context->lastid));
			}
			context->anst = VAL;
		}
		else
		{
			scanner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				return; // 1
			}

			if (op == LAND)
			{
				if (context->anst == VAL)
				{
					context_error(context, wrong_addr);
					context->error_flag = 7;
					return; // 1
				}

				if (context->anst == IDENT)
				{
					vector_set(&TREE, vector_size(&TREE) - 2, TIdenttoaddr); // &a
				}

				context->stackoperands[context->sopnd] = context->ansttype =
					(int)newdecl(context->sx, mode_pointer, context->ansttype);
				context->anst = VAL;
			}
			else if (op == LMULT)
			{
				if (!is_pointer(context->sx, context->ansttype))
				{
					context_error(context, aster_not_for_pointer);
					context->error_flag = 7;
					return; // 1
				}

				if (context->anst == IDENT)
				{
					vector_set(&TREE, vector_size(&TREE) - 2, TIdenttoval); // *p
				}

				context->stackoperands[context->sopnd] = context->ansttype = (int)mode_get(context->sx, context->ansttype + 1);
				context->anst = ADDR;
			}
			else
			{
				toval(context);
				if ((op == LNOT || op == LOGNOT) && context->ansttype == LFLOAT)
				{
					context_error(context, int_op_for_float);
					context->error_flag = 7;
					return; // 1
				}
				else if (op == LMINUS)
				{
					const size_t size = vector_size(&TREE);
					if (vector_get(&TREE, size - 2) == TConst)
					{
						vector_set(&TREE, vector_size(&TREE) - 1, -vector_get(&TREE, vector_size(&TREE) - 1));
					}
					else if (vector_get(&TREE, size - 3) == TConstd)
					{
						double_to_tree(&TREE, -double_from_tree(&TREE));
					}
					else
					{
						totreef(context, UNMINUS);
					}
				}
				else if (op == LPLUS)
				{
					;
				}
				else
				{
					totree(context, op);
				}
				context->anst = VAL;
			}
		}
	}
	else
	{
		primaryexpr(context);
		if (context->error_flag == 4)
		{
			context->error_flag = 7;
			return; // 1
		}
	}

	postexpr(context); // 0
	context->stackoperands[context->sopnd] = context->ansttype;
	if (context->error_flag == 4)
	{
		context->error_flag = 7;
		return; // 1
	}
}

void exprinbrkts(parser *context, int er)
{
	mustbe(context, LEFTBR, er);
	scanner(context);
	exprval(context);
	if (context->error_flag == 4)
	{
		context->error_flag = 3;
		return; // 1
	}
	mustbe(context, RIGHTBR, er);
}

void exprassninbrkts(parser *context, int er)
{
	mustbe(context, LEFTBR, er);
	scanner(context);
	exprassnval(context);
	if (context->error_flag == 4)
	{
		context->error_flag = 3;
		return; // 1
	}
	mustbe(context, RIGHTBR, er);
}

int prio(int op)
{
	// возвращает 0, если не операция
	return op == LOGOR
			   ? 1
			   : op == LOGAND
					 ? 2
					 : op == LOR
						   ? 3
						   : op == LEXOR
								 ? 4
								 : op == LAND
									   ? 5
									   : op == EQEQ
											 ? 6
											 : op == NOTEQ
												   ? 6
												   : op == LLT
														 ? 7
														 : op == LGT
															   ? 7
															   : op == LLE
																	 ? 7
																	 : op == LGE
																		   ? 7
																		   : op == LSHL
																				 ? 8
																				 : op == LSHR
																					   ? 8
																					   : op == LPLUS
																							 ? 9
																							 : op == LMINUS
																								   ? 9
																								   : op == LMULT
																										 ? 10
																										 : op == LDIV
																											   ? 10
																											   : op == LREM
																													 ? 10
																													 : 0;
}

void subexpr(parser *context)
{
	int oldsp = context->sp;
	int wasop = 0;

	int p = prio(context->next);
	while (p)
	{
		wasop = 1;
		toval(context);
		while (context->sp > oldsp && context->stack[context->sp - 1] >= p)
		{
			binop(context, --context->sp);
			if (context->error_flag == 5)
			{
				return;
			}
		}

		size_t ad = 0;
		if (p <= 2)
		{
			totree(context, p == 1 ? ADLOGOR : ADLOGAND);
			ad = vector_size(&TREE);
			vector_increase(&TREE, 1);
		}

		context->stack[context->sp] = p;
		context->stacklog[context->sp] = (int)ad;
		context->stackop[context->sp++] = context->next;
		scanner(context);
		scanner(context);
		unarexpr(context);
		if (context->error_flag == 7)
		{
			context->error_flag = 5;
			return; // 1
		}
		p = prio(context->next);
	}
	if (wasop)
	{
		toval(context);
	}
	while (context->sp > oldsp)
	{
		binop(context, --context->sp);
		if (context->error_flag == 5)
		{
			return;
		}
	}
}

int intopassn(int next)
{
	return next == REMASS || next == SHLASS || next == SHRASS || next == ANDASS || next == EXORASS || next == ORASS;
}

int opassn(parser *context)
{
	return (context->next == ASS || context->next == MULTASS || context->next == DIVASS || context->next == PLUSASS ||
			context->next == MINUSASS || intopassn(context->next))
			   ? context->op = context->next
			   : 0;
}

void condexpr(parser *context)
{
	int globtype = 0;
	size_t adif = 0;

	subexpr(context); // logORexpr();
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
	if (context->next == QUEST)
	{
		while (context->next == QUEST)
		{
			toval(context);
			if (!is_int(context->ansttype))
			{
				context_error(context, float_in_condition);
				context->error_flag = 4;
				return; // 1
			}
			totree(context, TCondexpr);
			scanner(context);
			scanner(context);
			context->sopnd--;
			exprval(context); // then
			if (context->error_flag == 4)
			{
				return; // 1
			}
			if (!globtype)
			{
				globtype = context->ansttype;
			}
			context->sopnd--;
			if (is_float(context->ansttype))
			{
				globtype = LFLOAT;
			}
			else
			{
				vector_add(&TREE, (item_t)adif);
				adif = vector_size(&TREE) - 1;
			}
			mustbe(context, COLON, no_colon_in_cond_expr);
			scanner(context);
			unarexpr(context);
			if (context->error_flag == 7)
			{
				context->error_flag = 4;
				return; // 1
			}
			subexpr(context); // logORexpr();	else or elif
			if (context->error_flag == 5)
			{
				context->error_flag = 4;
				return; // 1
			}
		}
		toval(context);
		totree(context, TExprend);
		if (is_float(context->ansttype))
		{
			globtype = LFLOAT;
		}
		else
		{
			vector_add(&TREE, (item_t)adif);
			adif = vector_size(&TREE) - 1;
		}

		while (adif != 0 && adif <= vector_size(&TREE))
		{
			item_t r = (size_t)vector_get(&TREE, adif);
			vector_set(&TREE, adif, TExprend);
			vector_set(&TREE, adif - 1, is_float(globtype) ? WIDEN : NOP);
			adif = (size_t)r;
		}

		context->stackoperands[context->sopnd] = context->ansttype = globtype;
	}
	else
	{
		context->stackoperands[context->sopnd] = context->ansttype;
	}
}

void exprassnvoid(parser *context)
{
	const size_t size = vector_size(&TREE);
	size_t t = vector_get(&TREE, size - 2) < 9000 ? size - 3 : size - 2;
	item_t tt = vector_get(&TREE, t);
	if ((tt >= ASS && tt <= DIVASSAT) || (tt >= POSTINC && tt <= DECAT) || (tt >= ASSR && tt <= DIVASSATR) ||
		(tt >= POSTINCR && tt <= DECATR))
	{
		vector_set(&TREE, t, vector_get(&TREE, t) + 200);
	}
	--context->sopnd;
}

void exprassn(parser *context, int level)
{
	int leftanst;
	int leftanstdispl;
	int ltype;
	int rtype;
	int lnext;

	if (context->cur == BEGIN)
	{
		if (is_struct(context->sx, context->leftansttype))
		{
			struct_init(context, context->leftansttype);
		}
		else if (is_array(context->sx, context->leftansttype))
		{
			// пока в RuC присваивать массивы нельзя
			array_init(context, context->leftansttype);
			if (context->error_flag == 7)
			{
				context->error_flag = 6;
				return; // 1
			}
		}
		else
		{
			context_error(context, init_not_struct);
			context->error_flag = 6;
			return; // 1
		}
		context->stackoperands[++context->sopnd] = context->ansttype = context->leftansttype;
		context->anst = VAL;
	}
	else
	{
		unarexpr(context);
	}
	if (context->error_flag == 7)
	{
		context->error_flag = 6;
		return; // 1
	}

	leftanst = context->anst;
	leftanstdispl = context->anstdispl;
	context->leftansttype = context->ansttype;
	if (opassn(context))
	{
		int opp = context->op;
		lnext = context->next;
		context->inass = 1;
		scanner(context);
		scanner(context);
		exprassn(context, level + 1);
		if (context->error_flag == 6)
		{
			return; // 1
		}
		context->inass = 0;

		if (leftanst == VAL)
		{
			context_error(context, unassignable);
			context->error_flag = 6;
			return; // 1
		}
		rtype = context->stackoperands[context->sopnd--]; // снимаем типы
														  // операндов со стека
		ltype = context->stackoperands[context->sopnd];

		if (intopassn(lnext) && (is_float(ltype) || is_float(rtype)))
		{
			context_error(context, int_op_for_float);
			context->error_flag = 6;
			return; // 1
		}

		if (is_array(context->sx, ltype)) // присваивать массив в массив в си нельзя
		{
			context_error(context, array_assigment);
			context->error_flag = 6;
			return; // 1
		}

		if (is_struct(context->sx, ltype)) // присваивание в структуру
		{
			if (ltype != rtype) // типы должны быть равны
			{
				context_error(context, type_missmatch);
				context->error_flag = 6;
				return; // 1
			}
			if (opp != ASS) // в структуру можно присваивать только с помощью =
			{
				context_error(context, wrong_struct_ass);
				context->error_flag = 6;
				return; // 1
			}

			if (context->anst == VAL)
			{
				opp = leftanst == IDENT ? COPY0STASS : COPY1STASS;
			}
			else
			{
				opp = leftanst == IDENT ? context->anst == IDENT ? COPY00 : COPY01
										: context->anst == IDENT ? COPY10 : COPY11;
			}
			totree(context, opp);
			if (leftanst == IDENT)
			{
				totree(context, leftanstdispl); // displleft
			}
			if (context->anst == IDENT)
			{
				totree(context, context->anstdispl); // displright
			}
			totree(context, mode_get(context->sx, ltype + 1)); // длина
			context->anst = leftanst;
			context->anstdispl = leftanstdispl;
		}
		else // оба операнда базового типа или указатели
		{
			if (is_pointer(context->sx, ltype) && opp != ASS) // в указатель можно присваивать только с помощью =
			{
				context_error(context, wrong_struct_ass);
				context->error_flag = 6;
				return; // 1
			}

			if (is_int(ltype) && is_float(rtype))
			{
				context_error(context, assmnt_float_to_int);
				context->error_flag = 6;
				return; // 1
			}

			toval(context);
			if (is_int(rtype) && is_float(ltype))
			{
				totree(context, WIDEN);
				context->ansttype = LFLOAT;
			}
			if (is_pointer(context->sx, ltype) && is_pointer(context->sx, rtype) && ltype != rtype)
			{
				// проверка нужна только для указателей
				context_error(context, type_missmatch);
				context->error_flag = 6;
				return; // 1
			}

			if (leftanst == ADDR)
			{
				opp += 11;
			}
			totreef(context, opp);
			if (leftanst == IDENT)
			{
				context->anstdispl = leftanstdispl;
				totree(context, leftanstdispl);
			}
			context->anst = VAL;
		}
		context->ansttype = ltype;
		context->stackoperands[context->sopnd] = ltype; // тип результата - на стек
	}
	else
	{
		condexpr(context); // condexpr учитывает тот факт, что начало выражения
		if (context->error_flag == 4)
		{
			context->error_flag = 6;
			return; // 1
		}
	}
	// в виде unarexpr уже выкушано
}

void expr(parser *context, int level)
{
	exprassn(context, level);
	if (context->error_flag == 6)
	{
		context->error_flag = 5;
		return; // 1
	}
	while (context->next == COMMA)
	{
		exprassnvoid(context);
		context->sopnd--;
		scanner(context);
		scanner(context);
		exprassn(context, level);
		if (context->error_flag == 6)
		{
			context->error_flag = 5;
			return; // 1
		}
	}
	if (level == 0)
	{
		totree(context, TExprend);
	}
}

void exprval(parser *context)
{
	expr(context, 1);
	if (context->error_flag == 5)
	{
		context->error_flag = 4;
		return; // 1
	}
	toval(context);
	totree(context, TExprend);
}

void exprassnval(parser *context)
{
	exprassn(context, 1);
	if (context->error_flag == 6)
	{
		context->error_flag = 4;
		return; // 1
	}
	toval(context);
	totree(context, TExprend);
}
