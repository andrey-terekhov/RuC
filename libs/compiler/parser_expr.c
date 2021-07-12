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


static void parse_unary_expression(parser *const prs);
static void parse_assignment_expression_internal(parser *const prs);
static void parse_expression_internal(parser *const prs);
static void parse_constant(parser *const prs);


static inline int operators_push(parser *const prs, const uint8_t priority, const token_t token, const node *const nd)
{
	return stack_push(&prs->stk.priorities, priority)
		|| stack_push(&prs->stk.tokens, token)
		|| stack_push(&prs->stk.nodes, node_save(nd));
}

static inline operator operators_pop(parser *const prs)
{
	operator op;

	op.priority = (uint8_t)stack_pop(&prs->stk.priorities);
	op.token = stack_pop(&prs->stk.tokens);
	op.nd = node_load(&prs->sx->tree, (size_t)stack_pop(&prs->stk.nodes));

	return op;
}

static inline operator operators_peek(parser *const prs)
{
	operator op;

	op.priority = (uint8_t)stack_peek(&prs->stk.priorities);
	op.token = stack_peek(&prs->stk.tokens);
	op.nd = node_load(&prs->sx->tree, (size_t)stack_peek(&prs->stk.nodes));

	return op;
}

static inline size_t operators_size(const parser *const prs)
{
	return stack_size(&prs->stk.tokens);
}

static inline int operands_push(parser *const prs, const operand_t type, const item_t mode)
{
	prs->last_type = type;
	return stack_push(&prs->anonymous, mode);
}


static void node_add_double(node *const nd, const double value)
{
	int64_t num64;
	memcpy(&num64, &value, sizeof(int64_t));

	const int32_t fst = num64 & 0x00000000ffffffff;
	const int32_t snd = (num64 & 0xffffffff00000000) >> 32;

	node_add_arg(nd, fst);
	node_add_arg(nd, snd);
}

static void node_set_double(node *const nd, const size_t index, const double value)
{
	int64_t num64;
	memcpy(&num64, &value, sizeof(int64_t));

	const int32_t fst = num64 & 0x00000000ffffffff;
	const int32_t snd = (num64 & 0xffffffff00000000) >> 32;

	node_set_arg(nd, index, fst);
	node_set_arg(nd, index + 1, snd);
}

static double node_get_double(const node *const nd, const size_t index)
{
	const int64_t fst = (int64_t)node_get_arg(nd, index) & 0x00000000ffffffff;
	const int64_t snd = (int64_t)node_get_arg(nd, index + 1) & 0x00000000ffffffff;
	const int64_t num64 = (snd << 32) | fst;

	double num;
	memcpy(&num, &num64, sizeof(double));
	return num;
}


static inline void float_operation(parser *const prs, const item_t type, const operation_t operation)
{
	to_tree(prs, mode_is_float(type) ? operation_to_float_ver(operation) : operation);
}

static inline bool is_integer_operator(const token_t operator)
{
	switch (operator)
	{
		case TK_EQUAL_EQUAL:
		case TK_EXCLAIM_EQUAL:
		case TK_LESS:
		case TK_LESS_EQUAL:
		case TK_GREATER:
		case TK_GREATER_EQUAL:
			return true;

		default:
			return false;
	}
}

static void binary_operation(parser *const prs, operator operator)
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
		if (token == TK_PIPE_PIPE || token == TK_AMP_AMP || token == TK_PIPE || token == TK_CARET
			|| token == TK_AMP || token == TK_GREATER_GREATER || token == TK_LESS_LESS || token == TK_PERCENT)
		{
			parser_error(prs, int_op_for_float);
		}

		if (mode_is_int(left_mode))
		{
			to_tree(prs, OP_WIDEN1);
		}
		else if (mode_is_int(right_mode))
		{
			to_tree(prs, OP_WIDEN);
		}

		result_mode = mode_float;
	}

	if (token == TK_PIPE_PIPE || token == TK_AMP_AMP)
	{
		to_tree(prs, token_to_binary(token));
		node_add_arg(&prs->nd, 0); // FIXME: useless

		// FIXME: just remove it after MIPS integration
		node_set_arg(&operator.nd, 0, node_save(&prs->nd));
		node_set_arg(&operator.nd, 0, node_get_arg(&operator.nd, 0) + 1);
	}
	else
	{
		float_operation(prs, result_mode, token_to_binary(token));
	}

	if (is_integer_operator(token))
	{
		result_mode = mode_integer;
	}

	operands_push(prs, VALUE, result_mode);
}

