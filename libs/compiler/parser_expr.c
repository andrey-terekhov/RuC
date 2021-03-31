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
void parse_assignment_expression_internal(parser *const prs);
void parse_expression_internal(parser *const prs);
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

void totree_float_operation(parser *const prs, item_t op)
{
	if (prs->ansttype == LFLOAT &&
		((op >= ASS && op <= DIVASS) || (op >= ASSAT && op <= DIVASSAT) || (op >= EQEQ && op <= UNMINUS)))
	{
		to_tree(prs, op + 50);
	}
	else
	{
		to_tree(prs, op);
	}
}


void node_add_double(node *const nd, const double value)
{
	int64_t num64;
	memcpy(&num64, &value, sizeof(int64_t));

	const int32_t fst = num64 & 0x00000000ffffffff;
	const int32_t snd = (num64 & 0xffffffff00000000) >> 32;

	node_add_arg(nd, fst);
	node_add_arg(nd, snd);
}

void node_set_double(node *const nd, const size_t index, const double value)
{
	int64_t num64;
	memcpy(&num64, &value, sizeof(int64_t));

	const int32_t fst = num64 & 0x00000000ffffffff;
	const int32_t snd = (num64 & 0xffffffff00000000) >> 32;

	if (node_set_arg(nd, index, fst))
	{
		node_add_arg(nd, fst);
		node_add_arg(nd, snd);
	}
	else if (node_set_arg(nd, index + 1, snd))
	{
		node_add_arg(nd, snd);
	}
}

double node_get_double(node *const nd, const size_t index)
{
	const int64_t fst = (int64_t)node_get_arg(nd, index) & 0x00000000ffffffff;
	const int64_t snd = (int64_t)node_get_arg(nd, index + 1) & 0x00000000ffffffff;
	const int64_t num64 = (snd << 32) | fst;

	double num;
	memcpy(&num, &num64, sizeof(double));
	return num;
}


item_t anst_push(parser *const prs, const anst_val type, const item_t mode)
{
	prs->stackoperands[++prs->sopnd] = prs->ansttype = mode;
	prs->anst = type;
	return mode;
}

item_t anst_pop(parser *const prs)
{
	return prs->stackoperands[prs->sopnd--];
}

anst_val anst_peek(parser *const prs)
{
	return prs->anst;
}


void binop(parser *const prs, size_t sp)
{
	const item_t op = prs->stackop[sp];
	const item_t right = anst_pop(prs);
	const item_t left = anst_pop(prs);

	if (mode_is_pointer(prs->sx, left) || mode_is_pointer(prs->sx, right))
	{
		parser_error(prs, operand_is_pointer);
	}

	if (mode_is_float(left) || mode_is_float(right))
	{
		if (op == LOGOR || op == LOGAND || op == LOR || op == LEXOR || op == LAND
			|| op == LSHR || op == LSHL || op == LREM)
		{
			parser_error(prs, int_op_for_float);
		}

		if (mode_is_int(left))
		{
			to_tree(prs, WIDEN1);
		}
		else if (mode_is_int(right))
		{
			to_tree(prs, WIDEN);
		}

		prs->ansttype = LFLOAT;
	}

	if (op == LOGOR || op == LOGAND)
	{
		to_tree(prs, op);
		vector_set(&TREE, prs->stacklog[sp], (item_t)vector_size(&TREE));
		vector_increase(&TREE, 1);
	}
	else
	{
		totree_float_operation(prs, op);
	}
	if (op >= EQEQ && op <= LGE)
	{
		prs->ansttype = LINT;
	}

	 anst_push(prs, value, prs->ansttype);
}

void toval(parser *const prs)
{
	if (anst_peek(prs) == value || anst_peek(prs) == number)
	{
		return;
	}

	if (mode_is_struct(prs->sx, prs->ansttype))
	{
		if (!prs->flag_in_assignment)
		{
			if (anst_peek(prs) == variable)
			{
				node_set_type(&prs->nd, COPY0ST);
				node_set_arg(&prs->nd, 0, prs->anstdispl);
			}
			else // тут может быть только address
			{
				to_tree(prs, COPY1ST);
			}

			node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)prs->ansttype + 1));
			prs->anst = value;
		}
		return;
	}

	if (anst_peek(prs) == variable)
	{
		node_set_type(&prs->nd, mode_is_float(prs->ansttype) ? TIdenttovald : TIdenttoval);
	}

	if (anst_peek(prs) == address && !mode_is_array(prs->sx, prs->ansttype) && !mode_is_pointer(prs->sx, prs->ansttype))
	{
		to_tree(prs, mode_is_float(prs->ansttype) ? TAddrtovald : TAddrtoval);
	}
	prs->anst = value;
}

