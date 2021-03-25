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
#include <stdlib.h>
#include <string.h>


void parse_unary_expression(parser *const prs);
void exprassn(parser *const prs);
void expr(parser *const prs);
item_t parse_constant(parser *const prs);


void scanner(parser *const prs)
{
	prs->token = lex(prs->lxr);
}

void must_be(parser *const prs, const token_t what, const error_t num)
{
	if (prs->token != what)
	{
		parser_error(prs, num);
	}
	else
	{
		scanner(prs);
	}
}

void applid(parser *const prs)
{
	const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
	if (id == ITEM_MAX)
	{
		parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, REPRTAB_POS));
		prs->was_error = 5;
	}

	prs->lastid = (size_t)id;
}

static void toval(parser *const prs)
{
	// надо значение положить на стек, например, чтобы передать параметром
	if (prs->anst == VAL || prs->anst == NUMBER)
	{
		return;
	}

	if (mode_is_struct(prs->sx, prs->ansttype))
	{
		if (!prs->flag_in_assignment)
		{
			if (prs->anst == IDENT)
			{
				vector_remove(&TREE);
				vector_remove(&TREE);
				totree(prs, COPY0ST);
				totree(prs, prs->anstdispl);
			}
			else // тут может быть только ADDR
			{
				totree(prs, COPY1ST);
			}
			totree(prs, mode_get(prs->sx, prs->ansttype + 1));
			prs->anst = VAL;
		}
		return;
	}

	if (prs->anst == IDENT)
	{
		vector_set(&prs->sx->tree, vector_size(&prs->sx->tree) - 2, mode_is_float(prs->ansttype) ? TIdenttovald : TIdenttoval);
	}

	if (!mode_is_array(prs->sx, prs->ansttype) && !mode_is_pointer(prs->sx, prs->ansttype) && prs->anst == ADDR)
	{
		totree(prs, mode_is_float(prs->ansttype) ? TAddrtovald : TAddrtoval);
	}
	prs->anst = VAL;
}

void mustbestring(parser *const prs)
{
	exprassn(prs);
	if (prs->was_error == 6)
	{
		prs->was_error = 5;
		return; // 1
	}
	toval(prs);
	prs->sopnd--;
	if (!(mode_is_string(prs->sx, prs->ansttype)))
	{
		parser_error(prs, not_string_in_stanfunc);
		prs->was_error = 5;
	}
}

void mustbepointstring(parser *const prs)
{
	exprassn(prs);
	if (prs->was_error == 6)
	{
		prs->was_error = 5;
		return; // 1
	}
	toval(prs);
	prs->sopnd--;
	if (!(mode_is_pointer(prs->sx, prs->ansttype) &&
		  mode_is_string(prs->sx, mode_get(prs->sx, prs->ansttype + 1))))
	{
		parser_error(prs, not_point_string_in_stanfunc);
		prs->was_error = 5;
		return; // 1
	}
}

void mustberow(parser *const prs)
{
	exprassn(prs);
	if (prs->was_error == 6)
	{
		prs->was_error = 5;
		return; // 1
	}
	toval(prs);
	prs->sopnd--;

	if (!mode_is_array(prs->sx, prs->ansttype))
	{
		parser_error(prs, not_array_in_stanfunc);
		prs->was_error = 5;
	}
}

void mustbeint(parser *const prs)
{
	exprassn(prs);
	if (prs->was_error == 6)
	{
		prs->was_error = 5;
		return; // 1
	}
	toval(prs);
	prs->sopnd--;
	if (prs->ansttype != LINT && prs->ansttype != LCHAR)
	{
		parser_error(prs, not_int_in_stanfunc);
		prs->was_error = 5;
	}
}

void mustberowofint(parser *const prs)
{
	if (prs->token == BEGIN)
	{
		parse_braced_init_list(prs, LINT), totree(prs, TExprend);
		if (prs->was_error == 2)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(prs);
		if (prs->was_error == 6)
		{
			prs->was_error = 5;
			return; // 1
		}
		toval(prs);
		prs->sopnd--;
		if (prs->ansttype == LINT || prs->ansttype == LCHAR)
		{
			totree(prs, ROWING);
			prs->ansttype = (int)to_modetab(prs, mode_array, LINT);
		}
	}
	if (!(mode_is_array(prs->sx, prs->ansttype) &&
		  mode_is_int(mode_get(prs->sx, prs->ansttype + 1))))
	{
		parser_error(prs, not_rowofint_in_stanfunc);
		prs->was_error = 5;
	}
}

void mustberowoffloat(parser *const prs)
{
	if (prs->token == BEGIN)
	{
		parse_braced_init_list(prs, LFLOAT), totree(prs, TExprend);
		if (prs->was_error == 2)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(prs);
		if (prs->was_error == 6)
		{
			prs->was_error = 5;
			return; // 1
		}
		toval(prs);
		prs->sopnd--;
		if (prs->ansttype == LFLOAT)
		{
			totree(prs, ROWINGD);
			prs->ansttype = (int)to_modetab(prs, mode_array, LFLOAT);
		}
	}

	if (!(mode_is_array(prs->sx, prs->ansttype) &&
		  mode_get(prs->sx, prs->ansttype + 1) == LFLOAT))
	{
		parser_error(prs, not_rowoffloat_in_stanfunc);
		prs->was_error = 5;
	}
}