static void to_value(parser *const prs)
{
	switch (prs->last_type)
	{
		case VARIABLE:
		{
			const item_t type = stack_pop(&prs->anonymous);
			if (mode_is_struct(prs->sx, type) && !prs->is_in_assignment)
			{
				node_set_type(&prs->nd, OP_COPY0ST);
				node_set_arg(&prs->nd, 0, prs->operand_displ);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else
			{
				node_set_type(&prs->nd, mode_is_float(type) ? operation_to_float_ver(OP_IDENT_TO_VAL) : OP_IDENT_TO_VAL);
			}

			operands_push(prs, VALUE, type);
		}
		break;

		case ADDRESS:
		{
			const item_t type = stack_pop(&prs->anonymous);
			if (mode_is_struct(prs->sx, type) && !prs->is_in_assignment)
			{
				to_tree(prs, OP_COPY1ST);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else if (!mode_is_array(prs->sx, type) && !mode_is_pointer(prs->sx, type))
			{
				float_operation(prs, type, OP_ADDR_TO_VAL);
			}

			operands_push(prs, VALUE, type);
		}
		break;

		case VALUE:
		case NUMBER:
			break;
	}
}

static item_t parse_braced_init_list(parser *const prs, const item_t type)
{
	token_consume(prs);
	float_operation(prs, type, OP_STRING);
	node_add_arg(&prs->nd, 0);

	node nd_init_list;
	node_copy(&nd_init_list, &prs->nd);

	size_t length = 0;
	do
	{
		const int sign = token_try_consume(prs, TK_MINUS) ? -1 : 1;

		if (prs->token == TK_INT_CONST || prs->token == TK_CHAR_CONST)
		{
			if (mode_is_float(type))
			{
				node_add_double(&nd_init_list, sign * prs->lxr->num);
			}
			else
			{
				node_add_arg(&nd_init_list, sign * prs->lxr->num);
			}
			token_consume(prs);
		}
		else if (prs->token == TK_FLOAT_CONST)
		{
			if (mode_is_float(type))
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
			token_skip_until(prs, TK_COMMA | TK_R_BRACE | TK_SEMICOLON);
		}
		length++;
	} while (token_try_consume(prs, TK_COMMA));

	node_set_arg(&nd_init_list, 0, length);
	to_tree(prs, OP_EXPR_END);
	if (!token_try_consume(prs, TK_R_BRACE))
	{
		parser_error(prs, no_comma_or_end);
		token_skip_until(prs, TK_R_BRACE | TK_SEMICOLON);
	}

	return to_modetab(prs, mode_array, type);
}

static void must_be_string(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_string(prs->sx, stack_pop(&prs->anonymous)))
	{
		parser_error(prs, not_string_in_stanfunc);
	}
}

static void must_be_point_string(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	const item_t type = stack_pop(&prs->anonymous);
	if (!(mode_is_pointer(prs->sx, type) && mode_is_string(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
	{
		parser_error(prs, not_point_string_in_stanfunc);
	}
}

static void must_be_row(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_array(prs->sx, stack_pop(&prs->anonymous)))
	{
		parser_error(prs, not_array_in_stanfunc);
	}
}

static void must_be_int(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	if (!mode_is_int(stack_pop(&prs->anonymous)))
	{
		parser_error(prs, not_int_in_stanfunc);
	}
}

static void must_be_float(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	to_value(prs);

	const item_t type = stack_pop(&prs->anonymous);
	if (mode_is_int(type))
	{
		to_tree(prs, OP_WIDEN);
	}
	else if (!mode_is_float(type))
	{
		parser_error(prs, bad_param_in_stand_func);
	}
}

static void must_be_row_of_int(parser *const prs)
{
	item_t type;
	if (prs->token == TK_L_BRACE)
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
			to_tree(prs, OP_ROWING);
			type = to_modetab(prs, mode_array, mode_integer);
		}
	}

	if (!(mode_is_array(prs->sx, type) && mode_is_int(mode_get(prs->sx, (size_t)type + 1))))
	{
		parser_error(prs, not_rowofint_in_stanfunc);
	}
}

static void must_be_row_of_float(parser *const prs)
{
	item_t type;
	if (prs->token == TK_L_BRACE)
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
			to_tree(prs, OP_ROWING_D);
			type = to_modetab(prs, mode_array, mode_float);
		}
	}

	if (!(mode_is_array(prs->sx, type) && mode_get(prs->sx, (size_t)type + 1) == mode_float))
	{
		parser_error(prs, not_rowoffloat_in_stanfunc);
	}
}