void parse_braced_init_list(parser *const prs, const item_t type)
{
	token_consume(prs);
	to_tree(prs, type == mode_float ? TStringd : TString);
	node nd_init_list = prs->nd;
	node_add_arg(&nd_init_list, 0);

	size_t length = 0;
	do
	{
		const int sign = token_try_consume(prs, minus) ? -1 : 1;

		if (prs->token == int_constant || prs->token == char_constant)
		{
			if (type == mode_float)
			{
				node_add_double(&nd_init_list, sign * (double)prs->lxr->num);
			}
			else
			{
				node_add_arg(&nd_init_list, sign * prs->lxr->num);
			}
			token_consume(prs);
		}
		else if (prs->token == float_constant)
		{
			if (type == mode_float)
			{
				node_add_double(&nd_init_list, sign * prs->lxr->num_double);
			}
			else
			{
				parser_error(prs, init_int_by_float);
			}
			token_consume(prs);
		}
		else
		{
			parser_error(prs, wrong_init_in_actparam);
			token_skip_until(prs, comma | r_brace | semicolon);
		}
		length++;
	} while (token_try_consume(prs, comma));

	node_set_arg(&nd_init_list, 0, length);
	if (!token_try_consume(prs, r_brace))
	{
		parser_error(prs, no_comma_or_end);
		token_skip_until(prs, r_brace | semicolon);
	}
	prs->ansttype = (int)to_modetab(prs, mode_array, type);
	prs->anst = value;
}

void mustbestring(parser *const prs)
{
	parse_assignment_expression_internal(prs);
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
	parse_assignment_expression_internal(prs);
	if (prs->was_error == 6)
	{
		prs->was_error = 5;
		return; // 1
	}
	toval(prs);
	prs->sopnd--;
	if (!(mode_is_pointer(prs->sx, prs->ansttype) &&
		  mode_is_string(prs->sx, mode_get(prs->sx, (size_t)prs->ansttype + 1))))
	{
		parser_error(prs, not_point_string_in_stanfunc);
		prs->was_error = 5;
		return; // 1
	}
}

void mustberow(parser *const prs)
{
	parse_assignment_expression_internal(prs);
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
	parse_assignment_expression_internal(prs);
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
		parse_braced_init_list(prs, LINT), to_tree(prs, TExprend);
		if (prs->was_error == 2)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	else
	{
		parse_assignment_expression_internal(prs);
		if (prs->was_error == 6)
		{
			prs->was_error = 5;
			return; // 1
		}
		toval(prs);
		prs->sopnd--;
		if (prs->ansttype == LINT || prs->ansttype == LCHAR)
		{
			to_tree(prs, ROWING);
			prs->ansttype = (int)to_modetab(prs, mode_array, LINT);
		}
	}
	if (!(mode_is_array(prs->sx, prs->ansttype) &&
		  mode_is_int(mode_get(prs->sx, (size_t)prs->ansttype + 1))))
	{
		parser_error(prs, not_rowofint_in_stanfunc);
		prs->was_error = 5;
	}
}

