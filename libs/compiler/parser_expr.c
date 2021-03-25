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


static void scanner(parser *const prs)
{
	prs->token = lex(prs->lxr);
}

static void must_be(parser *const prs, const token_t what, const error_t num)
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

void float_operation_to_tree(parser *const prs, const item_t operation)
{
	if (prs->ansttype == mode_float
		&& ((operation >= ASS && operation <= DIVASS)
			|| (operation >= ASSAT && operation <= DIVASSAT)
			|| (operation >= EQEQ && operation <= UNMINUS)))
	{
		totree(prs, operation + 50);
	}
	else
	{
		totree(prs, operation);
	}
}

void double_to_tree(node *const nd, const size_t start, const double num)
{
	int64_t num64;
	memcpy(&num64, &num, sizeof(int64_t));

	int32_t fst = num64 & 0x00000000ffffffff;
	int32_t snd = (num64 & 0xffffffff00000000) >> 32;

	if (node_set_arg(nd, start, fst) == -1)
	{
		node_add_arg(nd, fst);
	}
	if (node_set_arg(nd, start + 1, snd) == -1)
	{
		node_add_arg(nd, snd);
	}
}

double double_from_tree(node *const nd)
{
	int64_t fst = (int64_t)node_get_arg(nd, 0) & 0x00000000ffffffff;
	int64_t snd = (int64_t)node_get_arg(nd, 1) & 0x00000000ffffffff;
	int64_t num64 = (snd << 32) | fst;

	double num;
	memcpy(&num, &num64, sizeof(double));
	return num;
}


typedef enum ANST_VAL
{
	variable = IDENT,
	value = VAL,
	number = NUMBER,
	address = ADDR,
} anst_val;

item_t anst_push(parser *const prs, const anst_val type, const item_t mode)
{
	prs->stackoperands[++prs->sopnd] = prs->ansttype = (int)mode;
	prs->anst = (int)type;
	return mode;
}

item_t anst_pop(parser *const prs)
{
	--prs->sopnd;
	return (item_t)prs->ansttype;
}