void parse_standard_function_call(parser *const prs)
{
	token_t func = prs->token;
	token_consume(prs);

	if (!token_try_consume(prs, l_paren))
	{
		parser_error(prs, no_leftbr_in_stand_func);
		token_skip_until(prs, r_paren | semicolon);
		return;
	}

	if (func == ASSERT)
	{
		mustbeint(prs);
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}
		must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
		mustbestring(prs);
	}
	else if (func <= STRCPY && func >= STRLEN) // функции работы со строками
	{
		if (func >= STRNCAT)
		{
			mustbepointstring(prs);
		}
		else
		{
			mustbestring(prs);
		}
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}
		if (func != STRLEN)
		{
			must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
			mustbestring(prs);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
			if (func == STRNCPY || func == STRNCAT || func == STRNCMP)
			{
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				mustbeint(prs);
			}
		}
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}
		if (func < STRNCAT)
		{
			prs->stackoperands[++prs->sopnd] = prs->ansttype = LINT;
		}
	}
	else if (func >= RECEIVE_STRING && func <= SEND_INT)
	{
		// новые функции Фадеева
		mustbeint(prs);
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}
		if (func == SEND_INT || func == SEND_STRING)
		{
			must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
			// scaner(context);
			mustberowofint(prs);
		}
		else if (func == SEND_FLOAT)
		{
			must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
			// scaner(context);
			mustberowoffloat(prs);
		}
		else
		{
			prs->stackoperands[++prs->sopnd] = prs->ansttype =
			func == RECEIVE_INT ? LINT : func == RECEIVE_FLOAT ? LFLOAT : (int)to_modetab(prs, mode_array, LCHAR);
		}
	}
	else if (func >= ICON && func <= WIFI_CONNECT) // функции Фадеева
	{
		if (func <= PIXEL && func >= ICON)
		{
			// scaner(context);
			mustberowofint(prs);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
			if (func != CLEAR)
			{
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
			}

			if (func == LINE || func == RECTANGLE || func == ELLIPS)
			{
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				if (func != LINE)
				{
					must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(prs);
				}
			}
			else if (func == ICON || func == PIXEL)
			{
				mustbeint(prs);
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				if (func == ICON)
				{
					must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(prs);
				}
			}
			else if (func == DRAW_NUMBER || func == DRAW_STRING)
			{
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				mustbeint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);

				if (func == DRAW_STRING)
				{
					mustbestring(prs);
				}
				else // DRAW_NUMBER
				{
					exprassn(prs);
					if (prs->was_error == 6)
					{
						prs->was_error = 4;
						return; // 1
					}
					toval(prs);
					prs->sopnd--;
					if (mode_is_int(prs->ansttype))
					{
						totree(prs, WIDEN);
					}
					else if (prs->ansttype != LFLOAT)
					{
						parser_error(prs, not_float_in_stanfunc);
						prs->was_error = 4;
						return; // 1
					}
				}
			}
		}
		else if (func == SETSIGNAL)
		{
			mustbeint(prs);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
			must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
			mustberowofint(prs);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
			must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
			mustberowofint(prs);
		}
		else if (func == WIFI_CONNECT || func == BLYNK_AUTORIZATION || func == BLYNK_NOTIFICATION)
		{
			mustbestring(prs);
			if (func == WIFI_CONNECT)
			{
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				mustbestring(prs);
			}
		}
		else
		{
			mustbeint(prs);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
			if (func != BLYNK_RECEIVE)
			{
				must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
				if (func == BLYNK_TERMINAL)
				{
					mustbestring(prs);
				}
				else if (func == BLYNK_SEND)
				{
					mustbeint(prs);
					if (prs->was_error == 5)
					{
						prs->was_error = 4;
						return; // 1
					}
				}
				else if (func == BLYNK_PROPERTY)
				{
					mustbestring(prs);
					if (prs->was_error == 5)
					{
						prs->was_error = 4;
						return; // 1
					}
					must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
					mustbestring(prs);
				}
				else // BLYNK_LCD
				{
					mustbeint(prs);
					if (prs->was_error == 5)
					{
						prs->was_error = 4;
						return; // 1
					}
					must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
					mustbeint(prs);
					if (prs->was_error == 5)
					{
						prs->was_error = 4;
						return; // 1
					}
					must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
					mustbestring(prs);
				}
			}
			else
			{
				prs->stackoperands[++prs->sopnd] = prs->ansttype = LINT;
			}
		}
	}
	else if (func == UPB) // UPB
	{
		mustbeint(prs);
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}
		must_be(prs, COMMA, no_comma_in_act_params_stanfunc);
		mustberow(prs);
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}
		prs->stackoperands[++prs->sopnd] = prs->ansttype = LINT;
	}
	else if (func <= TMSGSEND && func >= TGETNUM) // процедуры управления параллельными нитями
	{
		if (func == TINIT || func == TDESTROY || func == TEXIT)
		{
			; // void()
		}
		else if (func == TMSGRECEIVE || func == TGETNUM) // getnum int()   msgreceive msg_info()
		{
			prs->anst = VAL;
			prs->ansttype = prs->stackoperands[++prs->sopnd] =
			func == TGETNUM ? LINT : 2; // 2 - это ссылка на msg_info
										//не было параметра,  выдали 1 результат
		}
		else
		{
			// MSGSEND void(msg_info)  CREATE int(void*(*func)(void*))
			// SEMCREATE int(int)  JOIN,  SLEEP,  SEMWAIT,  SEMPOST void(int)
			// у этих процедур 1 параметр

			if (func == TCREATE)
			{
				item_t dn;

				if (!token_try_consume(prs, IDENT))
				{
					parser_error(prs, act_param_not_ident);
					prs->was_error = 4;
					return; // 1
				}
				applid(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				if (ident_get_mode(prs->sx, prs->lastid) != 15 ||
					prs->was_error == 5) // 15 - это аргумент типа void* (void*)
				{
					parser_error(prs, wrong_arg_in_create);
					prs->was_error = 4;
					return; // 1
				}

				prs->stackoperands[prs->sopnd] = prs->ansttype = LINT;
				dn = ident_get_displ(prs->sx, prs->lastid);
				if (dn < 0)
				{
					totree(prs, TIdenttoval);
					node_add_arg(&prs->nd, -dn);
				}
				else
				{
					totree(prs, TConst);
					node_add_arg(&prs->nd, dn);
				}
				prs->anst = VAL;
			}
			else
			{
				prs->leftansttype = 2;
				exprassn(prs);
				if (prs->was_error == 6)
				{
					prs->was_error = 4;
					return; // 1
				}
				toval(prs);

				if (func == TMSGSEND)
				{
					if (prs->ansttype != 2) // 2 - это аргумент типа msg_info (struct{int numTh; int data;})
					{
						parser_error(prs, wrong_arg_in_send);
						prs->was_error = 4;
						return; // 1
					}
					--prs->sopnd;
				}
				else
				{
					if (!mode_is_int(prs->ansttype))
					{
						parser_error(prs, param_threads_not_int);
						prs->was_error = 4;
						return; // 1
					}
					if (func == TSEMCREATE)
					{
						prs->anst = VAL,
						prs->ansttype = prs->stackoperands[prs->sopnd] =
						LINT; // съели 1 параметр, выдали int
					}
					else
					{
						--prs->sopnd; // съели 1 параметр, не выдали
					}
					// результата
				}
			}
		}
	}
	else if (func == RAND)
	{
		prs->ansttype = prs->stackoperands[++prs->sopnd] = LFLOAT;
	}
	else if (func == ROUND)
	{
		exprassn(prs);
		if (prs->was_error == 6)
		{
			prs->was_error = 4;
			return; // 1
		}
		toval(prs);
		prs->ansttype = prs->stackoperands[prs->sopnd] = LINT;
	}
	else
	{
		exprassn(prs);
		if (prs->was_error == 6)
		{
			prs->was_error = 4;
			return; // 1
		}
		toval(prs);

		// GETDIGSENSOR int(int port, int pins[]),
		// GETANSENSOR int (int port, int pin),
		// SETMOTOR и VOLTAGE void (int port, int volt)
		if (func == GETDIGSENSOR || func == GETANSENSOR || func == SETMOTOR || func == VOLTAGE)
		{
			if (!mode_is_int(prs->ansttype))
			{
				parser_error(prs, param_setmotor_not_int);
				prs->was_error = 4;
				return; // 1
			}
			must_be(prs, COMMA, no_comma_in_setmotor);
			if (func == GETDIGSENSOR)
			{
				mustberowofint(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
				prs->ansttype = prs->stackoperands[++prs->sopnd] = LINT;
			}
			else
			{
				exprassn(prs);
				if (prs->was_error == 6)
				{
					prs->was_error = 4;
					return; // 1
				}
				toval(prs);
				if (!mode_is_int(prs->ansttype))
				{
					parser_error(prs, param_setmotor_not_int);
					prs->was_error = 4;
					return; // 1
				}
				if (func == SETMOTOR || func == VOLTAGE)
				{
					prs->sopnd -= 2;
				}
				else
				{
					--prs->sopnd, prs->anst = VAL;
				}
			}
		}
		else if (func == ABS && mode_is_int(prs->ansttype))
		{
			func = ABSI;
		}
		else
		{
			if (mode_is_int(prs->ansttype))
			{
				totree(prs, WIDEN);
				prs->ansttype = prs->stackoperands[prs->sopnd] = LFLOAT;
			}
			if (!mode_is_float(prs->ansttype))
			{
				parser_error(prs, bad_param_in_stand_func);
				prs->was_error = 4;
				return; // 1
			}
		}
	}
	if (prs->was_error == 5)
	{
		prs->was_error = 4;
		return; // 1
	}
	totree(prs, 9500 - func);
	must_be(prs, RIGHTBR, no_rightbr_in_stand_func);
}