void mustberowoffloat(parser *const prs)
{
	if (prs->token == BEGIN)
	{
		parse_braced_init_list(prs, LFLOAT), to_tree(prs, TExprend);
		if (prs->was_error == 2)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	else
	{
		parse_assignment_expression_internal(prs);
		if (prs->was_error == 6)
		{
			prs->was_error = 5;
			return; // 1
		}
		toval(prs);
		prs->sopnd--;
		if (prs->ansttype == LFLOAT)
		{
			to_tree(prs, ROWINGD);
			prs->ansttype = (int)to_modetab(prs, mode_array, LFLOAT);
		}
	}

	if (!(mode_is_array(prs->sx, prs->ansttype) &&
		  mode_get(prs->sx, (size_t)prs->ansttype + 1) == LFLOAT))
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
					parse_assignment_expression_internal(prs);
					if (prs->was_error == 6)
					{
						prs->was_error = 4;
						return; // 1
					}
					toval(prs);
					prs->sopnd--;
					if (mode_is_int(prs->ansttype))
					{
						to_tree(prs, WIDEN);
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
			prs->anst = value;
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
				const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, prs->lxr->repr));
					prs->was_error = 5;
				}

				prs->lastid = (size_t)id;
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
					to_tree(prs, TIdenttoval);
					node_add_arg(&prs->nd, -dn);
				}
				else
				{
					to_tree(prs, TConst);
					node_add_arg(&prs->nd, dn);
				}
				prs->anst = value;
			}
			else
			{
				prs->leftansttype = 2;
				parse_assignment_expression_internal(prs);
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
						prs->anst = value,
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
		// Здесь была проблема в том, что вызов toval() после парсинга этой функции
		// перезаписывала id идентификатора в узле TIdent
		// Намеренно ли при разборе стандартных функций не устанавливается prs->anst?
		prs->anst = value;
		prs->ansttype = prs->stackoperands[++prs->sopnd] = LFLOAT;
	}
	else if (func == ROUND)
	{
		parse_assignment_expression_internal(prs);
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
		parse_assignment_expression_internal(prs);
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
				parse_assignment_expression_internal(prs);
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
					--prs->sopnd, prs->anst = value;
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
				to_tree(prs, WIDEN);
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
	to_tree(prs, 9500 - func);
	must_be(prs, RIGHTBR, no_rightbr_in_stand_func);
}

/**
 *	Parse identifier [C99 6.5.1p1]
 *
 *	@param	prs			Parser structure
 *
 *	@return	Index of parsed identifier in identifiers table
 */
size_t parse_identifier(parser *const prs)
{
	const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
	if (id == ITEM_MAX)
	{
		parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, prs->lxr->repr));
	}

	to_tree(prs, TIdent);
	prs->anstdispl = (int)ident_get_displ(prs->sx, (size_t)id);
	node_add_arg(&prs->nd, prs->anstdispl);

	const item_t mode = ident_get_mode(prs->sx, (size_t)id);

	prs->lastid = (size_t)id;
	token_consume(prs);
	anst_push(prs, variable, mode);
	return (size_t)id;
}

/**
 *	Parse constant [C99 6.5.1p2]
 *
 *	constant:
 *		character-constant
 *		integer-constant
 *		floating-constant
 *
 *	@param	prs			Parser structure
 *
 *	@return	Type of parsed expression
 */
item_t parse_constant(parser *const prs)
{
	item_t mode = mode_undefined;
	switch (prs->token)
	{
		case char_constant:
			to_tree(prs, TConst);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_character;
			break;

		case int_constant:
			to_tree(prs, TConst);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_integer;
			break;

		case float_constant:
			to_tree(prs, TConstd);
			node_set_double(&prs->nd, 0, prs->lxr->num_double);
			mode = mode_float;
			break;

		default:
			break;
	}

	token_consume(prs);
	return anst_push(prs, number, mode);
}

/**
 *	Parse primary expression [C99 6.5.1]
 *
 *	primary-expression:
 *		identifier
 *		constant
 *		string-literal
 *		'(' expression ')'
 *		standard-function-call [RuC]
 *
 *	@param	prs			Parser structure
 */
void parse_primary_expression(parser *const prs)
{
	switch (prs->token)
	{
		case identifier:
			parse_identifier(prs);
			break;

		case char_constant:
		case int_constant:
		case float_constant:
			parse_constant(prs);
			break;

		case string_literal:
			parse_string_literal(prs, &prs->nd);
			break;

		case l_paren:
			token_consume(prs);
			if (token_try_consume(prs, kw_void))
			{
				token_expect_and_consume(prs, star, no_mult_in_cast);
				parse_unary_expression(prs);
				if (!mode_is_pointer(prs->sx, prs->ansttype))
				{
					parser_error(prs, not_pointer_in_cast);
				}
				token_expect_and_consume(prs, r_paren, no_rightbr_in_cast);
				toval(prs);
				to_tree(prs, TExprend);
			}
			else
			{
				const size_t oldsp = prs->sp;
				parse_expression_internal(prs);
				token_expect_and_consume(prs, r_paren, wait_rightbr_in_primary);
				while (prs->sp > oldsp)
				{
					binop(prs, --prs->sp);
				}
			}
			break;

		default:
			if (prs->token <= STANDARD_FUNC_START)
			{
				parse_standard_function_call(prs);
				break;
			}
			else
			{
				token_consume(prs);
				parser_error(prs, expected_expression, prs->token);
				anst_push(prs, number, mode_undefined);
				break;
			}
	}
}

