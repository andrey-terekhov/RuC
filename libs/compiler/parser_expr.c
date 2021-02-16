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
#include <string.h>


void unarexpr(parser *const prs);
void exprassn(parser *const prs, int level);
void expr(parser *const prs, int level);


int scanner(parser *const prs)
{
	prs->curr_token = prs->next_token;

	if (!prs->buf_flag)
	{
		prs->next_token = lex(prs->lxr);
	}
	else
	{
		prs->next_token = prs->buf_cur;
		prs->buf_flag--;
	}

	return prs->curr_token;
}

void must_be(parser *const prs, int what, int e)
{
	if (prs->next_token != what)
	{
		parser_error(prs, e);
		prs->curr_token = what;
	}
	else
	{
		scanner(prs);
	}
}

void applid(parser *const prs)
{
	prs->lastid = REPRTAB[REPRTAB_POS + 1];
	if (prs->lastid == 1)
	{
		char buffer[MAXSTRINGL];
		repr_get_name(prs->sx, REPRTAB_POS, buffer);
		parser_error(prs, ident_is_not_declared, buffer);
		prs->was_error = 5;
	}
}

void totree(parser *const prs, item_t op)
{
	vector_add(&TREE, op);
}

void totree_float_operation(parser *const prs, item_t op)
{
	if (prs->ansttype == LFLOAT &&
		((op >= ASS && op <= DIVASS) || (op >= ASSAT && op <= DIVASSAT) || (op >= EQEQ && op <= UNMINUS)))
	{
		tree_add(prs->sx, op + 50);
	}
	else
	{
		tree_add(prs->sx, op);
	}
}

int double_to_tree(vector *const tree, const double num)
{
	int64_t num64;
	memcpy(&num64, &num, sizeof(int64_t));

	int32_t fst = num64 & 0x00000000ffffffff;
	int32_t snd = (num64 & 0xffffffff00000000) >> 32;

#if INT_MIN < ITEM_MIN || INT_MAX > ITEM_MAX
	if (fst < ITEM_MIN || fst > ITEM_MAX || snd < ITEM_MIN || snd > ITEM_MAX)
	{
		return -1;
	}
#endif

	size_t ret = vector_add(tree, fst);
	ret = ret != SIZE_MAX ? vector_add(tree, snd) : SIZE_MAX;
	return ret == SIZE_MAX;
}

double double_from_tree(vector *const tree)
{
	const size_t index = vector_size(tree) - 2;

	const int64_t fst = (int64_t)vector_get(tree, index) & 0x00000000ffffffff;
	const int64_t snd = (int64_t)vector_get(tree, index + 1) & 0x00000000ffffffff;
	int64_t num64 = (snd << 32) | fst;

	vector_remove(tree);
	vector_remove(tree);

	double num;
	memcpy(&num, &num64, sizeof(double));
	return num;
}

void binop(parser *const prs, int sp)
{
	int op = prs->stackop[sp];
	int right = prs->stackoperands[prs->sopnd--];
	int left = prs->stackoperands[prs->sopnd];

	if (mode_is_pointer(prs->sx, left) || mode_is_pointer(prs->sx, right))
	{
		parser_error(prs, operand_is_pointer);
		prs->was_error = 5;
		return; // 1
	}

	if (mode_is_float(left) || mode_is_float(right))
	{
		if (op == LOGOR || op == LOGAND || op == LOR || op == LEXOR || op == LAND
			|| op == LSHR || op == LSHL || op == LREM)
		{
			parser_error(prs, int_op_for_float);
			prs->was_error = 5;
			return; // 1
		}

		if (mode_is_int(left))
		{
			tree_add(prs->sx, WIDEN1);
		}
		else if (mode_is_int(right))
		{
			tree_add(prs->sx, WIDEN);
		}

		prs->ansttype = LFLOAT;
	}

	if (op == LOGOR || op == LOGAND)
	{
		totree(prs, op);
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

	prs->stackoperands[prs->sopnd] = prs->ansttype;
	prs->anst = VAL;
}

void toval(parser *const prs)
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
		tree_set(prs->sx, tree_size(prs->sx) - 2, mode_is_float(prs->ansttype) ? TIdenttovald : TIdenttoval);
	}

	if (!mode_is_array(prs->sx, prs->ansttype) && !mode_is_pointer(prs->sx, prs->ansttype) && prs->anst == ADDR)
	{
		totree(prs, mode_is_float(prs->ansttype) ? TAddrtovald : TAddrtoval);
	}
	prs->anst = VAL;
}

