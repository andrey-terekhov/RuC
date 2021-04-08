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


void totree_float_operation(parser *const prs, const item_t type, const item_t op)
{
	if ((op >= ASS && op <= DIVASS)
		|| (op >= ASSAT && op <= DIVASSAT)
		|| (op >= EQEQ && op <= UNMINUS))
	{
		to_tree(prs, type == mode_float ? op + 50 : op);
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

	node_set_arg(nd, index, fst);
	node_set_arg(nd, index + 1, snd);
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


item_t anst_push(parser *const prs, const operand_t type, const item_t mode)
{
	prs->operands[++prs->operands_size] = mode;
	prs->ansttype = mode;
	prs->anst = type;
	return mode;
}

item_t anst_pop(parser *const prs)
{
	return prs->operands[prs->operands_size--];
}

operand_t anst_peek(parser *const prs)
{
	return prs->anst;
}


void binary_operation(parser *const prs, const operator_t operator)
{
	const token_t token = operator.token;
	const item_t right_mode = anst_pop(prs);
	const item_t left_mode = anst_pop(prs);
	item_t result_mode = right_mode;

	if (mode_is_pointer(prs->sx, left_mode) || mode_is_pointer(prs->sx, right_mode))
	{
		parser_error(prs, operand_is_pointer);
	}

	if (mode_is_float(left_mode) || mode_is_float(right_mode))
	{
		if (token == LOGOR || token == LOGAND || token == LOR || token == LEXOR || token == LAND
			|| token == LSHR || token == LSHL || token == LREM)
		{
			parser_error(prs, int_op_for_float);
		}

		if (mode_is_int(left_mode))
		{
			to_tree(prs, WIDEN1);
		}
		else if (mode_is_int(right_mode))
		{
			to_tree(prs, WIDEN);
		}

		result_mode = mode_float;
	}

	if (token == LOGOR || token == LOGAND)
	{
		to_tree(prs, token);
		vector_set(&TREE, operator.addr, tree_reference(prs));
		node_add_arg(&prs->nd, 0);
	}
	else
	{
		totree_float_operation(prs, result_mode, token);
	}
	if (token >= EQEQ && token <= LGE)
	{
		result_mode = mode_integer;
	}

	anst_push(prs, value, result_mode);
}

void to_value(parser *const prs)
{
	switch (anst_peek(prs))
	{
		case variable:
		{
			const item_t type = anst_pop(prs);
			if (mode_is_struct(prs->sx, type) && !prs->flag_in_assignment)
			{
				node_set_type(&prs->nd, COPY0ST);
				node_set_arg(&prs->nd, 0, prs->anstdispl);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else
			{
				node_set_type(&prs->nd, mode_is_float(type) ? TIdenttovald : TIdenttoval);
			}

			anst_push(prs, value, type);
		}
		break;

		case address:
		{
			const item_t type = anst_pop(prs);
			if (mode_is_struct(prs->sx, type) && !prs->flag_in_assignment)
			{
				to_tree(prs, COPY1ST);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else if (!mode_is_array(prs->sx, type) && !mode_is_pointer(prs->sx, type))
			{
				to_tree(prs, mode_is_float(type) ? TAddrtovald : TAddrtoval);
			}

			anst_push(prs, value, type);
		}
		break;

		case value:
		case number:
			break;
	}
}

item_t parse_braced_init_list(parser *const prs, const item_t type)
{
	token_consume(prs);
	to_tree(prs, type == mode_float ? TStringd : TString);
	node nd_init_list;
	node_copy(&nd_init_list, &prs->nd);
	node_add_arg(&nd_init_list, 0);

	size_t length = 0;
	do
	{
		const int sign = token_try_consume(prs, minus) ? -1 : 1;

		if (prs->token == int_constant || prs->token == char_constant)
		{
			if (type == mode_float)
			{
				node_add_double(&nd_init_list, sign * prs->lxr->num);
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
	to_tree(prs, TExprend);
	if (!token_try_consume(prs, r_brace))
	{
		parser_error(prs, no_comma_or_end);
		token_skip_until(prs, r_brace | semicolon);
	}

	return to_modetab(prs, mode_array, type);
}

void must_be_string(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);
	
	if (!mode_is_string(prs->sx, anst_pop(prs)))
	{
		parser_error(prs, not_string_in_stanfunc);
	}
}

void must_be_point_string(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	const item_t type = anst_pop(prs);
	if (!(mode_is_pointer(prs->sx, type) && mode_is_string(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
	{
		parser_error(prs, not_point_string_in_stanfunc);
	}
}

void must_be_row(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_array(prs->sx, anst_pop(prs)))
	{
		parser_error(prs, not_array_in_stanfunc);
	}
}

void must_be_int(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_int(anst_pop(prs)))
	{
		parser_error(prs, not_int_in_stanfunc);
	}
}

void must_be_row_of_int(parser *const prs)
{
	item_t type;
	if (prs->token == l_brace)
	{
		type = parse_braced_init_list(prs, mode_integer);
	}
	else
	{
		parse_assignment_expression_internal(prs);
		to_value(prs);

		type = anst_pop(prs);
		if (mode_is_int(type))
		{
			to_tree(prs, ROWING);
			type = to_modetab(prs, mode_array, mode_integer);
		}
	}

	if (!(mode_is_array(prs->sx, type) && mode_is_int(mode_get(prs->sx, (size_t)type + 1))))
	{
		parser_error(prs, not_rowofint_in_stanfunc);
	}
}

void must_be_row_of_float(parser *const prs)
{
	item_t type;
	if (prs->token == l_brace)
	{
		type = parse_braced_init_list(prs, mode_float);
	}
	else
	{
		parse_assignment_expression_internal(prs);
		to_value(prs);

		type = anst_pop(prs);
		if (mode_is_float(type))
		{
			to_tree(prs, ROWINGD);
			type = to_modetab(prs, mode_array, mode_float);
		}
	}

	if (!(mode_is_array(prs->sx, type) && mode_get(prs->sx, (size_t)type + 1) == mode_float))
	{
		parser_error(prs, not_rowoffloat_in_stanfunc);
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
		must_be_int(prs);
		token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
		must_be_string(prs);
	}
	else if (func <= STRCPY && func >= STRLEN) // функции работы со строками
	{
		if (func >= STRNCAT)
		{
			must_be_point_string(prs);
		}
		else
		{
			must_be_string(prs);
		}

		if (func != STRLEN)
		{
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_string(prs);
			if (func == STRNCPY || func == STRNCAT || func == STRNCMP)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
			}
		}

		if (func < STRNCAT)
		{
			prs->operands[++prs->operands_size] = prs->ansttype = LINT;
		}
	}
	else if (func >= RECEIVE_STRING && func <= SEND_INT)
	{
		// новые функции Фадеева
		must_be_int(prs);
		if (func == SEND_INT || func == SEND_STRING)
		{
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
		}
		else if (func == SEND_FLOAT)
		{
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_float(prs);
		}
		else
		{
			prs->operands[++prs->operands_size] = prs->ansttype =
			func == RECEIVE_INT ? LINT : func == RECEIVE_FLOAT ? LFLOAT : (int)to_modetab(prs, mode_array, LCHAR);
		}
	}
	else if (func >= ICON && func <= WIFI_CONNECT) // функции Фадеева
	{
		if (func <= PIXEL && func >= ICON)
		{
			must_be_row_of_int(prs);
			if (func != CLEAR)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			}

			if (func == LINE || func == RECTANGLE || func == ELLIPS)
			{
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				if (func != LINE)
				{
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
				}
			}
			else if (func == ICON || func == PIXEL)
			{
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				if (func == ICON)
				{
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
				}
			}
			else if (func == DRAW_NUMBER || func == DRAW_STRING)
			{
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);

				if (func == DRAW_STRING)
				{
					must_be_string(prs);
				}
				else // DRAW_NUMBER
				{
					parse_assignment_expression_internal(prs);
					to_value(prs);
					prs->operands_size--;
					if (mode_is_int(prs->ansttype))
					{
						to_tree(prs, WIDEN);
					}
					else if (prs->ansttype != LFLOAT)
					{
						parser_error(prs, not_float_in_stanfunc);
					}
				}
			}
		}
		else if (func == SETSIGNAL)
		{
			must_be_int(prs);
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
		}
		else if (func == WIFI_CONNECT || func == BLYNK_AUTORIZATION || func == BLYNK_NOTIFICATION)
		{
			must_be_string(prs);
			if (func == WIFI_CONNECT)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_string(prs);
			}
		}
		else
		{
			must_be_int(prs);
			if (func != BLYNK_RECEIVE)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				if (func == BLYNK_TERMINAL)
				{
					must_be_string(prs);
				}
				else if (func == BLYNK_SEND)
				{
					must_be_int(prs);
				}
				else if (func == BLYNK_PROPERTY)
				{
					must_be_string(prs);
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_string(prs);
				}
				else // BLYNK_LCD
				{
					must_be_int(prs);
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_string(prs);
				}
			}
			else
			{
				prs->operands[++prs->operands_size] = prs->ansttype = LINT;
			}
		}
	}
	else if (func == UPB) // UPB
	{
		must_be_int(prs);
		token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
		must_be_row(prs);
		prs->operands[++prs->operands_size] = prs->ansttype = LINT;
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
			prs->ansttype = prs->operands[++prs->operands_size] =
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
				if (!token_try_consume(prs, identifier))
				{
					parser_error(prs, act_param_not_ident);
				}

				const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
				if (id == ITEM_MAX)
				{
					parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, prs->lxr->repr));
				}

				prs->last_id = (size_t)id;
				if (ident_get_mode(prs->sx, prs->last_id) != 15) // 15 - это аргумент типа void* (void*)
				{
					parser_error(prs, wrong_arg_in_create);
				}

				prs->operands[prs->operands_size] = prs->ansttype = LINT;
				item_t displ = ident_get_displ(prs->sx, prs->last_id);
				if (displ < 0)
				{
					to_tree(prs, TIdenttoval);
					node_add_arg(&prs->nd, -displ);
				}
				else
				{
					to_tree(prs, TConst);
					node_add_arg(&prs->nd, displ);
				}
				prs->anst = value;
			}
			else
			{
				prs->leftansttype = 2;
				parse_assignment_expression_internal(prs);
				to_value(prs);

				if (func == TMSGSEND)
				{
					if (prs->ansttype != 2) // 2 - это аргумент типа msg_info (struct{int numTh; int data;})
					{
						parser_error(prs, wrong_arg_in_send);
					}
					--prs->operands_size;
				}
				else
				{
					if (!mode_is_int(prs->ansttype))
					{
						parser_error(prs, param_threads_not_int);
					}

					if (func == TSEMCREATE)
					{
						prs->anst = value,
						prs->ansttype = prs->operands[prs->operands_size] =
						LINT; // съели 1 параметр, выдали int
					}
					else
					{
						--prs->operands_size; // съели 1 параметр, не выдали
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
		prs->ansttype = prs->operands[++prs->operands_size] = LFLOAT;
	}
	else if (func == ROUND)
	{
		parse_assignment_expression_internal(prs);
		to_value(prs);
		prs->ansttype = prs->operands[prs->operands_size] = LINT;
	}
	else
	{
		parse_assignment_expression_internal(prs);
		to_value(prs);

		// GETDIGSENSOR int(int port, int pins[]),
		// GETANSENSOR int (int port, int pin),
		// SETMOTOR и VOLTAGE void (int port, int volt)
		if (func == GETDIGSENSOR || func == GETANSENSOR || func == SETMOTOR || func == VOLTAGE)
		{
			if (!mode_is_int(prs->ansttype))
			{
				parser_error(prs, param_setmotor_not_int);
			}
			token_expect_and_consume(prs, comma, no_comma_in_setmotor);

			if (func == GETDIGSENSOR)
			{
				must_be_row_of_int(prs);
				prs->ansttype = prs->operands[++prs->operands_size] = LINT;
			}
			else
			{
				parse_assignment_expression_internal(prs);
				to_value(prs);
				if (!mode_is_int(prs->ansttype))
				{
					parser_error(prs, param_setmotor_not_int);
				}

				if (func == SETMOTOR || func == VOLTAGE)
				{
					prs->operands_size -= 2;
				}
				else
				{
					--prs->operands_size, prs->anst = value;
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
				prs->ansttype = prs->operands[prs->operands_size] = LFLOAT;
			}

			if (!mode_is_float(prs->ansttype))
			{
				parser_error(prs, bad_param_in_stand_func);
			}
		}
	}

	to_tree(prs, 9500 - func);
	token_expect_and_consume(prs, RIGHTBR, no_rightbr_in_stand_func);
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
	const size_t id = (size_t)repr_get_reference(prs->sx, prs->lxr->repr);
	if ((item_t)id == ITEM_MAX)
	{
		parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, prs->lxr->repr));
	}

	to_tree(prs, TIdent);
	prs->anstdispl = ident_get_displ(prs->sx, id);
	node_add_arg(&prs->nd, prs->anstdispl);

	const item_t mode = ident_get_mode(prs->sx, id);

	prs->last_id = id;
	token_consume(prs);
	anst_push(prs, variable, mode);
	return id;
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
		{
			to_tree(prs, TConst);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_character;
		}
		break;

		case int_constant:
		{
			to_tree(prs, TConst);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_integer;
		}
		break;

		case float_constant:
		{
			to_tree(prs, TConstd);
			node_add_double(&prs->nd, prs->lxr->num_double);
			mode = mode_float;
		}
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
		{
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
				to_value(prs);
				to_tree(prs, TExprend);
			}
			else
			{
				const size_t old_operators_size = prs->operators_size;
				parse_expression_internal(prs);
				token_expect_and_consume(prs, r_paren, wait_rightbr_in_primary);
				while (prs->operators_size > old_operators_size)
				{
					binary_operation(prs, prs->operators[--prs->operators_size]);
				}
			}
		}
		break;

		default:
			if (prs->token <= STANDARD_FUNC_START)
			{
				parse_standard_function_call(prs);
			}
			else
			{
				parser_error(prs, expected_expression, prs->token);
				anst_push(prs, number, mode_undefined);
				token_consume(prs);
			}
			break;
	}
}