item_t find_field(parser *const prs, const item_t stype)
{
	token_consume(prs);
	token_expect_and_consume(prs, identifier, after_dot_must_be_ident);

	item_t select_displ = 0;
	const size_t record_length = (size_t)mode_get(prs->sx, (size_t)stype + 2);
	for (size_t i = 0; i < record_length; i += 2) // тут хранится удвоенное n
	{
		const item_t field_type = mode_get(prs->sx, (size_t)stype + 3 + i);

		if ((size_t)mode_get(prs->sx, (size_t)stype + 4 + i) == prs->lxr->repr)
		{
			const anst_val peek = anst_peek(prs);
			anst_pop(prs);
			anst_push(prs, peek, field_type);
			return select_displ;
		}
		else
		{
			select_displ += (item_t)size_of(prs->sx, field_type);
		}
		// прибавляем к суммарному смещению длину поля
	}

	parser_error(prs, no_field, repr_get_name(prs->sx, prs->lxr->repr));
	return 0;
}

/**
 *	Parse function call [C99 6.5.2]
 *
 *	postfix-expression '(' argument-expression-listopt ')'
 *
 *	argument-expression-list:
 *		assignment-expression
 *		argument-expression-list ',' assignment-expression
 *
 *	@param	prs			Parser structure
 *	@param	function_id	Index of function identifier in identifiers table
 */
void parse_function_call(parser *const prs, const size_t function_id)
{
	int old_in_assignment = prs->flag_in_assignment;
	const size_t function_mode = (size_t)anst_pop(prs);

	if (!mode_is_function(prs->sx, function_mode))
	{
		parser_error(prs, call_not_from_function);
		token_skip_until(prs, r_paren | semicolon);
		return;
	}

	const size_t expected_args = (size_t)mode_get(prs->sx, function_mode + 2);
	size_t actual_args = 0;

	to_tree(prs, TCall1);
	node nd_call = prs->nd;
	node_add_arg(&nd_call, expected_args);
	size_t ref_arg_mode = function_mode + 3;

	if (token_try_consume(prs, r_paren))
	{
		actual_args = 0;
	}
	else
	{
		do
		{
			prs->nd = nd_call;
			const item_t expected_arg_mode = mode_get(prs->sx, ref_arg_mode);

			if (mode_is_function(prs->sx, expected_arg_mode))
			{
				if (prs->token != identifier)
				{
					parser_error(prs, act_param_not_ident);
					token_skip_until(prs, comma | r_paren | semicolon);
					continue;
				}

				const size_t id = parse_identifier(prs);

				if (ident_get_mode(prs->sx, id) != expected_arg_mode)
				{
					parser_error(prs, diff_formal_param_type_and_actual);
					token_skip_until(prs, comma | r_paren | semicolon);
					continue;
				}

				const item_t displ = ident_get_displ(prs->sx, id);
				node_set_type(&prs->nd, displ < 0 ? TIdenttoval : TConst);
				node_set_arg(&prs->nd, 0, llabs(displ));
				to_tree(prs, TExprend);
			}
			else if (mode_is_array(prs->sx, expected_arg_mode) && prs->token == l_brace)
			{
				parse_braced_init_list(prs, mode_get(prs->sx, (size_t)expected_arg_mode + 1));
				to_tree(prs, TExprend);
			}
			else
			{
				prs->flag_in_assignment = 0;
				const item_t actual_arg_mode = parse_assignment_expression(prs, &prs->nd);

				if (!mode_is_undefined(expected_arg_mode) && !mode_is_undefined(actual_arg_mode))
				{
					if (mode_is_int(expected_arg_mode) && mode_is_float(actual_arg_mode))
					{
						parser_error(prs, float_instead_int);
					}
					else if (mode_is_float(expected_arg_mode) && mode_is_int(actual_arg_mode))
					{
						parse_insert_widen(prs);
					}
					else if (expected_arg_mode != actual_arg_mode)
					{
						parser_error(prs, diff_formal_param_type_and_actual);
					}
				}
			}

			actual_args++;
			ref_arg_mode++;
		} while (token_try_consume(prs, comma));

		token_expect_and_consume(prs, r_paren, wrong_number_of_params);
	}

	if (expected_args != actual_args)
	{
		parser_error(prs, wrong_number_of_params);
	}

	prs->flag_in_assignment = old_in_assignment;
	node nd_call2 = node_add_child(&nd_call, TCall2);
	node_add_arg(&nd_call2, (item_t)function_id);
	prs->nd = nd_call2;
	anst_push(prs, value, mode_get(prs->sx, function_mode + 1));
}

