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


/** Operator stack member */
typedef struct operator
{
	uint8_t priority;	/**< Operator priority */
	token_t token;		/**< Operator token */
	node nd;			/**< Operator node in AST */
} operator;


void parse_unary_expression(parser *const prs);
void parse_assignment_expression_internal(parser *const prs);
void parse_expression_internal(parser *const prs);
void parse_constant(parser *const prs);


int operators_push(parser *const prs, const uint8_t priority, const token_t token, const node *const nd)
{
	return stack_push(&prs->stk.priorities, priority)
		|| stack_push(&prs->stk.tokens, token)
		|| stack_push(&prs->stk.nodes, node_save(nd));
}

operator operators_pop(parser *const prs)
{
	operator op;

	op.priority = (uint8_t)stack_pop(&prs->stk.priorities);
	op.token = stack_pop(&prs->stk.tokens);
	op.nd = node_load(&prs->sx->tree, (size_t)stack_pop(&prs->stk.nodes));

	return op;
}

operator operators_peek(parser *const prs)
{
	operator op;

	op.priority = (uint8_t)stack_peek(&prs->stk.priorities);
	op.token = stack_peek(&prs->stk.tokens);
	op.nd = node_load(&prs->sx->tree, (size_t)stack_peek(&prs->stk.nodes));

	return op;
}

size_t operators_size(const parser *const prs)
{
	return stack_size(&prs->stk.tokens);
}