anst_val anst_peek(parser *const prs)
{
	return prs->anst;
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
			totree(prs, WIDEN1);
		}
		else if (mode_is_int(right))
		{
			totree(prs, WIDEN);
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
		float_operation_to_tree(prs, op);
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
	if (anst_peek(prs) == value || anst_peek(prs) == number)
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

void parse_braced_init_list(parser *const prs, const item_t type)
{
	token_consume(prs);
	totree(prs, type == mode_float ? TStringd : TString);
	node nd_init_list = prs->nd;
	node_add_arg(&nd_init_list, 0);

	size_t length = 0;
	do
	{
		int sign = 1;
		if (token_try_consume(prs, minus))
		{
			sign = -1;
		}
		if (prs->token == int_constant || prs->token == char_constant)
		{
			if (type == mode_float)
			{
				double_to_tree(&nd_init_list, nd_init_list.argc, sign * (double)prs->lxr->num);
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
				double_to_tree(&nd_init_list, nd_init_list.argc, sign * prs->lxr->num_double);
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
	prs->anst = VAL;
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
		parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, REPRTAB_POS));
	}

	totree(prs, TIdent);
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
			totree(prs, TConst);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_character;
			break;

		case int_constant:
			totree(prs, TConst);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_integer;
			break;

		case float_constant:
			totree(prs, TConstd);
			double_to_tree(&prs->nd, 0, prs->lxr->num_double);
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
			if (prs->token == LVOID)
			{
				// TODO: мб перенести в unary или в отдельный cast expression?
				scanner(prs);
				must_be(prs, LMULT, no_mult_in_cast);
				parse_unary_expression(prs);
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
				expr(prs);
				must_be(prs, RIGHTBR, wait_rightbr_in_primary);
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
		parser_error(prs, no_field, repr_get_name(prs->sx, REPRTAB_POS));
		prs->was_error = 5;
		return 0; // 1
	}
	return select_displ;
}

void selectend(parser *const prs)
{
	while (prs->token == DOT)
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

	totree(prs, TCall1);
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
				totree(prs, displ < 0 ? TIdenttoval : TConst);
				node_add_arg(&prs->nd, llabs(displ));
				totree(prs, TExprend);
			}
			else if (mode_is_array(prs->sx, expected_arg_mode) && prs->token == l_brace)
			{
				parse_braced_init_list(prs, mode_get(prs->sx, (size_t)expected_arg_mode + 1));
				totree(prs, TExprend);
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

	while (prs->token == LEFTSQBR || prs->token == ARROW || prs->token == DOT)
	{
		while (prs->token == LEFTSQBR) // вырезка из массива (возможно, многомерного)
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
			expr(prs);
			toval(prs);
			prs->sopnd--;
			totree(prs, TExprend);
			if (prs->was_error == 4)
			{
				return; // 1
			}
			if (!mode_is_int(prs->ansttype))
			{
				parser_error(prs, index_must_be_int);
				prs->was_error = 4;
				return; // 1
			}

			must_be(prs, RIGHTSQBR, no_rightsqbr_in_slice);

			prs->stackoperands[prs->sopnd] = prs->ansttype = (int)elem_type;
			prs->anst = ADDR;
		}

		while (prs->token == ARROW)
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
		if (prs->token == DOT)

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
				while (prs->token == DOT)
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
				while (prs->token == DOT)
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

		float_operation_to_tree(prs, operator);

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

			float_operation_to_tree(prs, operator);

			if (anst_peek(prs) == variable)
			{
				totree(prs, ident_get_displ(prs->sx, prs->lastid));
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
					if (!mode_is_pointer(prs->sx, prs->ansttype))
					{
						parser_error(prs, aster_not_for_pointer);
					}

					if (prs->anst == IDENT)
					{
						node_set_type(&prs->nd, TIdenttoval);
					}

					anst_push(prs, address, mode_get(prs->sx, (size_t)anst_pop(prs) + 1));
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
							double_to_tree(&prs->nd, 0, -double_from_tree(&prs->nd));
						}
						else
						{
							float_operation_to_tree(prs, UNMINUS);
						}
					}
					else
					{
						if ((operator == tilde || operator == exclaim) && mode_is_float(prs->ansttype))
						{
							parser_error(prs, int_op_for_float);
						}

						if (operator != plus)
						{
							totree(prs, operator);
						}

						anst_push(prs, value, anst_pop(prs));
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

void subexpr(parser *const prs)
{
	int oldsp = prs->sp;
	int wasop = 0;

	int p = operator_precedence(prs->token);
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
		prs->stackop[prs->sp++] = prs->token;
		scanner(prs);
		parse_unary_expression(prs);
		if (prs->was_error == 7)
		{
			prs->was_error = 5;
			return; // 1
		}
		p = operator_precedence(prs->token);
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
	return (prs->token == ASS || prs->token == MULTASS || prs->token == DIVASS || prs->token == PLUSASS ||
			prs->token == MINUSASS || intopassn(prs->token))
	? prs->op = prs->token
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
	if (prs->token == QUEST)
	{
		while (prs->token == QUEST)
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
			expr(prs); // then
			toval(prs);
			prs->sopnd--;
			totree(prs, TExprend);
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
			parse_unary_expression(prs);
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
}

void exprassn(parser *const prs)
{
	int leftanst;
	int leftanstdispl;
	int ltype;
	int rtype;
	int lnext;

	if (prs->token == BEGIN)
	{
		const int type = prs->leftansttype;
		if (mode_is_struct(prs->sx, type) || mode_is_array(prs->sx, type))
		{
			parse_initializer(prs, &prs->nd, type);
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
		parse_unary_expression(prs);
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
		lnext = prs->token;
		prs->flag_in_assignment = 1;
		scanner(prs);
		exprassn(prs);
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
			float_operation_to_tree(prs, opp);
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

void expr(parser *const prs)
{
	exprassn(prs);
	while (token_try_consume(prs, comma))
	{
		exprassnvoid(prs);
		anst_pop(prs);
		exprassn(prs);
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
	expr(prs);
	totree(prs, TExprend);
	exprassnvoid(prs);
	return anst_pop(prs);
}

item_t parse_assignment_expression(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	exprassn(prs);
	toval(prs);
	totree(prs, TExprend);
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
	condexpr(prs);
	toval(prs);
	totree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_condition(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	expr(prs);
	toval(prs);
	totree(prs, TExprend);
	return anst_pop(prs);
}

item_t parse_string_literal(parser *const prs, node *const parent)
{
	prs->nd = *parent;
	totree(prs, TString);
	node_add_arg(&prs->nd, prs->lxr->num);

	for (size_t i = 0; i < (size_t)prs->lxr->num; i++)
	{
		node_add_arg(&prs->nd, prs->lxr->lexstr[i]);
	}

    token_consume(prs);
	return anst_push(prs, value, to_modetab(prs, mode_array, mode_character));
}

void parse_insert_widen(parser *const parser)
{
	vector_remove(&parser->sx->tree);
	totree(parser, WIDEN);
	totree(parser, TExprend);
}