/**
 *	Parse postfix expression [C99 6.5.2]
 *
 *	postfix-expression:
 *		primary-expression
 *		postfix-expression '[' expression ']'
 *		postfix-expression '(' argument-expression-listopt ')'
 *		postfix-expression '.' identifier
 *		postfix-expression '->' identifier
 *		postfix-expression '++'
 *		postfix-expression '--'
 *
 *	@param	prs			Parser structure
 */
void parse_postfix_expression(parser *const prs)
{
	int was_func = 0;
	const size_t lid = prs->lastid;

	if (token_try_consume(prs, l_paren))
	{
		was_func = 1;
		parse_function_call(prs, lid);
	}

	while (prs->token == l_square || prs->token == arrow || prs->token == period)
	{
		while (token_try_consume(prs, l_square))
		{
			if (was_func)
			{
				parser_error(prs, slice_from_func);
			}

			if (anst_peek(prs) == variable)
			{
				node_set_type(&prs->nd, TSliceident);
				node_set_arg(&prs->nd, 0, prs->anstdispl);
			}
			else
			{
				to_tree(prs, TSlice);
			}

			const item_t anst_mode = anst_pop(prs);
			if (!mode_is_array(prs->sx, anst_mode))
			{
				parser_error(prs, slice_not_from_array);
			}

			const item_t elem_type = mode_get(prs->sx, (size_t)anst_mode + 1);
			node_add_arg(&prs->nd, elem_type);

			const item_t index_type = parse_condition(prs, &prs->nd);
			if (!mode_is_int(index_type))
			{
				parser_error(prs, index_must_be_int);
			}

			token_expect_and_consume(prs, r_square, no_rightsqbr_in_slice);
			anst_push(prs, address, elem_type);
		}

		while (prs->token == arrow)
		{
			if (!mode_is_pointer(prs->sx, prs->ansttype) ||
				!mode_is_struct(prs->sx, mode_get(prs->sx, (size_t)prs->ansttype + 1)))
			{
				parser_error(prs, get_field_not_from_struct_pointer);
			}

			if (prs->anst == variable)
			{
				node_set_type(&prs->nd, TIdenttoval);
			}
			prs->anst = address;
			to_tree(prs, TSelect);

			prs->ansttype = mode_get(prs->sx, (size_t)prs->ansttype + 1);
			prs->anstdispl = find_field(prs, prs->ansttype);
			while (prs->token == period)
			{
				prs->anstdispl += find_field(prs, prs->ansttype);
			}

			to_tree(prs, prs->anstdispl);
			if (mode_is_array(prs->sx, prs->ansttype) || mode_is_pointer(prs->sx, prs->ansttype))
			{
				to_tree(prs, TAddrtoval);
			}
		}

		if (prs->token == period)
		{
			if (!mode_is_struct(prs->sx, prs->ansttype))
			{
				parser_error(prs, select_not_from_struct);
			}

			if (anst_peek(prs) == value)
			{
				const size_t len = size_of(prs->sx, prs->ansttype);
				prs->anstdispl = 0;
				while (prs->token == period)
				{
					prs->anstdispl += find_field(prs, prs->ansttype);
				}
				to_tree(prs, COPYST);
				node_add_arg(&prs->nd, prs->anstdispl);
				node_add_arg(&prs->nd, (item_t)size_of(prs->sx, prs->ansttype));
				node_add_arg(&prs->nd, (item_t)len);
			}
			else if (anst_peek(prs) == variable)
			{
				int globid = prs->anstdispl < 0 ? -1 : 1;
				while (prs->token == period)
				{
					prs->anstdispl += globid * find_field(prs, (int)prs->ansttype);
				}

				node_set_arg(&prs->nd, 0, prs->anstdispl);
			}
			else //if (anst_peek(prs) == address)
			{
				to_tree(prs, TSelect);
				prs->anstdispl = 0;
				while (prs->token == period)
				{
					prs->anstdispl += find_field(prs, prs->ansttype);
				}

				node_add_arg(&prs->nd, prs->anstdispl);
				if (mode_is_array(prs->sx, prs->ansttype) || mode_is_pointer(prs->sx, prs->ansttype))
				{
					to_tree(prs, TAddrtoval);
				}
			}
		}
	}

	if (prs->token == plusplus || prs->token == minusminus)
	{
		int operator = (prs->token == plusplus) ? POSTINC : POSTDEC;
		token_consume(prs);

		if (!mode_is_int(prs->ansttype) && !mode_is_float(prs->ansttype))
		{
			parser_error(prs, wrong_operand);
		}

		if (anst_peek(prs) != variable && anst_peek(prs) != address)
		{
			parser_error(prs, unassignable_inc);
		}

		if (anst_peek(prs) == address)
		{
			operator += 4;
		}

		totree_float_operation(prs, operator);

		if (anst_peek(prs) == variable)
		{
			node_add_arg(&prs->nd, ident_get_displ(prs->sx, lid));
		}

		prs->anst = value;
	}
}