static void parse_standard_function_call(parser *const prs)
{
	const token_t func = prs->token;
	token_consume(prs);

	if (!token_try_consume(prs, TK_L_PAREN))
	{
		parser_error(prs, no_leftbr_in_stand_func);
		token_skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
		return;
	}

	if (func == TK_ASSERT)
	{
		must_be_int(prs);
		token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
		must_be_string(prs);
		operands_push(prs, VALUE, mode_void);
	}
	else if (func > BEGIN_TK_STR && func < END_TK_STR)
	{
		if (func == TK_STRCPY || func == TK_STRNCPY || func == TK_STRCAT || func == TK_STRNCAT)
		{
			must_be_point_string(prs);
		}
		else
		{
			must_be_string(prs);
		}

		if (func != TK_STRLEN)
		{
			token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
			must_be_string(prs);
			if (func == TK_STRNCPY || func == TK_STRNCAT || func == TK_STRNCMP)
			{
				token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
				must_be_int(prs);
			}
		}

		if (func == TK_STRCMP || func == TK_STRNCMP || func == TK_STRSTR || func == TK_STRLEN)
		{
			operands_push(prs, VALUE, mode_integer);
		}
		else
		{
			operands_push(prs, VALUE, mode_void);
		}
	}
	else if (func > BEGIN_TK_ROBOT && func < END_TK_ROBOT)
	{
		// новые функции Фадеева
		must_be_int(prs);
		if (func == TK_ROBOT_SEND_INT || func == TK_ROBOT_SEND_STRING)
		{
			token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
			must_be_row_of_int(prs);
			operands_push(prs, VALUE, mode_void);
		}
		else if (func == TK_ROBOT_SEND_FLOAT)
		{
			token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
			must_be_row_of_float(prs);
			operands_push(prs, VALUE, mode_void);
		}
		else
		{
			operands_push(prs, VALUE, func == TK_ROBOT_RECEIVE_INT
											? mode_integer
											: func == TK_ROBOT_RECEIVE_FLOAT
												? mode_float
												: to_modetab(prs, mode_array, mode_character));

		}
	}
	else if (func > BEGIN_TK_THREAD && func < END_TK_THREAD)
	{
		if (func == TK_INIT || func == TK_DESTROY || func == TK_EXIT)
		{
			operands_push(prs, VALUE, mode_void);
		}
		else if (func == TK_MSG_RECEIVE || func == TK_GETNUM) // getnum int(), msgreceive msg_info()
		{
			operands_push(prs, VALUE, func == TK_GETNUM ? mode_integer : mode_msg_info);
		}
		else
		{
			// MSGSEND void(msg_info)  CREATE int(void*(*func)(void*))
			// SEMCREATE int(int)  JOIN,  SLEEP,  SEMWAIT,  SEMPOST void(int)
			// у этих процедур 1 параметр

			if (func == TK_CREATE)
			{
				if (!token_try_consume(prs, TK_IDENTIFIER))
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
					to_tree(prs, OP_IDENT_TO_VAL);
					node_add_arg(&prs->nd, -displ);
				}
				else
				{
					to_tree(prs, OP_CONST);
					node_add_arg(&prs->nd, displ);
				}

				operands_push(prs, VALUE, mode_integer);
			}
			else
			{
				if (func == TK_MSG_SEND)
				{
					parse_initializer(prs, &prs->nd, mode_msg_info);
					operands_push(prs, VALUE, mode_void);
				}
				else
				{
					must_be_int(prs);

					if (func == TK_SEM_CREATE)
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
	else if (func == TK_UPB)
	{
		must_be_int(prs);
		token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
		must_be_row(prs);
		operands_push(prs, VALUE, mode_integer);
	}
	else if (func == TK_RAND)
	{
			operands_push(prs, VALUE, mode_float);
	}
	else if (func == TK_ROUND)
	{
		must_be_float(prs);
		operands_push(prs, VALUE, mode_integer);
	}
	else if (func == TK_ABS)
	{
		parse_assignment_expression_internal(prs);
		to_value(prs);

		if (stack_pop(&prs->anonymous) == mode_integer)
		{
			operands_push(prs, VALUE, mode_integer);
			to_tree(prs, OP_ABSI);
			token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_stand_func);
			return;
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

	to_tree(prs, token_to_function(func));
	token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_stand_func);
}

/**
 *	Parse identifier [C99 6.5.1p1]
 *
 *	@param	prs			Parser structure
 *
 *	@return	Index of parsed identifier in identifiers table
 */
static size_t parse_identifier(parser *const prs)
{
	const item_t id = repr_get_reference(prs->sx, prs->lxr->repr);
	if (id == ITEM_MAX)
	{
		parser_error(prs, ident_is_not_declared, repr_get_name(prs->sx, prs->lxr->repr));
	}

	to_tree(prs, OP_IDENT);

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
static void parse_constant(parser *const prs)
{
	item_t mode = mode_undefined;
	switch (prs->token)
	{
		case TK_CHAR_CONST:
		{
			to_tree(prs, OP_CONST);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_character;
		}
		break;

		case TK_INT_CONST:
		{
			to_tree(prs, OP_CONST);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_integer;
		}
		break;

		case TK_FLOAT_CONST:
		{
			to_tree(prs, OP_CONST_D);
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
static void parse_primary_expression(parser *const prs)
{
	switch (prs->token)
	{
		case TK_IDENTIFIER:
			parse_identifier(prs);
			break;

		case TK_CHAR_CONST:
		case TK_INT_CONST:
		case TK_FLOAT_CONST:
			parse_constant(prs);
			break;

		case TK_STRING:
			parse_string_literal(prs, &prs->nd);
			break;

		case TK_L_PAREN:
		{
			token_consume(prs);
			if (token_try_consume(prs, TK_VOID))
			{
				token_expect_and_consume(prs, TK_STAR, no_mult_in_cast);

				parse_unary_expression(prs);
				const operand_t type = prs->last_type;
				const item_t mode = stack_pop(&prs->anonymous);
				if (!mode_is_pointer(prs->sx, mode))
				{
					parser_error(prs, not_pointer_in_cast);
				}

				token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_cast);
				operands_push(prs, type, mode);
				to_value(prs);
				to_tree(prs, OP_EXPR_END);
			}
			else
			{
				const size_t old_operators_size = operators_size(prs);
				parse_expression_internal(prs);
				token_expect_and_consume(prs, TK_R_PAREN, wait_rightbr_in_primary);
				while (old_operators_size < operators_size(prs))
				{
					binary_operation(prs, operators_pop(prs));
				}
			}
		}
		break;

		default:
			if (prs->token > BEGIN_TK_FUNC && prs->token < END_TK_FUNC)
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
static item_t find_field(parser *const prs)
{
	token_consume(prs);
	token_expect_and_consume(prs, TK_IDENTIFIER, after_dot_must_be_ident);

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
static void parse_function_call(parser *const prs, const size_t function_id)
{
	const bool old_in_assignment = prs->is_in_assignment;
	const size_t function_mode = (size_t)stack_pop(&prs->anonymous);

	if (!mode_is_function(prs->sx, function_mode))
	{
		parser_error(prs, call_not_from_function);
		token_skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
		return;
	}

	const size_t expected_args = (size_t)mode_get(prs->sx, function_mode + 2);
	size_t ref_arg_mode = function_mode + 3;
	size_t actual_args = 0;

	to_tree(prs, OP_CALL1);
	node_add_arg(&prs->nd, expected_args);

	node nd_call1;
	node_copy(&nd_call1, &prs->nd);

	if (!token_try_consume(prs, TK_R_PAREN))
	{
		do
		{
			node_copy(&prs->nd, &nd_call1);
			const item_t expected_arg_mode = mode_get(prs->sx, ref_arg_mode);

			if (mode_is_function(prs->sx, expected_arg_mode))
			{
				if (prs->token != TK_IDENTIFIER)
				{
					parser_error(prs, act_param_not_ident);
					token_skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
					continue;
				}

				const size_t id = parse_identifier(prs);
				if (ident_get_mode(prs->sx, id) != expected_arg_mode)
				{
					parser_error(prs, diff_formal_param_type_and_actual);
					token_skip_until(prs, TK_COMMA | TK_R_PAREN | TK_SEMICOLON);
					continue;
				}

				const item_t displ = ident_get_displ(prs->sx, id);
				node_set_type(&prs->nd, displ < 0 ? OP_IDENT_TO_VAL : OP_CONST);
				node_set_arg(&prs->nd, 0, llabs(displ));
				to_tree(prs, OP_EXPR_END);
			}
			else if (mode_is_array(prs->sx, expected_arg_mode) && prs->token == TK_L_BRACE)
			{
				parse_braced_init_list(prs, mode_get(prs->sx, (size_t)expected_arg_mode + 1));
			}
			else
			{
				prs->is_in_assignment = false;
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
		} while (token_try_consume(prs, TK_COMMA));

		token_expect_and_consume(prs, TK_R_PAREN, wrong_number_of_params);
	}

	if (expected_args != actual_args)
	{
		parser_error(prs, wrong_number_of_params);
	}

	prs->is_in_assignment = old_in_assignment;
	node nd_call2 = node_add_child(&nd_call1, OP_CALL2);
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
static void parse_postfix_expression(parser *const prs)
{
	const size_t last_id = prs->last_id;
	bool was_func = false;

	if (token_try_consume(prs, TK_L_PAREN))
	{
		was_func = true;
		parse_function_call(prs, last_id);
	}

	while (prs->token == TK_L_SQUARE || prs->token == TK_ARROW || prs->token == TK_PERIOD)
	{
		while (token_try_consume(prs, TK_L_SQUARE))
		{
			if (was_func)
			{
				parser_error(prs, slice_from_func);
			}

			if (prs->last_type == VARIABLE)
			{
				node_set_type(&prs->nd, OP_SLICE_IDENT);
				node_set_arg(&prs->nd, 0, prs->operand_displ);
			}
			else
			{
				to_tree(prs, OP_SLICE);
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

			token_expect_and_consume(prs, TK_R_SQUARE, no_rightsqbr_in_slice);
			operands_push(prs, ADDRESS, elem_type);
		}

		while (prs->token == TK_ARROW)
		{
			if (prs->last_type == VARIABLE)
			{
				node_set_type(&prs->nd, OP_IDENT_TO_VAL);
			}

			to_tree(prs, OP_SELECT);

			// Здесь мы ожидаем указатель, снимаем указатель со стека и кладем саму структуру
			const item_t type = stack_pop(&prs->anonymous);
			if (!(mode_is_pointer(prs->sx, type) && mode_is_struct(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
			{
				parser_error(prs, get_field_not_from_struct_pointer);
			}

			operands_push(prs, ADDRESS, mode_get(prs->sx, (size_t)type + 1));
			prs->operand_displ = find_field(prs);
			while (prs->token == TK_PERIOD)
			{
				prs->operand_displ += find_field(prs);
			}

			node_add_arg(&prs->nd, prs->operand_displ);

			// find_field вернула тип результата через стек, проверим его и вернем обратно
			const item_t field_type = stack_pop(&prs->anonymous);
			if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
			{
				to_tree(prs, OP_ADDR_TO_VAL);
			}

			operands_push(prs, ADDRESS, field_type);
		}

		if (prs->token == TK_PERIOD)
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
				while (prs->token == TK_PERIOD)
				{
					prs->operand_displ += find_field(prs);
				}

				const item_t field_type = stack_pop(&prs->anonymous);
				to_tree(prs, OP_COPYST);
				node_add_arg(&prs->nd, prs->operand_displ);
				node_add_arg(&prs->nd, (item_t)size_of(prs->sx, field_type));
				node_add_arg(&prs->nd, (item_t)length);
				operands_push(prs, VALUE, field_type);
			}
			else if (peek == VARIABLE)
			{
				const item_t sign = prs->operand_displ < 0 ? -1 : 1;
				operands_push(prs, VARIABLE, type);
				while (prs->token == TK_PERIOD)
				{
					prs->operand_displ += sign * find_field(prs);
				}

				node_set_arg(&prs->nd, 0, prs->operand_displ);
			}
			else //if (peek == address)
			{
				to_tree(prs, OP_SELECT);
				operands_push(prs, VARIABLE, type);
				prs->operand_displ = 0;
				while (prs->token == TK_PERIOD)
				{
					prs->operand_displ += find_field(prs);
				}

				node_add_arg(&prs->nd, prs->operand_displ);
				// find_field вернула тип результата через стек, проверим его и вернем обратно
				const item_t field_type = stack_pop(&prs->anonymous);
				if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
				{
					to_tree(prs, OP_ADDR_TO_VAL);
				}

				operands_push(prs, ADDRESS, field_type);
			}
		}
	}

	if (prs->token == TK_PLUS_PLUS || prs->token == TK_MINUS_MINUS)
	{
		operation_t operator = prs->token == TK_PLUS_PLUS ? OP_POST_INC : OP_POST_DEC;
		token_consume(prs);

		bool is_variable = false;
		if (prs->last_type == ADDRESS)
		{
			operator = operation_to_address_ver(operator);
		}
		else if (prs->last_type == VARIABLE)
		{
			is_variable = true;
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

static void parse_unary_expression(parser *const prs)
{
	token_t token = prs->token;
	switch (token)
	{
		case TK_PLUS_PLUS:
		case TK_MINUS_MINUS:
		{
			token_consume(prs);
			parse_unary_expression(prs);
			operation_t operator = token_to_unary(token);

			bool is_variable = false;
			if (prs->last_type == ADDRESS)
			{
				operator = operation_to_address_ver(operator);
			}
			else if (prs->last_type == VARIABLE)
			{
				is_variable = true;
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

		case TK_EXCLAIM:
		case TK_TILDE:
		case TK_PLUS:
		case TK_MINUS:
		case TK_AMP:
		case TK_STAR:
		{
			token_consume(prs);
			parse_unary_expression(prs);
			switch (token)
			{
				case TK_AMP:
				{
					if (prs->last_type == VALUE)
					{
						parser_error(prs, wrong_addr);
					}

					if (prs->last_type == VARIABLE)
					{
						node_set_type(&prs->nd, OP_IDENT_TO_ADDR);
					}

					operands_push(prs, VALUE, to_modetab(prs, mode_pointer, stack_pop(&prs->anonymous)));
				}
				break;

				case TK_STAR:
				{
					if (prs->last_type == VARIABLE)
					{
						node_set_type(&prs->nd, OP_IDENT_TO_VAL);
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
					if (token == TK_MINUS)
					{
						if (node_get_type(&prs->nd) == OP_CONST)
						{
							node_set_arg(&prs->nd, 0, -node_get_arg(&prs->nd, 0));
						}
						else if (node_get_type(&prs->nd) == OP_CONST_D)
						{
							node_set_double(&prs->nd, 0, -node_get_double(&prs->nd, 0));
						}
						else
						{
							const item_t type = stack_pop(&prs->anonymous);
							float_operation(prs, type, OP_UNMINUS);
							operands_push(prs, VALUE, type);
						}
					}
					else
					{
						if (token != TK_PLUS)
						{
							to_tree(prs, token_to_unary(token));
						}

						const item_t type = stack_pop(&prs->anonymous);
						if ((token == TK_TILDE || token == TK_EXCLAIM) && mode_is_float(type))
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

static inline uint8_t operator_priority(const token_t operator)
{
	switch (operator)
	{
		case TK_PIPE_PIPE:			// '||'
			return 1;

		case TK_AMP_AMP:			// '&&'
			return 2;

		case TK_PIPE:				// '|'
			return 3;

		case TK_CARET:				// '^'
			return 4;

		case TK_AMP:				// '&'
			return 5;

		case TK_EQUAL_EQUAL:		// '=='
		case TK_EXCLAIM_EQUAL:		// '!='
			return 6;

		case TK_LESS:				// '<'
		case TK_GREATER:			// '<'
		case TK_LESS_EQUAL:			// '<='
		case TK_GREATER_EQUAL:		// '>='
			return 7;

		case TK_LESS_LESS:			// '<<'
		case TK_GREATER_GREATER:	// '>>'
			return 8;

		case TK_PLUS:				// '+'
		case TK_MINUS:				// '-'
			return 9;

		case TK_STAR:				// '*'
		case TK_SLASH:				// '/'
		case TK_PERCENT:			// '%'
			return 10;

		default:
			return 0;
	}
}

static inline bool is_int_assignment_operator(const token_t operator)
{
	switch (operator)
	{
		case TK_PERCENT_EQUAL:			// '%='
		case TK_LESS_LESS_EQUAL:		// '<<='
		case TK_GREATER_GREATER_EQUAL:	// '>>='
		case TK_AMP_EQUAL:				// '&='
		case TK_PIPE_EQUAL:				// '|='
		case TK_CARET_EQUAL:			// '^='
			return true;

		default:
			return false;
	}
}


static inline bool is_assignment_operator(const token_t operator)
{
	switch (operator)
	{
		case TK_EQUAL:					// '='
		case TK_STAR_EQUAL:				// '*='
		case TK_SLASH_EQUAL:			// '/='
		case TK_PLUS_EQUAL:				// '+='
		case TK_MINUS_EQUAL:			// '-='
			return true;

		default:
			return is_int_assignment_operator(operator);
	}
}

static void parse_subexpression(parser *const prs)
{
	size_t old_operators_size = operators_size(prs);
	bool was_operator = false;

	uint8_t priority = operator_priority(prs->token);
	while (priority)
	{
		was_operator = true;
		to_value(prs);
		while (old_operators_size < operators_size(prs) && priority <= operators_peek(prs).priority)
		{
			binary_operation(prs, operators_pop(prs));
		}

		if (priority <= 2)
		{
			to_tree(prs, priority == 1 ? OP_AD_LOG_OR : OP_AD_LOG_AND);
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

static void parse_conditional_expression(parser *const prs)
{
	parse_subexpression(prs); // logORexpr();

	if (prs->token == TK_QUESTION)
	{
		item_t global_type = 0;
		item_t addr_if = OP_EXPR_END;

		while (token_try_consume(prs, TK_QUESTION))
		{
			to_value(prs);
			if (!mode_is_int(stack_pop(&prs->anonymous)))
			{
				parser_error(prs, float_in_condition);
			}

			to_tree(prs, OP_CONDITIONAL);
			node nd_conditional;
			node_copy(&nd_conditional, &prs->nd);
			const item_t expr_type = parse_condition(prs, &nd_conditional); // then

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
				if (addr_if == OP_EXPR_END)
				{
					node_set_type(&prs->nd, OP_NOP);
				}

				const item_t old_addr_if = addr_if;
				addr_if = (item_t)node_save(&prs->nd);
				to_tree(prs, old_addr_if);
			}

			token_expect_and_consume(prs, TK_COLON, no_colon_in_cond_expr);
			node_copy(&prs->nd, &nd_conditional);
			parse_unary_expression(prs);
			parse_subexpression(prs); // logORexpr();	else or elif
		}

		to_value(prs);
		// Это особый случай, когда после OP_EXPR_END мы храним дополнительную информацию
		prs->nd = node_add_child(&prs->nd, OP_EXPR_END);

		if (mode_is_float(stack_pop(&prs->anonymous)))
		{
			global_type = mode_float;
		}
		else
		{
			if (addr_if == OP_EXPR_END)
			{
				node_set_type(&prs->nd, OP_NOP);
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

		while (addr_if != OP_EXPR_END)
		{
			node node_addr = node_load(&prs->sx->tree, (size_t)addr_if);
			node_set_type(&node_addr, mode_is_float(global_type) ? OP_WIDEN : OP_NOP);

			node_addr = node_get_child(&node_addr, 0);
			addr_if = node_get_type(&node_addr);
			node_set_type(&node_addr, OP_EXPR_END);
		}
	}
}

static inline void assignment_to_void(parser *const prs)
{
	node_set_type(&prs->nd, operation_to_void_ver(node_get_type(&prs->nd)));
}

static void parse_assignment_expression_internal(parser *const prs)
{
	if (prs->token == TK_L_BRACE)
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

		token_t token = prs->token;
		operation_t operator = token_to_binary(token);
		token_consume(prs);

		prs->is_in_assignment = true;
		parse_assignment_expression_internal(prs);
		prs->is_in_assignment = false;

		// Снимаем типы операндов со стека
		const operand_t right_type = prs->last_type;
		const item_t right_mode = stack_pop(&prs->anonymous);
		const item_t left_mode = stack_pop(&prs->anonymous);
		item_t result_mode = right_mode;

		if (is_int_assignment_operator(token) && (mode_is_float(left_mode) || mode_is_float(right_mode)))
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

			if (token != TK_EQUAL) // в структуру можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
			}

			if (right_type == VALUE)
			{
				operator = left_type == VARIABLE ? OP_COPY0ST_ASSIGN : OP_COPY1ST_ASSIGN;
			}
			else
			{
				operator = left_type == VARIABLE
					? right_type == VARIABLE
						? OP_COPY00 : OP_COPY01
					: right_type == VARIABLE
						? OP_COPY10 : OP_COPY11;
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
			if (mode_is_pointer(prs->sx, left_mode) && token != TK_EQUAL)
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
				to_tree(prs, OP_WIDEN);
				result_mode = mode_float;
			}
			if (mode_is_pointer(prs->sx, left_mode) && mode_is_pointer(prs->sx, right_mode) && left_mode != right_mode)
			{
				// проверка нужна только для указателей
				parser_error(prs, type_missmatch);
			}

			if (left_type == ADDRESS)
			{
				operator = operation_to_address_ver(operator);
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

static void parse_expression_internal(parser *const prs)
{
	parse_assignment_expression_internal(prs);
	while (token_try_consume(prs, TK_COMMA))
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
	to_tree(prs, OP_EXPR_END);
	return stack_pop(&prs->anonymous);
}

item_t parse_assignment_expression(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_assignment_expression_internal(prs);
	to_value(prs);
	to_tree(prs, OP_EXPR_END);
	return stack_pop(&prs->anonymous);
}

item_t parse_parenthesized_expression(parser *const prs, node *const parent)
{
	token_expect_and_consume(prs, TK_L_PAREN, cond_must_be_in_brkts);
	const item_t condition_type = parse_condition(prs, parent);
	token_expect_and_consume(prs, TK_R_PAREN, cond_must_be_in_brkts);
	return condition_type;
}

item_t parse_constant_expression(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_unary_expression(prs);
	parse_conditional_expression(prs);
	to_value(prs);
	to_tree(prs, OP_EXPR_END);
	return stack_pop(&prs->anonymous);
}

item_t parse_condition(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_expression_internal(prs);
	to_value(prs);
	to_tree(prs, OP_EXPR_END);
	return stack_pop(&prs->anonymous);
}

void parse_string_literal(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	to_tree(prs, OP_STRING);
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
	// Сейчас последней нодой в поддереве выражения является OP_EXPR_END, просто меняем ее тип
	node_set_type(&prs->nd, OP_WIDEN);
	to_tree(prs, OP_EXPR_END);
}