int operands_push(parser *const prs, const operand_t type, const item_t mode)
{
	prs->last_type = type;
	return stack_push(&prs->anonymous, mode);
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

double node_get_double(const node *const nd, const size_t index)
{
	const int64_t fst = (int64_t)node_get_arg(nd, index) & 0x00000000ffffffff;
	const int64_t snd = (int64_t)node_get_arg(nd, index + 1) & 0x00000000ffffffff;
	const int64_t num64 = (snd << 32) | fst;

	double num;
	memcpy(&num, &num64, sizeof(double));
	return num;
}


void float_operation(parser *const prs, const item_t type, const item_t operation)
{
	if ((operation >= ASS && operation <= DIVASS)
		|| (operation >= ASSAT && operation <= DIVASSAT)
		|| (operation >= EQEQ && operation <= UNMINUS))
	{
		to_tree(prs, type == mode_float ? operation + 50 : operation);
	}
	else
	{
		to_tree(prs, operation);
	}
}

void binary_operation(parser *const prs, operator operator)
{
	const token_t token = operator.token;
	const item_t right_mode = stack_pop(&prs->anonymous);
	const item_t left_mode = stack_pop(&prs->anonymous);
	item_t result_mode = right_mode;

	if (mode_is_pointer(prs->sx, left_mode) || mode_is_pointer(prs->sx, right_mode))
	{
		parser_error(prs, operand_is_pointer);
	}

	if (mode_is_float(left_mode) || mode_is_float(right_mode))
	{
		if (token == pipepipe || token == ampamp || token == pipe || token == caret || token == amp
			|| token == greatergreater || token == lessless || token == percent)
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

	if (token == pipepipe || token == ampamp)
	{
		to_tree(prs, token);
		node_add_arg(&prs->nd, 0); // FIXME: useless

		// FIXME: just remove it after MIPS integration
		node_set_arg(&operator.nd, 0, node_save(&prs->nd));
		node_set_arg(&operator.nd, 0, node_get_arg(&operator.nd, 0) + 1);
	}
	else
	{
		float_operation(prs, result_mode, token);
	}

	if (token >= equalequal && token <= greaterequal)
	{
		result_mode = mode_integer;
	}

	operands_push(prs, VALUE, result_mode);
}

void to_value(parser *const prs)
{
	switch (prs->last_type)
	{
		case VARIABLE:
		{
			const item_t type = stack_pop(&prs->anonymous);
			if (mode_is_struct(prs->sx, type) && !prs->flag_in_assignment)
			{
				node_set_type(&prs->nd, COPY0ST);
				node_set_arg(&prs->nd, 0, prs->operand_displ);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else
			{
				node_set_type(&prs->nd, mode_is_float(type) ? TIdenttovald : TIdenttoval);
			}

			operands_push(prs, VALUE, type);
		}
		break;

		case ADDRESS:
		{
			const item_t type = stack_pop(&prs->anonymous);
			if (mode_is_struct(prs->sx, type) && !prs->flag_in_assignment)
			{
				to_tree(prs, COPY1ST);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else if (!mode_is_array(prs->sx, type) && !mode_is_pointer(prs->sx, type))
			{
				to_tree(prs, mode_is_float(type) ? TAddrtovald : TAddrtoval);
			}

			operands_push(prs, VALUE, type);
		}
		break;

		case VALUE:
		case NUMBER:
			break;
	}
}

item_t parse_braced_init_list(parser *const prs, const item_t type)
{
	token_consume(prs);
	to_tree(prs, type == mode_float ? TStringd : TString);
	node_add_arg(&prs->nd, 0);

	node nd_init_list;
	node_copy(&nd_init_list, &prs->nd);

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

	if (!mode_is_string(prs->sx, stack_pop(&prs->anonymous)))
	{
		parser_error(prs, not_string_in_stanfunc);
	}
}

void must_be_point_string(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	const item_t type = stack_pop(&prs->anonymous);
	if (!(mode_is_pointer(prs->sx, type) && mode_is_string(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
	{
		parser_error(prs, not_point_string_in_stanfunc);
	}
}

void must_be_row(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_array(prs->sx, stack_pop(&prs->anonymous)))
	{
		parser_error(prs, not_array_in_stanfunc);
	}
}

void must_be_int(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_int(stack_pop(&prs->anonymous)))
	{
		parser_error(prs, not_int_in_stanfunc);
	}
}

void must_be_float(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	const item_t type = stack_pop(&prs->anonymous);
	if (mode_is_int(type))
	{
		to_tree(prs, WIDEN);
	}
	else if (!mode_is_float(type))
	{
		parser_error(prs, bad_param_in_stand_func);
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

		type = stack_pop(&prs->anonymous);
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

		type = stack_pop(&prs->anonymous);
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

	if (func == kw_assert)
	{
		must_be_int(prs);
		token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
		must_be_string(prs);
		operands_push(prs, VALUE, mode_void);
	}
	else if (func <= kw_strcpy && func >= kw_strlen) // функции работы со строками
	{
		if (func >= kw_strncat)
		{
			must_be_point_string(prs);
		}
		else
		{
			must_be_string(prs);
		}

		if (func != kw_strlen)
		{
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_string(prs);
			if (func == kw_strncpy || func == kw_strncat || func == kw_strncmp)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
			}
		}

		if (func < kw_strncat)
		{
			operands_push(prs, VALUE, mode_integer);
		}
		else
		{
			operands_push(prs, VALUE, mode_void);
		}
	}
	else if (func >= kw_receive_string && func <= kw_send_int)
	{
		// новые функции Фадеева
		must_be_int(prs);
		if (func == kw_send_int || func == kw_send_string)
		{
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
			operands_push(prs, VALUE, mode_void);
		}
		else if (func == kw_send_float)
		{
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_float(prs);
			operands_push(prs, VALUE, mode_void);
		}
		else
		{
			operands_push(prs, VALUE, func == kw_receive_int
				? mode_integer : func == kw_receive_float
				? mode_float : to_modetab(prs, mode_array, mode_character));
		}
	}
	else if (func >= kw_icon && func <= kw_wifi_connect) // функции Фадеева
	{
		if (func <= kw_pixel && func >= kw_icon)
		{
			must_be_row_of_int(prs);
			if (func != kw_clear)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			}

			if (func == kw_line || func == kw_rectangle || func == kw_ellipse)
			{
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);

				if (func != kw_line)
				{
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
				}
			}
			else if (func == kw_icon || func == kw_pixel)
			{
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);

				if (func == kw_icon)
				{
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
				}
			}
			else if (func == kw_draw_number || func == kw_draw_string)
			{
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);

				if (func == kw_draw_string)
				{
					must_be_string(prs);
				}
				else // kw_draw_number
				{
					must_be_float(prs);
				}
			}
			operands_push(prs, VALUE, mode_void);
		}
		else if (func == kw_setsignal)
		{
			must_be_int(prs);
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
			token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
			operands_push(prs, VALUE, mode_void);
		}
		else if (func == kw_wifi_connect || func == kw_blynk_authorization || func == kw_blynk_notification)
		{
			must_be_string(prs);

			if (func == kw_wifi_connect)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
				must_be_string(prs);
			}

			operands_push(prs, VALUE, mode_void);
		}
		else
		{
			must_be_int(prs);
			if (func != kw_blynk_receive)
			{
				token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);

				if (func == kw_blynk_terminal)
				{
					must_be_string(prs);
				}
				else if (func == kw_blynk_send)
				{
					must_be_int(prs);
				}
				else if (func == kw_blynk_property)
				{
					must_be_string(prs);
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_string(prs);
				}
				else // kw_blynk_lcd
				{
					must_be_int(prs);
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
					token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
					must_be_string(prs);
				}

				operands_push(prs, VALUE, mode_void);
			}
			else
			{
				operands_push(prs, VALUE, mode_integer);
			}
		}
	}
	else if (func == kw_upb)
	{
		must_be_int(prs);
		token_expect_and_consume(prs, comma, no_comma_in_act_params_stanfunc);
		must_be_row(prs);
		operands_push(prs, VALUE, mode_integer);
	}
	else if (func <= kw_msg_send && func >= kw_getnum) // процедуры управления параллельными нитями
	{
		if (func == kw_init || func == kw_destroy || func == kw_exit)
		{
			operands_push(prs, VALUE, mode_void);
		}
		else if (func == kw_msg_receive || func == kw_getnum) // getnum int(), msgreceive msg_info()
		{
			operands_push(prs, VALUE, func == kw_getnum ? mode_integer : mode_msg_info);
		}
		else
		{
			// MSGSEND void(msg_info)  CREATE int(void*(*func)(void*))
			// SEMCREATE int(int)  JOIN,  SLEEP,  SEMWAIT,  SEMPOST void(int)
			// у этих процедур 1 параметр

			if (func == kw_create)
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
				if (ident_get_mode(prs->sx, prs->last_id) != mode_void_pointer)
				{
					parser_error(prs, wrong_arg_in_create);
				}

				const item_t displ = ident_get_displ(prs->sx, prs->last_id);
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

				operands_push(prs, VALUE, mode_integer);
			}
			else
			{
				if (func == kw_msg_send)
				{
					parse_initializer(prs, &prs->nd, mode_msg_info);
					operands_push(prs, VALUE, mode_void);
				}
				else
				{
					must_be_int(prs);

					if (func == kw_sem_create)
					{
						operands_push(prs, VALUE, mode_integer);
					}
					else
					{
						operands_push(prs, VALUE, mode_void);
					}
				}
			}
		}
	}
	else if (func == kw_rand)
	{
		operands_push(prs, VALUE, mode_float);
	}
	else if (func == kw_round)
	{
		must_be_float(prs);
		operands_push(prs, VALUE, mode_integer);
	}
	else if (func == kw_getdigsensor || func == kw_getansensor || func == kw_setmotor || func == kw_setvoltage)
	{
		// GETDIGSENSOR int(int port, int pins[]),
		// GETANSENSOR int (int port, int pin),
		// SETMOTOR и VOLTAGE void (int port, int volt)
		must_be_int(prs);
		token_expect_and_consume(prs, comma, no_comma_in_setmotor);

		if (func == kw_getdigsensor)
		{
			must_be_row_of_int(prs);
			operands_push(prs, VALUE, mode_integer);
		}
		else
		{
			must_be_int(prs);
			if (func == kw_setmotor || func == kw_setvoltage)
			{
				operands_push(prs, VALUE, mode_void);
			}
			else
			{
				operands_push(prs, VALUE, mode_void);
			}
		}
	}
	else if (func == kw_abs)
	{
		parse_assignment_expression_internal(prs);
		to_value(prs);

		if (stack_pop(&prs->anonymous) == mode_integer)
		{
			func = ABSI;
			operands_push(prs, VALUE, mode_integer);
		}
		else
		{
			operands_push(prs, VALUE, mode_float);
		}
	}
	else
	{
		must_be_float(prs);
		operands_push(prs, VALUE, mode_float);
	}

	to_tree(prs, 9500 - func);
	token_expect_and_consume(prs, r_paren, no_rightbr_in_stand_func);
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

	prs->operand_displ = ident_get_displ(prs->sx, (size_t)id);
	node_add_arg(&prs->nd, prs->operand_displ);

	prs->last_id = (size_t)id;
	token_consume(prs);
	operands_push(prs, VARIABLE, ident_get_mode(prs->sx, (size_t)id));
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
 */