void parse_unary_expression(parser *const prs)
{
	token_t operator = prs->token;
	switch (operator)
	{
		case plusplus:
		case minusminus:
		{
			token_consume(prs);
			parse_unary_expression(prs);

			if (anst_peek(prs) != variable && anst_peek(prs) != address)
			{
				parser_error(prs, unassignable_inc);
			}

			if (anst_peek(prs) == address)
			{
				operator += 4;
			}

			totree_float_operation(prs, operator);

			if (anst_peek(prs) == variable)
			{
				node_add_arg(&prs->nd, ident_get_displ(prs->sx, prs->lastid));
			}

			anst_push(prs, value, anst_pop(prs));
		}
		break;

		case exclaim:
		case tilde:
		case plus:
		case minus:
		case amp:
		case star:
		{
			token_consume(prs);
			parse_unary_expression(prs);
			switch (operator)
			{
				case amp:
				{
					if (anst_peek(prs) == value)
					{
						parser_error(prs, wrong_addr);
					}

					if (anst_peek(prs) == variable)
					{
						node_set_type(&prs->nd, TIdenttoaddr);
					}

					anst_push(prs, value, to_modetab(prs, mode_pointer, anst_pop(prs)));
				}
				break;

				case star:
				{
					if (anst_peek(prs) == variable)
					{
						node_set_type(&prs->nd, TIdenttoval);
					}

					const item_t type = anst_pop(prs);
					if (!mode_is_pointer(prs->sx, type))
					{
						parser_error(prs, aster_not_for_pointer);
					}

					anst_push(prs, address, mode_get(prs->sx, (size_t)type + 1));
				}
				break;

				default:
				{
					toval(prs);
					if (operator == minus)
					{
						if (node_get_type(&prs->nd) == TConst)
						{
							node_set_arg(&prs->nd, 0, -node_get_arg(&prs->nd, 0));
						}
						else if (node_get_type(&prs->nd) == TConstd)
						{
							node_set_double(&prs->nd, 0, -node_get_double(&prs->nd, 0));
						}
						else
						{
							totree_float_operation(prs, UNMINUS);
						}
					}
					else
					{
						if (operator != plus)
						{
							to_tree(prs, operator);
						}

						const item_t type = anst_pop(prs);
						if ((operator == tilde || operator == exclaim) && mode_is_float(type))
						{
							parser_error(prs, int_op_for_float);
						}

						anst_push(prs, value, type);
					}
				}
				break;
			}
		}
		break;

		default:
			parse_primary_expression(prs);
			break;
	}

	parse_postfix_expression(prs);
}

int operator_precedence(const token_t operator)
{
	switch (operator)
	{
		case pipepipe:			// '||'
			return 1;

		case ampamp:			// '&&'
			return 2;

		case pipe:				// '|'
			return 3;

		case caret:				// '^'
			return 4;

		case amp:				// '&'
			return 5;

		case equalequal:		// '=='
		case exclaimequal:		// '!='
			return 6;

		case less:				// '<'
		case greater:			// '<'
		case lessequal:			// '<='
		case greaterequal:		// '>='
			return 7;

		case lessless:			// '<<'
		case greatergreater:	// '>>'
			return 8;

		case plus:				// '+'
		case minus:				// '-'
			return 9;

		case star:				// '*'
		case slash:				// '/'
		case percent:			// '%'
			return 10;

		default:
			return 0;
	}
}