/** Кушает токены, относящиеся к вырезкам, берет тип структуры со стека и кладет тип поля на стек */
item_t find_field(parser *const prs)
{
	token_consume(prs);
	token_expect_and_consume(prs, identifier, after_dot_must_be_ident);

	const operand_t peek = anst_peek(prs);
	const size_t type = (size_t)anst_pop(prs);
	const size_t record_length = (size_t)mode_get(prs->sx, type + 2);
	if ((item_t)record_length == ITEM_MAX)
	{
		return 0;
	}

	item_t select_displ = 0;
	for (size_t i = 0; i < record_length; i += 2)
	{
		const item_t field_type = mode_get(prs->sx, type + 3 + i);

		if ((size_t)mode_get(prs->sx, type + 4 + i) == prs->lxr->repr)
		{
			anst_push(prs, peek, field_type);
			return select_displ;
		}
		else
		{
			// Прибавляем к суммарному смещению длину поля
			select_displ += (item_t)size_of(prs->sx, field_type);
		}
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
	node nd_call;
	node_copy(&nd_call, &prs->nd);
	node_add_arg(&nd_call, expected_args);
	size_t ref_arg_mode = function_mode + 3;

	if (!token_try_consume(prs, r_paren))
	{
		do
		{
			node_copy(&prs->nd, &nd_call);
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
	node_copy(&prs->nd, &nd_call2);
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
	const size_t last_id = prs->last_id;

	if (token_try_consume(prs, l_paren))
	{
		was_func = 1;
		parse_function_call(prs, last_id);
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

			const item_t mode = anst_pop(prs);
			if (!mode_is_array(prs->sx, mode))
			{
				parser_error(prs, slice_not_from_array);
			}

			const item_t elem_type = mode_get(prs->sx, (size_t)mode + 1);
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
			if (anst_peek(prs) == variable)
			{
				node_set_type(&prs->nd, TIdenttoval);
			}

			to_tree(prs, TSelect);

			// Здесь мы ожидаем указатель, снимаем указатель со стека и кладем саму структуру
			const item_t type = anst_pop(prs);
			if (!(mode_is_pointer(prs->sx, type) && mode_is_struct(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
			{
				parser_error(prs, get_field_not_from_struct_pointer);
			}

			anst_push(prs, address, mode_get(prs->sx, (size_t)type + 1));
			prs->anstdispl = find_field(prs);
			while (prs->token == period)
			{
				prs->anstdispl += find_field(prs);
			}

			to_tree(prs, prs->anstdispl);

			// find_field вернула тип результата через стек, проверим его и вернем обратно
			const item_t field_type = anst_pop(prs);
			if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
			{
				to_tree(prs, TAddrtoval);
			}

			anst_push(prs, address, field_type);
		}

		if (prs->token == period)
		{
			const operand_t peek = anst_peek(prs);
			const item_t type = anst_pop(prs);
			if (!mode_is_struct(prs->sx, type))
			{
				parser_error(prs, select_not_from_struct);
			}

			if (peek == value)
			{
				const size_t length = size_of(prs->sx, type);
				anst_push(prs, value, type);
				prs->anstdispl = 0;
				while (prs->token == period)
				{
					prs->anstdispl += find_field(prs);
				}

				const item_t field_type = anst_pop(prs);
				to_tree(prs, COPYST);
				node_add_arg(&prs->nd, prs->anstdispl);
				node_add_arg(&prs->nd, (item_t)size_of(prs->sx, field_type));
				node_add_arg(&prs->nd, (item_t)length);
				anst_push(prs, value, field_type);
			}
			else if (peek == variable)
			{
				int is_global = prs->anstdispl < 0 ? -1 : 1;
				anst_push(prs, variable, type);
				while (prs->token == period)
				{
					prs->anstdispl += is_global * find_field(prs);
				}

				node_set_arg(&prs->nd, 0, prs->anstdispl);
			}
			else //if (peek == address)
			{
				to_tree(prs, TSelect);
				anst_push(prs, variable, type);
				prs->anstdispl = 0;
				while (prs->token == period)
				{
					prs->anstdispl += find_field(prs);
				}

				node_add_arg(&prs->nd, prs->anstdispl);
				// find_field вернула тип результата через стек, проверим его и вернем обратно
				const item_t field_type = anst_pop(prs);
				if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
				{
					to_tree(prs, TAddrtoval);
				}

				anst_push(prs, address, field_type);
			}
		}
	}

	if (prs->token == plusplus || prs->token == minusminus)
	{
		int operator = prs->token == plusplus ? POSTINC : POSTDEC;
		token_consume(prs);

		int is_variable = 0;
		if (anst_peek(prs) == address)
		{
			operator += 4;
		}
		else if (anst_peek(prs) == variable)
		{
			is_variable = 1;
		}
		else
		{
			parser_error(prs, unassignable_inc);
		}

		const item_t type = anst_pop(prs);
		if (!mode_is_int(type) && !mode_is_float(type))
		{
			parser_error(prs, wrong_operand);
		}

		anst_push(prs, value, type);
		totree_float_operation(prs, type, operator);

		if (is_variable)
		{
			node_add_arg(&prs->nd, ident_get_displ(prs->sx, last_id));
		}
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

			int is_variable = 0;
			if (anst_peek(prs) == address)
			{
				operator += 4;
			}
			else if (anst_peek(prs) == variable)
			{
				is_variable = 1;
			}
			else
			{
				parser_error(prs, unassignable_inc);
			}

			const item_t type = anst_pop(prs);
			if (!mode_is_int(type) && !mode_is_float(type))
			{
				parser_error(prs, wrong_operand);
			}

			anst_push(prs, value, type);
			totree_float_operation(prs, type, operator);

			if (is_variable)
			{
				node_add_arg(&prs->nd, ident_get_displ(prs->sx, prs->last_id));
			}
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
					to_value(prs);
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
							const item_t type = anst_pop(prs);
							totree_float_operation(prs, type, UNMINUS);
							anst_push(prs, value, type);
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

uint8_t operator_precedence(const token_t operator)
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

int is_int_assignment_operator(const token_t operator)
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
	switch (operator)
	{
		case equal:					// '='
		case starequal:				// '*='
		case slashequal:			// '/='
		case plusequal:				// '+='
		case minusequal:			// '-='
			return 1;

		default:
			return is_int_assignment_operator(operator);
	}
}

void parse_subexpression(parser *const prs)
{
	size_t old_operators_size = prs->operators_size;
	int was_operator = 0;

	uint8_t precedence = operator_precedence(prs->token);
	while (precedence)
	{
		was_operator = 1;
		to_value(prs);
		while (prs->operators_size > old_operators_size
			   && prs->operators[prs->operators_size - 1].precedence >= precedence)
		{
			binary_operation(prs, prs->operators[--prs->operators_size]);
		}

		size_t addr = 0;
		if (precedence <= 2)
		{
			to_tree(prs, precedence == 1 ? ADLOGOR : ADLOGAND);
			addr = (size_t)tree_reference(prs);
			node_add_arg(&prs->nd, 0);
		}

		operator_t operator;
		operator.precedence = precedence;
		operator.token = prs->token;
		operator.addr = addr;
		prs->operators[prs->operators_size++] = operator;

		token_consume(prs);
		parse_unary_expression(prs);
		precedence = operator_precedence(prs->token);
	}

	if (was_operator)
	{
		to_value(prs);
	}

	while (prs->operators_size > old_operators_size)
	{
		binary_operation(prs, prs->operators[--prs->operators_size]);
	}
}

void parse_conditional_expression(parser *const prs)
{
	parse_subexpression(prs); // logORexpr();

	if (prs->token == question)
	{
		item_t global_type = 0;
		size_t addr_if = 0;

		while (token_try_consume(prs, question))
		{
			to_value(prs);
			if (!mode_is_int(anst_pop(prs)))
			{
				parser_error(prs, float_in_condition);
			}

			to_tree(prs, TCondexpr);
			node nd_condexpr;
			node_copy(&nd_condexpr, &prs->nd);
			const item_t expr_type = parse_condition(prs, &nd_condexpr); // then

			if (!global_type)
			{
				global_type = expr_type;
			}

			if (mode_is_float(expr_type))
			{
				global_type = mode_float;
			}
			else
			{
				const size_t ref = (size_t)tree_reference(prs);	// эта строка потом уйдет
				node_add_arg(&prs->nd, (item_t)addr_if);
				addr_if = ref;
			}

			token_expect_and_consume(prs, colon, no_colon_in_cond_expr);
			node_copy(&prs->nd, &nd_condexpr);
			parse_unary_expression(prs);
			parse_subexpression(prs); // logORexpr();	else or elif
		}

		to_value(prs);
		to_tree(prs, TExprend);

		if (mode_is_float(anst_pop(prs)))
		{
			global_type = mode_float;
		}
		else
		{
			const size_t ref = (size_t)tree_reference(prs);
			node_add_arg(&prs->nd, (item_t)addr_if);
			addr_if = ref;
		}

		while (addr_if != 0 && addr_if <= vector_size(&TREE))
		{
			const size_t ref = (size_t)vector_get(&TREE, addr_if);
			vector_set(&TREE, addr_if - 1, mode_is_float(global_type) ? WIDEN : NOP);
			vector_set(&TREE, addr_if, TExprend);
			addr_if = ref;
		}

		anst_push(prs, value, global_type);
	}
}

void assignment_to_void(parser *const prs)
{
	const item_t operation = node_get_type(&prs->nd);
	if ((operation >= ASS && operation <= DIVASSAT)
		|| (operation >= POSTINC && operation <= DECAT)
		|| (operation >= ASSR && operation <= DIVASSATR)
		|| (operation >= POSTINCR && operation <= DECATR))
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
		const item_t target_displ = prs->anstdispl;
		const operand_t left_type = anst_peek(prs);
		if (left_type == value)
		{
			parser_error(prs, unassignable);
		}

		item_t operator = prs->token;
		token_consume(prs);

		prs->flag_in_assignment = 1;
		parse_assignment_expression_internal(prs);
		prs->flag_in_assignment = 0;

		// Снимаем типы операндов со стека
		const operand_t right_type = anst_peek(prs);
		const item_t right_mode = anst_pop(prs);
		const item_t left_mode = anst_pop(prs);
		item_t result_mode = right_mode;

		if (is_int_assignment_operator(operator) && (mode_is_float(left_mode) || mode_is_float(right_mode)))
		{
			parser_error(prs, int_op_for_float);
		}
		else if (mode_is_array(prs->sx, left_mode))
		{
			parser_error(prs, array_assigment);
		}
		else if (mode_is_struct(prs->sx, left_mode))
		{
			if (left_mode != right_mode) // типы должны быть равны
			{
				parser_error(prs, type_missmatch);
			}

			if (operator != equal) // в структуру можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
			}

			if (right_type == value)
			{
				operator = left_type == variable ? COPY0STASS : COPY1STASS;
			}
			else
			{
				operator = left_type == variable
					? right_type == variable
						? COPY00 : COPY01
					: right_type == variable
						? COPY10 : COPY11;
			}

			to_tree(prs, operator);
			if (left_type == variable)
			{
				node_add_arg(&prs->nd, target_displ);
			}
			if (right_type == variable)
			{
				node_add_arg(&prs->nd, prs->anstdispl);
			}
			node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)left_mode + 1));
			prs->anstdispl = target_displ;
			anst_push(prs, left_type, left_mode);
		}
		else // оба операнда базового типа или указатели
		{
			// В указатель можно присваивать только с помощью '='
			if (mode_is_pointer(prs->sx, left_mode) && operator != equal)
			{
				parser_error(prs, wrong_struct_ass);
			}

			if (mode_is_int(left_mode) && mode_is_float(right_mode))
			{
				parser_error(prs, assmnt_float_to_int);
			}

			// Здесь мы используем стек, чтобы передать в to_value тип и вид значения
			// Это не очень красивый вариант, но рабочий: мы точно знаем, где эти тип и вид взять
			// TODO: придумать вариант красивее
			anst_push(prs, right_type, right_mode);
			to_value(prs);
			anst_pop(prs);

			if (mode_is_float(left_mode) && mode_is_int(right_mode))
			{
				to_tree(prs, WIDEN);
				result_mode = mode_float;
			}
			if (mode_is_pointer(prs->sx, left_mode) && mode_is_pointer(prs->sx, right_mode) && left_mode != right_mode)
			{
				// проверка нужна только для указателей
				parser_error(prs, type_missmatch);
			}

			if (left_type == address)
			{
				operator += 11;
			}
			totree_float_operation(prs, result_mode, operator);
			if (left_type == variable)
			{
				prs->anstdispl = target_displ;
				node_add_arg(&prs->nd, target_displ);
			}
			anst_push(prs, value, left_mode);
		}
	}
	else
	{
		// Эта функция учитывает, что начало в виде унарного выражения уже выкушано
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
	node_copy(&prs->nd, parent);
	parse_expression_internal(prs);
	assignment_to_void(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_assignment_expression(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_assignment_expression_internal(prs);
	to_value(prs);
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
	node_copy(&prs->nd, parent);
	parse_unary_expression(prs);
	parse_conditional_expression(prs);
	to_value(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_condition(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_expression_internal(prs);
	to_value(prs);
	to_tree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_string_literal(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	to_tree(prs, TString);
	node_add_arg(&prs->nd, prs->lxr->num);

	for (int i = 0; i < prs->lxr->num; i++)
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