void actstring(int type, parser *const prs)
{
	scanner(prs);
	totree(prs, type == LFLOAT ? TStringd : TString);
	size_t adn = vector_size(&TREE);
	vector_increase(&TREE, 1);

	int n = 0;
	do
	{
		exprassn(prs, 1);
		if (prs->was_error == 6)
		{
			prs->was_error = 1;
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
			parser_error(prs, wrong_init_in_actparam);
			prs->was_error = 1;
			return; // 1
		}
		++n;
	} while (scanner(prs) == COMMA ? scanner(prs), 1 : 0);

	vector_set(&TREE, adn, n);
	if (prs->curr_token != END)
	{
		parser_error(prs, no_comma_or_end);
		prs->was_error = 1;
		return; // 1
	}
	prs->ansttype = (int)to_modetab(prs, mode_array, type);
	prs->anst = VAL;
}

void mustbestring(parser *const prs)
{
	scanner(prs);
	exprassn(prs, 1);
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
	scanner(prs);
	exprassn(prs, 1);
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
	scanner(prs);
	exprassn(prs, 1);
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
	scanner(prs);
	exprassn(prs, 1);
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
	if (scanner(prs) == BEGIN)
	{
		actstring(LINT, prs), totree(prs, TExprend);
		if (prs->was_error == 2)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(prs, 1);
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
	if (scanner(prs) == BEGIN)
	{
		actstring(LFLOAT, prs), totree(prs, TExprend);
		if (prs->was_error == 2)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	else
	{
		exprassn(prs, 1);
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

void primaryexpr(parser *const prs)
{
	if (prs->curr_token == CHAR_CONST)
	{
		totree(prs, TConst);
		totree(prs, prs->lxr->num);
		prs->stackoperands[++prs->sopnd] = prs->ansttype = LCHAR;
		prs->anst = NUMBER;
	}
	else if (prs->curr_token == INT_CONST)
	{
		totree(prs, TConst);
		totree(prs, prs->lxr->num);
		prs->stackoperands[++prs->sopnd] = prs->ansttype = LINT;
		prs->anst = NUMBER;
	}
	else if (prs->curr_token == FLOAT_CONST)
	{
		totree(prs, TConstd);
		double_to_tree(&TREE, prs->lxr->num_double);
		prs->stackoperands[++prs->sopnd] = prs->ansttype = LFLOAT;
		prs->anst = NUMBER;
	}
	else if (prs->curr_token == STRING)
	{
		parse_string_literal(prs);
	}
	else if (prs->curr_token == IDENT)
	{
		applid(prs);
		if (prs->was_error == 5)
		{
			prs->was_error = 4;
			return; // 1
		}

		totree(prs, TIdent);
		prs->anstdispl = (int)ident_get_displ(prs->sx, prs->lastid);
		totree(prs, prs->anstdispl);
		prs->ansttype = (int)ident_get_mode(prs->sx, prs->lastid);
		prs->stackoperands[++prs->sopnd] = prs->ansttype;
		prs->anst = IDENT;
	}
	else if (prs->curr_token == LEFTBR)
	{
		if (prs->next_token == LVOID)
		{
			scanner(prs);
			must_be(prs, LMULT, no_mult_in_cast);
			unarexpr(prs);
			if (prs->was_error == 7)
			{
				prs->was_error = 4;
				return; // 1
			}
			if (!mode_is_pointer(prs->sx, prs->ansttype))
			{
				parser_error(prs, not_pointer_in_cast);
				prs->was_error = 4;
				return; // 1
			}
			must_be(prs, RIGHTBR, no_rightbr_in_cast);
			toval(prs);
			// totree(context, CASTC);
			totree(prs, TExprend);
		}
		else
		{
			int oldsp = prs->sp;
			scanner(prs);
			expr(prs, 1);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
			must_be(prs, RIGHTBR, wait_rightbr_in_primary);
			while (prs->sp > oldsp)
			{
				binop(prs, --prs->sp);
			}
		}
	}
	else if (prs->curr_token <= STANDARD_FUNC_START) // стандартная функция
	{
		int func = prs->curr_token;

		if (scanner(prs) != LEFTBR)
		{
			parser_error(prs, no_leftbr_in_stand_func);
			prs->buf_cur = prs->next_token;
			prs->next_token = prs->curr_token;
			prs->curr_token = LEFTBR;
			prs->buf_flag++;
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
						scanner(prs);
						exprassn(prs, 1);
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
				scanner(prs);

				if (func == TCREATE)
				{
					item_t dn;

					if (prs->curr_token != IDENT)
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
						totree(prs, -dn);
					}
					else
					{
						totree(prs, TConst);
						totree(prs, dn);
					}
					prs->anst = VAL;
				}
				else
				{
					prs->leftansttype = 2;
					exprassn(prs, 1);
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
			scanner(prs);
			exprassn(prs, 1);
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
			scanner(prs);
			exprassn(prs, 1);
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
					scanner(prs);
					exprassn(prs, 1);
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
	else
	{
		parser_error(prs, not_primary, prs->curr_token);
		prs->was_error = 4;
		return; // 1
	}
	if (prs->was_error == 5)
	{
		prs->was_error = 4;
		return; // 1
	}
}

void index_check(parser *const prs)
{
	if (!mode_is_int(prs->ansttype))
	{
		parser_error(prs, index_must_be_int);
		prs->was_error = 5;
	}
}

int find_field(parser *const prs, int stype)
{
	// выдает смещение до найденного поля или ошибку

	int flag = 1;
	int select_displ = 0;
	const item_t record_length = mode_get(prs->sx, stype + 2);

	scanner(prs);
	must_be(prs, IDENT, after_dot_must_be_ident);

	for (item_t i = 0; i < record_length; i += 2) // тут хранится удвоенное n
	{
		int field_type = (int)mode_get(prs->sx, stype + 3 + (int)i);

		if ((size_t)mode_get(prs->sx, stype + 4 + (int)i) == REPRTAB_POS)
		{
			prs->stackoperands[prs->sopnd] = prs->ansttype = field_type;
			flag = 0;
			break;
		}
		else
		{
			select_displ += (int)size_of(prs->sx, field_type);
		}
		// прибавляем к суммарному смещению длину поля
	}
	if (flag)
	{
		char buffer[MAXSTRINGL];
		repr_get_name(prs->sx, REPRTAB_POS, buffer);
		parser_error(prs, no_field, buffer);
		prs->was_error = 5;
		return 0; // 1
	}
	return select_displ;
}

void selectend(parser *const prs)
{
	while (prs->next_token == DOT)
	{
		prs->anstdispl += find_field(prs, prs->ansttype);
		if (prs->was_error == 6)
		{
			prs->was_error = 5;
			return; // 1
		}
	}

	totree(prs, prs->anstdispl);
	if (mode_is_array(prs->sx, prs->ansttype) || mode_is_pointer(prs->sx, prs->ansttype))
	{
		totree(prs, TAddrtoval);
	}
}

void postexpr(parser *const prs)
{
	int lid;
	int leftansttyp;
	int was_func = 0;

	lid = prs->lastid;
	leftansttyp = prs->ansttype;

	if (prs->next_token == LEFTBR) // вызов функции
	{
		int j;
		item_t n;
		item_t dn;
		int oldinass = prs->flag_in_assignment;

		was_func = 1;
		scanner(prs);
		if (!mode_is_function(prs->sx, leftansttyp))
		{
			parser_error(prs, call_not_from_function);
			prs->was_error = 4;
			return; // 1
		}

		n = mode_get(prs->sx, leftansttyp + 2); // берем количество аргументов функции

		totree(prs, TCall1);
		totree(prs, n);
		j = leftansttyp + 3;
		for (item_t i = 0; i < n; i++) // фактические параметры
		{
			int mdj = prs->leftansttype = (int)mode_get(prs->sx, j); // это вид формального параметра, в
																			 // context->ansttype будет вид фактического
																			 // параметра
			scanner(prs);
			if (mode_is_function(prs->sx, mdj))
			{
				// фактическим параметром должна быть функция, в С - это только идентификатор

				if (prs->curr_token != IDENT)
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
				if ((int)ident_get_mode(prs->sx, prs->lastid) != mdj)
				{
					parser_error(prs, diff_formal_param_type_and_actual);
					prs->was_error = 4;
					return; // 1
				}
				dn = ident_get_displ(prs->sx, prs->lastid);
				if (dn < 0)
				{
					totree(prs, TIdenttoval);
					totree(prs, -dn);
				}
				else
				{
					totree(prs, TConst);
					totree(prs, dn);
				}
				totree(prs, TExprend);
			}
			else
			{
				if (prs->curr_token == BEGIN && mode_is_array(prs->sx, mdj))
				{
					actstring((int)mode_get(prs->sx, mdj + 1), prs), totree(prs, TExprend);
					if (prs->was_error == 2)
					{
						prs->was_error = 4;
						return; // 1
					}
				}
				else
				{
					prs->flag_in_assignment = 0;
					exprassn(prs, 1);
					if (prs->was_error == 6)
					{
						prs->was_error = 4;
						return; // 1
					}
					toval(prs);
					totree(prs, TExprend);

					if (mdj > 0 && mdj != prs->ansttype)
					{
						parser_error(prs, diff_formal_param_type_and_actual);
						prs->was_error = 4;
						return; // 1
					}

					if (mode_is_int(mdj) && mode_is_float(prs->ansttype))
					{
						parser_error(prs, float_instead_int);
						prs->was_error = 4;
						return; // 1
					}

					if (mode_is_float(mdj) && mode_is_int(prs->ansttype))
					{
						parse_insert_widen(prs);
					}
					--prs->sopnd;
				}
			}
			if (i < n - 1 && scanner(prs) != COMMA)
			{
				parser_error(prs, no_comma_in_act_params);
				prs->was_error = 4;
				return; // 1
			}
			j++;
		}
		prs->flag_in_assignment = oldinass;
		must_be(prs, RIGHTBR, wrong_number_of_params);
		totree(prs, TCall2);
		totree(prs, lid);
		prs->stackoperands[prs->sopnd] = prs->ansttype = (int)mode_get(prs->sx, leftansttyp + 1);
		prs->anst = VAL;
	}

	while (prs->next_token == LEFTSQBR || prs->next_token == ARROW || prs->next_token == DOT)
	{
		while (prs->next_token == LEFTSQBR) // вырезка из массива (возможно, многомерного)
		{
			if (was_func)
			{
				parser_error(prs, slice_from_func);
				prs->was_error = 4;
				return; // 1
			}
			if (!mode_is_array(prs->sx, prs->ansttype)) // вырезка не из массива
			{
				parser_error(prs, slice_not_from_array);
				prs->was_error = 4;
				return; // 1
			}

			item_t elem_type = mode_get(prs->sx, prs->ansttype + 1);

			scanner(prs);

			if (prs->anst == IDENT) // a[i]
			{
				const size_t size = vector_size(&TREE);
				vector_set(&TREE, size - 2, TSliceident);
				vector_set(&TREE, size - 1, prs->anstdispl);
			}
			else // a[i][j]
			{
				totree(prs, TSlice);
			}

			totree(prs, elem_type);
			parse_condition(prs);
			if (prs->was_error == 4)
			{
				return; // 1
			}
			index_check(prs); // проверка, что индекс int или char
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}

			must_be(prs, RIGHTSQBR, no_rightsqbr_in_slice);

			prs->stackoperands[prs->sopnd] = prs->ansttype = (int)elem_type;
			prs->anst = ADDR;
		}

		while (prs->next_token == ARROW)
		{
			// это выборка поля из указателя на структуру, если после
			// -> больше одной точки подряд, схлопываем в 1 select
			// перед выборкой мог быть вызов функции или вырезка элемента массива

			if (!mode_is_pointer(prs->sx, prs->ansttype) ||
				!mode_is_struct(prs->sx, (int)mode_get(prs->sx, prs->ansttype + 1)))
			{
				parser_error(prs, get_field_not_from_struct_pointer);
				prs->was_error = 4;
				return; // 1
			}

			if (prs->anst == IDENT)
			{
				vector_set(&TREE, vector_size(&TREE) - 2, TIdenttoval);
			}
			prs->anst = ADDR;
			// pointer  мог быть значением функции (VAL) или, может быть,
			totree(prs, TSelect); // context->anst уже был ADDR, т.е. адрес
									  // теперь уже всегда на верхушке стека

			prs->ansttype = (int)mode_get(prs->sx, prs->ansttype + 1);
			prs->anstdispl = find_field(prs, prs->ansttype);
			if (prs->was_error == 6)
			{
				prs->was_error = 4;
				return; // 1
			}
			selectend(prs);
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
		}
		if (prs->next_token == DOT)

		{
			if (!mode_is_struct(prs->sx, prs->ansttype))
			{
				parser_error(prs, select_not_from_struct);
				prs->was_error = 4;
				return; // 1
			}
			if (prs->anst == VAL) // структура - значение функции
			{
				int len1 = (int)size_of(prs->sx, prs->ansttype);
				prs->anstdispl = 0;
				while (prs->next_token == DOT)
				{
					prs->anstdispl += find_field(prs, prs->ansttype);
					if (prs->was_error == 6)
					{
						prs->was_error = 4;
						return; // 1
					}
				}
				totree(prs, COPYST);
				totree(prs, prs->anstdispl);
				totree(prs, (item_t)size_of(prs->sx, prs->ansttype));
				totree(prs, len1);
			}
			else if (prs->anst == IDENT)
			{
				int globid = prs->anstdispl < 0 ? -1 : 1;
				while (prs->next_token == DOT)
				{
					prs->anstdispl += globid * find_field(prs, prs->ansttype);
					if (prs->was_error == 6)
					{
						prs->was_error = 4;
						return; // 1
					}
				}
				vector_set(&TREE, vector_size(&TREE) - 1, prs->anstdispl);
			}
			else // ADDR
			{
				totree(prs, TSelect);
				prs->anstdispl = 0;
				selectend(prs);
				if (prs->was_error == 5)
				{
					prs->was_error = 4;
					return; // 1
				}
			}
		}
	}
	if (prs->next_token == INC || prs->next_token == DEC) // a++, a--
	{
		int op;

		if (!mode_is_int(prs->ansttype) && !mode_is_float(prs->ansttype))
		{
			parser_error(prs, wrong_operand);
			prs->was_error = 4;
			return; // 1
		}

		if (prs->anst != IDENT && prs->anst != ADDR)
		{
			parser_error(prs, unassignable_inc);
			prs->was_error = 4;
			return; // 1
		}
		op = (prs->next_token == INC) ? POSTINC : POSTDEC;
		if (prs->anst == ADDR)
		{
			op += 4;
		}
		scanner(prs);
		totree_float_operation(prs, op);
		if (prs->anst == IDENT)
		{
			totree(prs, ident_get_displ(prs->sx, lid));
		}
		prs->anst = VAL;
	}
}

void unarexpr(parser *const prs)
{
	int op = prs->curr_token;
	if (prs->curr_token == LNOT || prs->curr_token == LOGNOT || prs->curr_token == LPLUS || prs->curr_token == LMINUS ||
		prs->curr_token == LAND || prs->curr_token == LMULT || prs->curr_token == INC || prs->curr_token == DEC)
	{
		if (prs->curr_token == INC || prs->curr_token == DEC)
		{
			scanner(prs);
			unarexpr(prs);
			if (prs->was_error == 7)
			{
				return; // 1
			}
			if (prs->anst != IDENT && prs->anst != ADDR)
			{
				parser_error(prs, unassignable_inc);
				prs->was_error = 7;
				return; // 1
			}
			if (prs->anst == ADDR)
			{
				op += 4;
			}
			totree_float_operation(prs, op);
			if (prs->anst == IDENT)
			{
				totree(prs, ident_get_displ(prs->sx, prs->lastid));
			}
			prs->anst = VAL;
		}
		else
		{
			scanner(prs);
			unarexpr(prs);
			if (prs->was_error == 7)
			{
				return; // 1
			}

			if (op == LAND)
			{
				if (prs->anst == VAL)
				{
					parser_error(prs, wrong_addr);
					prs->was_error = 7;
					return; // 1
				}

				if (prs->anst == IDENT)
				{
					vector_set(&TREE, vector_size(&TREE) - 2, TIdenttoaddr); // &a
				}

				prs->stackoperands[prs->sopnd] = prs->ansttype =
				(int)to_modetab(prs, mode_pointer, prs->ansttype);
				prs->anst = VAL;
			}
			else if (op == LMULT)
			{
				if (!mode_is_pointer(prs->sx, prs->ansttype))
				{
					parser_error(prs, aster_not_for_pointer);
					prs->was_error = 7;
					return; // 1
				}

				if (prs->anst == IDENT)
				{
					vector_set(&TREE, vector_size(&TREE) - 2, TIdenttoval); // *p
				}

				prs->stackoperands[prs->sopnd] = prs->ansttype = (int)mode_get(prs->sx, prs->ansttype + 1);
				prs->anst = ADDR;
			}
			else
			{
				toval(prs);
				if ((op == LNOT || op == LOGNOT) && prs->ansttype == LFLOAT)
				{
					parser_error(prs, int_op_for_float);
					prs->was_error = 7;
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
						totree_float_operation(prs, UNMINUS);
					}
				}
				else if (op == LPLUS)
				{
					;
				}
				else
				{
					totree(prs, op);
				}
				prs->anst = VAL;
			}
		}
	}
	else
	{
		primaryexpr(prs);
		if (prs->was_error == 4)
		{
			prs->was_error = 7;
			return; // 1
		}
	}

	postexpr(prs); // 0
	prs->stackoperands[prs->sopnd] = prs->ansttype;
	if (prs->was_error == 4)
	{
		prs->was_error = 7;
		return; // 1
	}
}

int operator_precedence(const token_t operator)
{
	switch (operator)
	{
		case pipepipe:        // '||'
			return 1;

		case ampamp:          // '&&'
			return 2;

		case pipe:            // '|'
			return 3;

		case caret:           // '^'
			return 4;

		case amp:             // '&'
			return 5;

		case equalequal:      // '=='
		case exclaimequal:    // '!='
			return 6;

		case less:            // '<'
		case greater:         // '<'
		case lessequal:       // '<='
		case greaterequal:    // '>='
			return 7;

		case lessless:        // '<<'
		case greatergreater:  // '>>'
			return 8;

		case plus:            // '+'
		case minus:           // '-'
			return 9;

		case star:            // '*'
		case slash:           // '/'
		case percent:         // '%'
			return 10;

		default:
			return 0;
	}
}

void subexpr(parser *const prs)
{
	int oldsp = prs->sp;
	int wasop = 0;

	int p = operator_precedence(prs->next_token);
	while (p)
	{
		wasop = 1;
		toval(prs);
		while (prs->sp > oldsp && prs->stack[prs->sp - 1] >= p)
		{
			binop(prs, --prs->sp);
			if (prs->was_error == 5)
			{
				return;
			}
		}

		size_t ad = 0;
		if (p <= 2)
		{
			totree(prs, p == 1 ? ADLOGOR : ADLOGAND);
			ad = vector_size(&TREE);
			vector_increase(&TREE, 1);
		}

		prs->stack[prs->sp] = p;
		prs->stacklog[prs->sp] = (int)ad;
		prs->stackop[prs->sp++] = prs->next_token;
		scanner(prs);
		scanner(prs);
		unarexpr(prs);
		if (prs->was_error == 7)
		{
			prs->was_error = 5;
			return; // 1
		}
		p = operator_precedence(prs->next_token);
	}
	if (wasop)
	{
		toval(prs);
	}
	while (prs->sp > oldsp)
	{
		binop(prs, --prs->sp);
		if (prs->was_error == 5)
		{
			return;
		}
	}
}

int intopassn(int next)
{
	return next == REMASS || next == SHLASS || next == SHRASS || next == ANDASS || next == EXORASS || next == ORASS;
}

int opassn(parser *const prs)
{
	return (prs->next_token == ASS || prs->next_token == MULTASS || prs->next_token == DIVASS || prs->next_token == PLUSASS ||
			prs->next_token == MINUSASS || intopassn(prs->next_token))
	? prs->op = prs->next_token
	: 0;
}

void condexpr(parser *const prs)
{
	int globtype = 0;
	size_t adif = 0;

	subexpr(prs); // logORexpr();
	if (prs->was_error == 5)
	{
		prs->was_error = 4;
		return; // 1
	}
	if (prs->next_token == QUEST)
	{
		while (prs->next_token == QUEST)
		{
			toval(prs);
			if (!mode_is_int(prs->ansttype))
			{
				parser_error(prs, float_in_condition);
				prs->was_error = 4;
				return; // 1
			}
			totree(prs, TCondexpr);
			scanner(prs);
			parse_condition(prs); // then
			if (prs->was_error == 4)
			{
				return; // 1
			}
			if (!globtype)
			{
				globtype = prs->ansttype;
			}
			prs->sopnd--;
			if (mode_is_float(prs->ansttype))
			{
				globtype = LFLOAT;
			}
			else
			{
				vector_add(&TREE, (item_t)adif);
				adif = vector_size(&TREE) - 1;
			}
			must_be(prs, COLON, no_colon_in_cond_expr);
			scanner(prs);
			unarexpr(prs);
			if (prs->was_error == 7)
			{
				prs->was_error = 4;
				return; // 1
			}
			subexpr(prs); // logORexpr();	else or elif
			if (prs->was_error == 5)
			{
				prs->was_error = 4;
				return; // 1
			}
		}
		toval(prs);
		totree(prs, TExprend);
		if (mode_is_float(prs->ansttype))
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
			item_t r = vector_get(&TREE, adif);
			vector_set(&TREE, adif, TExprend);
			vector_set(&TREE, adif - 1, mode_is_float(globtype) ? WIDEN : NOP);
			adif = (size_t)r;
		}

		prs->stackoperands[prs->sopnd] = prs->ansttype = globtype;
	}
	else
	{
		prs->stackoperands[prs->sopnd] = prs->ansttype;
	}
}

void exprassnvoid(parser *const prs)
{
	const size_t size = vector_size(&TREE);
	size_t t = vector_get(&TREE, size - 2) < 9000 ? size - 3 : size - 2;
	item_t tt = vector_get(&TREE, t);
	if ((tt >= ASS && tt <= DIVASSAT) || (tt >= POSTINC && tt <= DECAT) || (tt >= ASSR && tt <= DIVASSATR) ||
		(tt >= POSTINCR && tt <= DECATR))
	{
		vector_set(&TREE, t, vector_get(&TREE, t) + 200);
	}
	--prs->sopnd;
}

void exprassn(parser *const prs, int level)
{
	int leftanst;
	int leftanstdispl;
	int ltype;
	int rtype;
	int lnext;

	if (prs->curr_token == BEGIN)
	{
		const int type = prs->leftansttype;
		if (mode_is_struct(prs->sx, type) || mode_is_array(prs->sx, type))
		{
			parse_initializer(prs, type);
			prs->leftansttype = type;
		}
		else
		{
			parser_error(prs, init_not_struct);
			prs->was_error = 6;
			return; // 1
		}
		prs->stackoperands[++prs->sopnd] = prs->ansttype = type;
		prs->anst = VAL;
	}
	else
	{
		unarexpr(prs);
	}
	if (prs->was_error == 7)
	{
		prs->was_error = 6;
		return; // 1
	}

	leftanst = prs->anst;
	leftanstdispl = prs->anstdispl;
	prs->leftansttype = prs->ansttype;
	if (opassn(prs))
	{
		int opp = prs->op;
		lnext = prs->next_token;
		prs->flag_in_assignment = 1;
		scanner(prs);
		scanner(prs);
		exprassn(prs, level + 1);
		if (prs->was_error == 6)
		{
			return; // 1
		}
		prs->flag_in_assignment = 0;

		if (leftanst == VAL)
		{
			parser_error(prs, unassignable);
			prs->was_error = 6;
			return; // 1
		}
		rtype = prs->stackoperands[prs->sopnd--]; // снимаем типы
														  // операндов со стека
		ltype = prs->stackoperands[prs->sopnd];

		if (intopassn(lnext) && (mode_is_float(ltype) || mode_is_float(rtype)))
		{
			parser_error(prs, int_op_for_float);
			prs->was_error = 6;
			return; // 1
		}

		if (mode_is_array(prs->sx, ltype)) // присваивать массив в массив в си нельзя
		{
			parser_error(prs, array_assigment);
			prs->was_error = 6;
			return; // 1
		}

		if (mode_is_struct(prs->sx, ltype)) // присваивание в структуру
		{
			if (ltype != rtype) // типы должны быть равны
			{
				parser_error(prs, type_missmatch);
				prs->was_error = 6;
				return; // 1
			}
			if (opp != ASS) // в структуру можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
				prs->was_error = 6;
				return; // 1
			}

			if (prs->anst == VAL)
			{
				opp = leftanst == IDENT ? COPY0STASS : COPY1STASS;
			}
			else
			{
				opp = leftanst == IDENT ? prs->anst == IDENT ? COPY00 : COPY01
				: prs->anst == IDENT ? COPY10 : COPY11;
			}
			totree(prs, opp);
			if (leftanst == IDENT)
			{
				totree(prs, leftanstdispl); // displleft
			}
			if (prs->anst == IDENT)
			{
				totree(prs, prs->anstdispl); // displright
			}
			totree(prs, mode_get(prs->sx, ltype + 1)); // длина
			prs->anst = leftanst;
			prs->anstdispl = leftanstdispl;
		}
		else // оба операнда базового типа или указатели
		{
			if (mode_is_pointer(prs->sx, ltype) && opp != ASS) // в указатель можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
				prs->was_error = 6;
				return; // 1
			}

			if (mode_is_int(ltype) && mode_is_float(rtype))
			{
				parser_error(prs, assmnt_float_to_int);
				prs->was_error = 6;
				return; // 1
			}

			toval(prs);
			if (mode_is_int(rtype) && mode_is_float(ltype))
			{
				totree(prs, WIDEN);
				prs->ansttype = LFLOAT;
			}
			if (mode_is_pointer(prs->sx, ltype) && mode_is_pointer(prs->sx, rtype) && ltype != rtype)
			{
				// проверка нужна только для указателей
				parser_error(prs, type_missmatch);
				prs->was_error = 6;
				return; // 1
			}

			if (leftanst == ADDR)
			{
				opp += 11;
			}
			totree_float_operation(prs, opp);
			if (leftanst == IDENT)
			{
				prs->anstdispl = leftanstdispl;
				totree(prs, leftanstdispl);
			}
			prs->anst = VAL;
		}
		prs->ansttype = ltype;
		prs->stackoperands[prs->sopnd] = ltype; // тип результата - на стек
	}
	else
	{
		condexpr(prs); // condexpr учитывает тот факт, что начало выражения
		if (prs->was_error == 4)
		{
			prs->was_error = 6;
			return; // 1
		}
	}
	// в виде unarexpr уже выкушано
}

void expr(parser *const prs, int level)
{
	exprassn(prs, level);
	if (prs->was_error == 6)
	{
		prs->was_error = 5;
		return; // 1
	}
	while (prs->next_token == COMMA)
	{
		exprassnvoid(prs);
		prs->sopnd--;
		scanner(prs);
		scanner(prs);
		exprassn(prs, level);
		if (prs->was_error == 6)
		{
			prs->was_error = 5;
			return; // 1
		}
	}
	if (level == 0)
	{
		totree(prs, TExprend);
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


item_t parse_expression(parser *const prs)
{
	expr(prs, 0);
	exprassnvoid(prs);
	return (item_t)prs->ansttype;
}

item_t parse_assignment_expression(parser *const prs)
{
	exprassn(prs, 1);
	toval(prs);
	totree(prs, TExprend);
	prs->sopnd--;
	return (item_t)prs->ansttype;
}

item_t parse_parenthesized_expression(parser *const prs)
{
	must_be(prs, LEFTBR, cond_must_be_in_brkts);
	parse_condition(prs);
	must_be(prs, RIGHTBR, cond_must_be_in_brkts);
	return (item_t)prs->ansttype;
}

item_t parse_constant_expression(parser *const prs)
{
	scanner(prs);
	unarexpr(prs);
	condexpr(prs);
	toval(prs);
	totree(prs, TExprend);
	prs->sopnd--;
	return (item_t)prs->ansttype;
}

item_t parse_condition(parser *const prs)
{
	scanner(prs);
	expr(prs, 1);
	toval(prs);
	totree(prs, TExprend);
	prs->sopnd--;
	return (item_t)prs->ansttype;
}

void parse_string_literal(parser *const prs)
{
	totree(prs, TString);
	totree(prs, prs->lxr->num);

	for (int i = 0; i < prs->lxr->num; i++)
	{
		totree(prs, prs->lxr->lexstr[i]);
	}

	prs->ansttype = (int)to_modetab(prs, mode_array, LCHAR);
	prs->stackoperands[++prs->sopnd] = prs->ansttype;
	prs->anst = VAL;
}

void parse_insert_widen(parser *const parser)
{
	vector_remove(&parser->sx->tree);
	totree(parser, WIDEN);
	totree(parser, TExprend);
}