int is_int_assignment_operator(const item_t operator)
{
	switch (operator)
	{
		case percentequal:			// '%='
		case lesslessequal:			// '<<='
		case greatergreaterequal:	// '>>='
		case ampequal:				// '&='
		case pipeequal:				// '|='
		case caretequal:			// '^='
			return 1;

		default:
			return 0;
	}
}


int is_assignment_operator(const token_t operator)
{
	return operator == equal || operator == starequal || operator == slashequal || operator == plusequal
		|| operator == minusequal || is_int_assignment_operator(operator);
}

void parse_subexpression(parser *const prs)
{
	size_t oldsp = prs->sp;
	int wasop = 0;

	int p = operator_precedence(prs->token);
	while (p)
	{
		wasop = 1;
		toval(prs);
		while (prs->sp > oldsp && prs->stack[prs->sp - 1] >= p)
		{
			binop(prs, --prs->sp);
		}

		size_t ad = 0;
		if (p <= 2)
		{
			to_tree(prs, p == 1 ? ADLOGOR : ADLOGAND);
			ad = vector_size(&TREE);
			vector_increase(&TREE, 1);
		}

		prs->stack[prs->sp] = p;
		prs->stacklog[prs->sp] = (int)ad;
		prs->stackop[prs->sp++] = prs->token;
		token_consume(prs);
		parse_unary_expression(prs);
		p = operator_precedence(prs->token);
	}
	if (wasop)
	{
		toval(prs);
	}
	while (prs->sp > oldsp)
	{
		binop(prs, --prs->sp);
	}
}

void parse_conditional_expression(parser *const prs)
{
	parse_subexpression(prs); // logORexpr();

	if (prs->token == question)
	{
		item_t globtype = 0;
		size_t adif = 0;

		while (token_try_consume(prs, question))
		{
			toval(prs);
			if (!mode_is_int(anst_pop(prs)))
			{
				parser_error(prs, float_in_condition);
			}

			to_tree(prs, TCondexpr);
			node nd_condexpr = prs->nd;
			const item_t expr_type = parse_condition(prs, &nd_condexpr); // then

			if (!globtype)
			{
				globtype = expr_type;
			}

			if (mode_is_float(expr_type))
			{
				globtype = mode_float;
			}
			else
			{
				vector_add(&TREE, (item_t)adif);
				adif = vector_size(&TREE) - 1;
			}

			token_expect_and_consume(prs, colon, no_colon_in_cond_expr);
			prs->nd = nd_condexpr;
			parse_unary_expression(prs);
			parse_subexpression(prs); // logORexpr();	else or elif
		}

		toval(prs);
		to_tree(prs, TExprend);

		if (mode_is_float(anst_pop(prs)))
		{
			globtype = mode_float;
		}
		else
		{
			vector_add(&TREE, (item_t)adif);
			adif = vector_size(&TREE) - 1;
		}

		while (adif != 0 && adif <= vector_size(&TREE))
		{
			item_t r = vector_get(&TREE, adif);
			vector_set(&TREE, adif, TExprend);
			vector_set(&TREE, adif - 1, mode_is_float(globtype) ? WIDEN : NOP);
			adif = (size_t)r;
		}

		anst_push(prs, value, globtype);
	}
}

void assignment_to_void(parser *const prs)
{
	const item_t operation = node_get_type(&prs->nd);
	if ((operation >= ASS && operation <= DIVASSAT) || (operation >= POSTINC && operation <= DECAT)
		|| (operation >= ASSR && operation <= DIVASSATR) || (operation >= POSTINCR && operation <= DECATR))
	{
		node_set_type(&prs->nd, node_get_type(&prs->nd) + 200);
	}
}