void parse_constant(parser *const prs)
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
	operands_push(prs, NUMBER, mode);
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
				const operand_t type = prs->last_type;
				const item_t mode = stack_pop(&prs->anonymous);
				if (!mode_is_pointer(prs->sx, mode))
				{
					parser_error(prs, not_pointer_in_cast);
				}

				token_expect_and_consume(prs, r_paren, no_rightbr_in_cast);
				operands_push(prs, type, mode);
				to_value(prs);
				to_tree(prs, TExprend);
			}
			else
			{
				const size_t old_operators_size = operators_size(prs);
				parse_expression_internal(prs);
				token_expect_and_consume(prs, r_paren, wait_rightbr_in_primary);
				while (old_operators_size < operators_size(prs))
				{
					binary_operation(prs, operators_pop(prs));
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
				operands_push(prs, NUMBER, mode_undefined);
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

	const operand_t peek = prs->last_type;
	const size_t type = (size_t)stack_pop(&prs->anonymous);
	const size_t record_length = (size_t)mode_get(prs->sx, type + 2);
	if (record_length == ITEM_MAX)
	{
		return 0;
	}

	item_t select_displ = 0;
	for (size_t i = 0; i < record_length; i += 2)
	{
		const item_t field_type = mode_get(prs->sx, type + 3 + i);

		if ((size_t)mode_get(prs->sx, type + 4 + i) == prs->lxr->repr)
		{
			operands_push(prs, peek, field_type);
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
	const int old_in_assignment = prs->flag_in_assignment;
	const size_t function_mode = (size_t)stack_pop(&prs->anonymous);

	if (!mode_is_function(prs->sx, function_mode))
	{
		parser_error(prs, call_not_from_function);
		token_skip_until(prs, r_paren | semicolon);
		return;
	}

	const size_t expected_args = (size_t)mode_get(prs->sx, function_mode + 2);
	size_t ref_arg_mode = function_mode + 3;
	size_t actual_args = 0;

	to_tree(prs, TCall1);
	node_add_arg(&prs->nd, expected_args);

	node nd_call;
	node_copy(&nd_call, &prs->nd);

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
	operands_push(prs, VALUE, mode_get(prs->sx, function_mode + 1));
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
	const size_t last_id = prs->last_id;
	int was_func = 0;

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

			if (prs->last_type == VARIABLE)
			{
				node_set_type(&prs->nd, TSliceident);
				node_set_arg(&prs->nd, 0, prs->operand_displ);
			}
			else
			{
				to_tree(prs, TSlice);
			}

			const item_t mode = stack_pop(&prs->anonymous);
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
			operands_push(prs, ADDRESS, elem_type);
		}

		while (prs->token == arrow)
		{
			if (prs->last_type == VARIABLE)
			{
				node_set_type(&prs->nd, TIdenttoval);
			}

			to_tree(prs, TSelect);

			// Здесь мы ожидаем указатель, снимаем указатель со стека и кладем саму структуру
			const item_t type = stack_pop(&prs->anonymous);
			if (!(mode_is_pointer(prs->sx, type) && mode_is_struct(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
			{
				parser_error(prs, get_field_not_from_struct_pointer);
			}

			operands_push(prs, ADDRESS, mode_get(prs->sx, (size_t)type + 1));
			prs->operand_displ = find_field(prs);
			while (prs->token == period)
			{
				prs->operand_displ += find_field(prs);
			}

			node_add_arg(&prs->nd, prs->operand_displ);

			// find_field вернула тип результата через стек, проверим его и вернем обратно
			const item_t field_type = stack_pop(&prs->anonymous);
			if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
			{
				to_tree(prs, TAddrtoval);
			}

			operands_push(prs, ADDRESS, field_type);
		}

		if (prs->token == period)
		{
			const operand_t peek = prs->last_type;
			const item_t type = stack_pop(&prs->anonymous);
			if (!mode_is_struct(prs->sx, type))
			{
				parser_error(prs, select_not_from_struct);
			}

			if (peek == VALUE)
			{
				const size_t length = size_of(prs->sx, type);
				operands_push(prs, VALUE, type);
				prs->operand_displ = 0;
				while (prs->token == period)
				{
					prs->operand_displ += find_field(prs);
				}

				const item_t field_type = stack_pop(&prs->anonymous);
				to_tree(prs, COPYST);
				node_add_arg(&prs->nd, prs->operand_displ);
				node_add_arg(&prs->nd, (item_t)size_of(prs->sx, field_type));
				node_add_arg(&prs->nd, (item_t)length);
				operands_push(prs, VALUE, field_type);
			}
			else if (peek == VARIABLE)
			{
				const item_t sign = prs->operand_displ < 0 ? -1 : 1;
				operands_push(prs, VARIABLE, type);
				while (prs->token == period)
				{
					prs->operand_displ += sign * find_field(prs);
				}

				node_set_arg(&prs->nd, 0, prs->operand_displ);
			}
			else //if (peek == address)
			{
				to_tree(prs, TSelect);
				operands_push(prs, VARIABLE, type);
				prs->operand_displ = 0;
				while (prs->token == period)
				{
					prs->operand_displ += find_field(prs);
				}

				node_add_arg(&prs->nd, prs->operand_displ);
				// find_field вернула тип результата через стек, проверим его и вернем обратно
				const item_t field_type = stack_pop(&prs->anonymous);
				if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
				{
					to_tree(prs, TAddrtoval);
				}

				operands_push(prs, ADDRESS, field_type);
			}
		}
	}

	if (prs->token == plusplus || prs->token == minusminus)
	{
		item_t operator = prs->token == plusplus ? POSTINC : POSTDEC;
		token_consume(prs);

		int is_variable = 0;
		if (prs->last_type == ADDRESS)
		{
			operator += 4;
		}
		else if (prs->last_type == VARIABLE)
		{
			is_variable = 1;
		}
		else
		{
			parser_error(prs, unassignable_inc);
		}

		const item_t type = stack_pop(&prs->anonymous);
		if (!mode_is_int(type) && !mode_is_float(type))
		{
			parser_error(prs, wrong_operand);
		}

		operands_push(prs, VALUE, type);
		float_operation(prs, type, operator);

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
			if (prs->last_type == ADDRESS)
			{
				operator += 4;
			}
			else if (prs->last_type == VARIABLE)
			{
				is_variable = 1;
			}
			else
			{
				parser_error(prs, unassignable_inc);
			}

			const item_t type = stack_pop(&prs->anonymous);
			if (!mode_is_int(type) && !mode_is_float(type))
			{
				parser_error(prs, wrong_operand);
			}

			operands_push(prs, VALUE, type);
			float_operation(prs, type, operator);

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
					if (prs->last_type == VALUE)
					{
						parser_error(prs, wrong_addr);
					}

					if (prs->last_type == VARIABLE)
					{
						node_set_type(&prs->nd, TIdenttoaddr);
					}

					operands_push(prs, VALUE, to_modetab(prs, mode_pointer, stack_pop(&prs->anonymous)));
				}
				break;

				case star:
				{
					if (prs->last_type == VARIABLE)
					{
						node_set_type(&prs->nd, TIdenttoval);
					}

					const item_t type = stack_pop(&prs->anonymous);
					if (!mode_is_pointer(prs->sx, type))
					{
						parser_error(prs, aster_not_for_pointer);
					}

					operands_push(prs, ADDRESS, mode_get(prs->sx, (size_t)type + 1));
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
							const item_t type = stack_pop(&prs->anonymous);
							float_operation(prs, type, UNMINUS);
							operands_push(prs, VALUE, type);
						}
					}
					else
					{
						if (operator != plus)
						{
							to_tree(prs, operator);
						}

						const item_t type = stack_pop(&prs->anonymous);
						if ((operator == tilde || operator == exclaim) && mode_is_float(type))
						{
							parser_error(prs, int_op_for_float);
						}

						operands_push(prs, VALUE, type);
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

uint8_t operator_priority(const token_t operator)
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
	size_t old_operators_size = operators_size(prs);
	int was_operator = 0;

	uint8_t priority = operator_priority(prs->token);
	while (priority)
	{
		was_operator = 1;
		to_value(prs);
		while (old_operators_size < operators_size(prs) && priority <= operators_peek(prs).priority)
		{
			binary_operation(prs, operators_pop(prs));
		}

		if (priority <= 2)
		{
			to_tree(prs, priority == 1 ? ADLOGOR : ADLOGAND);
			node_add_arg(&prs->nd, 0); // FIXME: useless
		}

		operators_push(prs, priority, prs->token, &prs->nd);

		token_consume(prs);
		parse_unary_expression(prs);
		priority = operator_priority(prs->token);
	}

	if (was_operator)
	{
		to_value(prs);
	}

	while (old_operators_size < operators_size(prs))
	{
		binary_operation(prs, operators_pop(prs));
	}
}

void parse_conditional_expression(parser *const prs)
{
	parse_subexpression(prs); // logORexpr();

	if (prs->token == question)
	{
		item_t global_type = 0;
		item_t addr_if = TExprend;

		while (token_try_consume(prs, question))
		{
			to_value(prs);
			if (!mode_is_int(stack_pop(&prs->anonymous)))
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
				if (addr_if == TExprend)
				{
					node_set_type(&prs->nd, NOP);
				}

				const item_t old_addr_if = addr_if;
				addr_if = (item_t)node_save(&prs->nd);
				to_tree(prs, old_addr_if);
			}

			token_expect_and_consume(prs, colon, no_colon_in_cond_expr);
			node_copy(&prs->nd, &nd_condexpr);
			parse_unary_expression(prs);
			parse_subexpression(prs); // logORexpr();	else or elif
		}

		to_value(prs);
		// Это особый случай, когда после TExprend мы храним дополнительную информацию
		prs->nd = node_add_child(&prs->nd, TExprend);

		if (mode_is_float(stack_pop(&prs->anonymous)))
		{
			global_type = mode_float;
		}
		else
		{
			if (addr_if == TExprend)
			{
				node_set_type(&prs->nd, NOP);
			}

			const item_t old_addr_if = addr_if;
			addr_if = (item_t)node_save(&prs->nd);
			to_tree(prs, old_addr_if);
		}

		operands_push(prs, VALUE, global_type);

		if (prs->was_error)
		{
			// Если были ошибки, то нет смысла ставить в дереве нужные адреса
			// Кроме того, никто не гарантирует правильных адресов, можем уйти в бесконечный цикл
			return;
		}

		while (addr_if != TExprend)
		{
			node node_addr = node_load(&prs->sx->tree, (size_t)addr_if);
			node_set_type(&node_addr, mode_is_float(global_type) ? WIDEN : NOP);

			node_addr = node_get_child(&node_addr, 0);
			addr_if = node_get_type(&node_addr);
			node_set_type(&node_addr, TExprend);
		}
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
		const item_t type = prs->left_mode;
		if (mode_is_struct(prs->sx, type) || mode_is_array(prs->sx, type))
		{
			parse_initializer(prs, &prs->nd, type);
			prs->left_mode = type;
		}
		else
		{
			parser_error(prs, init_not_struct);
		}

		operands_push(prs, VALUE, type);
		return;
	}

	parse_unary_expression(prs);
	const operand_t type = prs->last_type;
	prs->left_mode = stack_pop(&prs->anonymous);
	operands_push(prs, type, prs->left_mode);

	if (is_assignment_operator(prs->token))
	{
		const item_t target_displ = prs->operand_displ;
		const operand_t left_type = prs->last_type;
		if (left_type == VALUE)
		{
			parser_error(prs, unassignable);
		}

		item_t operator = prs->token;
		token_consume(prs);

		prs->flag_in_assignment = 1;
		parse_assignment_expression_internal(prs);
		prs->flag_in_assignment = 0;

		// Снимаем типы операндов со стека
		const operand_t right_type = prs->last_type;
		const item_t right_mode = stack_pop(&prs->anonymous);
		const item_t left_mode = stack_pop(&prs->anonymous);
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

			if (right_type == VALUE)
			{
				operator = left_type == VARIABLE ? COPY0STASS : COPY1STASS;
			}
			else
			{
				operator = left_type == VARIABLE
					? right_type == VARIABLE
						? COPY00 : COPY01
					: right_type == VARIABLE
						? COPY10 : COPY11;
			}

			to_tree(prs, operator);
			if (left_type == VARIABLE)
			{
				node_add_arg(&prs->nd, target_displ);
			}
			if (right_type == VARIABLE)
			{
				node_add_arg(&prs->nd, prs->operand_displ);
			}
			node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)left_mode + 1));
			prs->operand_displ = target_displ;
			operands_push(prs, left_type, left_mode);
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
			operands_push(prs, right_type, right_mode);
			to_value(prs);
			stack_pop(&prs->anonymous);

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

			if (left_type == ADDRESS)
			{
				operator += 11;
			}
			float_operation(prs, result_mode, operator);
			if (left_type == VARIABLE)
			{
				prs->operand_displ = target_displ;
				node_add_arg(&prs->nd, target_displ);
			}
			operands_push(prs, VALUE, left_mode);
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
		stack_pop(&prs->anonymous);
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
	return stack_pop(&prs->anonymous);
}

item_t parse_assignment_expression(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_assignment_expression_internal(prs);
	to_value(prs);
	to_tree(prs, TExprend);
	return stack_pop(&prs->anonymous);
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
	return stack_pop(&prs->anonymous);
}

item_t parse_condition(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_expression_internal(prs);
	to_value(prs);
	to_tree(prs, TExprend);
	return stack_pop(&prs->anonymous);
}

void parse_string_literal(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	to_tree(prs, TString);
	node_add_arg(&prs->nd, prs->lxr->num);

	for (int i = 0; i < prs->lxr->num; i++)
	{
		node_add_arg(&prs->nd, prs->lxr->lexstr[i]);
	}

	token_consume(prs);
	operands_push(prs, VALUE, to_modetab(prs, mode_array, mode_character));
}

void parse_insert_widen(parser *const prs)
{
	// Сейчас последней нодой в поддереве выражения является TExprend, просто меняем ее тип
	node_set_type(&prs->nd, WIDEN);
	to_tree(prs, TExprend);
}