void parse_assignment_expression_internal(parser *const prs)
{
	if (prs->token == l_brace)
	{
		const item_t type = prs->leftansttype;
		if (mode_is_struct(prs->sx, type) || mode_is_array(prs->sx, type))
		{
			parse_initializer(prs, &prs->nd, type);
			prs->leftansttype = type;
		}
		else
		{
			parser_error(prs, init_not_struct);
		}

		anst_push(prs, value, type);
	}
	else
	{
		parse_unary_expression(prs);
	}

	prs->leftansttype = prs->ansttype;

	if (is_assignment_operator(prs->token))
	{
		const anst_val leftanst = anst_peek(prs);
		const item_t leftanstdispl = prs->anstdispl;
		item_t operator = prs->token;
		token_consume(prs);

		prs->flag_in_assignment = 1;
		parse_assignment_expression_internal(prs);
		prs->flag_in_assignment = 0;

		if (leftanst == value)
		{
			parser_error(prs, unassignable);
		}
		// Снимаем типы операндов со стека
		const item_t rtype = anst_pop(prs);
		const item_t ltype = anst_pop(prs);

		if (is_int_assignment_operator(operator) && (mode_is_float(ltype) || mode_is_float(rtype)))
		{
			parser_error(prs, int_op_for_float);
		}
		else if (mode_is_array(prs->sx, ltype))
		{
			parser_error(prs, array_assigment);
		}
		else if (mode_is_struct(prs->sx, ltype))
		{
			if (ltype != rtype) // типы должны быть равны
			{
				parser_error(prs, type_missmatch);
			}

			if (operator != equal) // в структуру можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
			}

			if (prs->anst == value)
			{
				operator = leftanst == variable ? COPY0STASS : COPY1STASS;
			}
			else
			{
				operator = leftanst == variable ? prs->anst == variable ? COPY00 : COPY01
				: prs->anst == variable ? COPY10 : COPY11;
			}

			to_tree(prs, operator);
			if (leftanst == variable)
			{
				node_add_arg(&prs->nd, leftanstdispl);
			}
			if (prs->anst == variable)
			{
				node_add_arg(&prs->nd, prs->anstdispl);
			}
			node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)ltype + 1));
			prs->anstdispl = leftanstdispl;
			anst_push(prs, leftanst, ltype);
		}
		else // оба операнда базового типа или указатели
		{
			if (mode_is_pointer(prs->sx, ltype) && operator != equal) // в указатель можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
			}

			if (mode_is_int(ltype) && mode_is_float(rtype))
			{
				parser_error(prs, assmnt_float_to_int);
			}

			toval(prs);
			if (mode_is_int(rtype) && mode_is_float(ltype))
			{
				to_tree(prs, WIDEN);
				prs->ansttype = LFLOAT;
			}
			if (mode_is_pointer(prs->sx, ltype) && mode_is_pointer(prs->sx, rtype) && ltype != rtype)
			{
				// проверка нужна только для указателей
				parser_error(prs, type_missmatch);
			}

			if (leftanst == address)
			{
				operator += 11;
			}
			totree_float_operation(prs, operator);
			if (leftanst == variable)
			{
				prs->anstdispl = leftanstdispl;
				node_add_arg(&prs->nd, leftanstdispl);
			}
			anst_push(prs, value, ltype);
		}
	}
	else
	{
		// Эта функция учитывает тот факт, что начало в виде унарного выражения уже выкушано
		parse_conditional_expression(prs);
	}
}

void parse_expression_internal(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	while (token_try_consume(prs, comma))
	{
		assignment_to_void(prs);
		anst_pop(prs);
		parse_assignment_expression_internal(prs);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


item_t parse_expression(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	parse_expression_internal(prs);
	assignment_to_void(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_assignment_expression(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	parse_assignment_expression_internal(prs);
	toval(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_parenthesized_expression(parser *const prs, node *const parent)
{
	token_expect_and_consume(prs, l_paren, cond_must_be_in_brkts);
	const item_t condition_type = parse_condition(prs, parent);
	token_expect_and_consume(prs, r_paren, cond_must_be_in_brkts);
	return condition_type;
}

item_t parse_constant_expression(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	parse_unary_expression(prs);
	parse_conditional_expression(prs);
	toval(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_condition(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	parse_expression_internal(prs);
	toval(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_string_literal(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	to_tree(prs, TString);
	node_add_arg(&prs->nd, prs->lxr->num);

	for (size_t i = 0; i < (size_t)prs->lxr->num; i++)
	{
		node_add_arg(&prs->nd, prs->lxr->lexstr[i]);
	}

	token_consume(prs);
	return anst_push(prs, value, to_modetab(prs, mode_array, mode_character));
}

void parse_insert_widen(parser *const prs)
{
	// Сейчас последней нодой в поддереве выражения является TExprend, просто меняем ее тип
	node_set_type(&prs->nd, WIDEN);
	to_tree(prs, TExprend);
}
