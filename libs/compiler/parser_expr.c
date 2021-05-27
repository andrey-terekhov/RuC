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


void float_operation(parser *const prs, const item_t type, const node_t operation)
{
	if (mode_is_float(type))
	{
		to_tree(prs, node_float_operator(operation));
	}
	else
	{
		to_tree(prs, operation);
	}
}

int is_integer_operator(const token_t operator)
{
	switch (operator)
	{
		case TOK_EQUALEQUAL:
		case TOK_EXCLAIMEQUAL:
		case TOK_LESS:
		case TOK_LESSEQUAL:
		case TOK_GREATER:
		case TOK_GREATEREQUAL:
			return 1;

		default:
			return 0;
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
		if (token == TOK_PIPEPIPE || token == TOK_AMPAMP || token == TOK_PIPE || token == TOK_CARET || token == TOK_AMP
			|| token == TOK_GREATERGREATER || token == TOK_LESSLESS || token == TOK_PERCENT)
		{
			parser_error(prs, int_op_for_float);
		}

		if (mode_is_int(left_mode))
		{
			to_tree(prs, ND_WIDEN1);
		}
		else if (mode_is_int(right_mode))
		{
			to_tree(prs, ND_WIDEN);
		}

		result_mode = mode_float;
	}

	if (token == TOK_PIPEPIPE || token == TOK_AMPAMP)
	{
		to_tree(prs, token_to_node(token));
		node_add_arg(&prs->nd, 0); // FIXME: useless

		// FIXME: just remove it after MIPS integration
		node_set_arg(&operator.nd, 0, node_save(&prs->nd));
		node_set_arg(&operator.nd, 0, node_get_arg(&operator.nd, 0) + 1);
	}
	else
	{
		float_operation(prs, result_mode, token_to_node(token));
	}

	if (is_integer_operator(token))
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
				node_set_type(&prs->nd, ND_COPY0ST);
				node_set_arg(&prs->nd, 0, prs->operand_displ);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else
			{
				node_set_type(&prs->nd, mode_is_float(type) ? ND_IDENTTOVALD : ND_IDENTTOVAL);
			}

			operands_push(prs, VALUE, type);
		}
		break;

		case ADDRESS:
		{
			const item_t type = stack_pop(&prs->anonymous);
			if (mode_is_struct(prs->sx, type) && !prs->flag_in_assignment)
			{
				to_tree(prs, ND_COPY1ST);
				node_add_arg(&prs->nd, mode_get(prs->sx, (size_t)type + 1));
			}
			else if (!mode_is_array(prs->sx, type) && !mode_is_pointer(prs->sx, type))
			{
				to_tree(prs, mode_is_float(type) ? ND_ADDRTOVALD : ND_ADDRTOVAL);
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
	to_tree(prs, mode_is_float(type) ? ND_STRINGD : ND_STRING);
	node_add_arg(&prs->nd, 0);

	node nd_init_list;
	node_copy(&nd_init_list, &prs->nd);

	size_t length = 0;
	do
	{
		const int sign = token_try_consume(prs, TOK_MINUS) ? -1 : 1;

		if (prs->token == TOK_INT_CONST || prs->token == TOK_CHAR_CONST)
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
		else if (prs->token == TOK_FLOAT_CONST)
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
			token_skip_until(prs, TOK_COMMA | TOK_RBRACE | TOK_SEMICOLON);
		}
		length++;
	} while (token_try_consume(prs, TOK_COMMA));

	node_set_arg(&nd_init_list, 0, length);
	to_tree(prs, ND_EXPRESSION_END);
	if (!token_try_consume(prs, TOK_RBRACE))
	{
		parser_error(prs, no_comma_or_end);
		token_skip_until(prs, TOK_RBRACE | TOK_SEMICOLON);
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
		to_tree(prs, ND_WIDEN);
	}
	else if (!mode_is_float(type))
	{
		parser_error(prs, bad_param_in_stand_func);
	}
}

void must_be_row_of_int(parser *const prs)
{
	item_t type;
	if (prs->token == TOK_LBRACE)
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
			to_tree(prs, ND_ROWING);
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
	if (prs->token == TOK_LBRACE)
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
			to_tree(prs, ND_ROWINGD);
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
	const token_t func = prs->token;
	token_consume(prs);

	if (!token_try_consume(prs, TOK_LPAREN))
	{
		parser_error(prs, no_leftbr_in_stand_func);
		token_skip_until(prs, TOK_RPAREN | TOK_SEMICOLON);
		return;
	}

	switch (func)
	{
		case TOK_ASSERT:
		{
			must_be_int(prs);
			token_expect_and_consume(prs, TOK_COMMA, no_comma_in_act_params_stanfunc);
			must_be_string(prs);
			operands_push(prs, VALUE, mode_void);
		}
		break;

		case TOK_STRCPY:
		case TOK_STRNCPY:
		case TOK_STRCAT:
		case TOK_STRNCAT:
		case TOK_STRCMP:
		case TOK_STRNCMP:
		case TOK_STRSTR:
		case TOK_STRLEN:
		{
			if (func == TOK_STRCPY || func == TOK_STRNCPY || func == TOK_STRCAT || func == TOK_STRNCAT)
			{
				must_be_point_string(prs);
			}
			else
			{
				must_be_string(prs);
			}

			if (func != TOK_STRLEN)
			{
				token_expect_and_consume(prs, TOK_COMMA, no_comma_in_act_params_stanfunc);
				must_be_string(prs);
				if (func == TOK_STRNCPY || func == TOK_STRNCAT || func == TOK_STRNCMP)
				{
					token_expect_and_consume(prs, TOK_COMMA, no_comma_in_act_params_stanfunc);
					must_be_int(prs);
				}
			}

			if (func == TOK_STRCMP || func == TOK_STRNCMP || func == TOK_STRSTR || func == TOK_STRLEN)
			{
				operands_push(prs, VALUE, mode_integer);
			}
			else
			{
				operands_push(prs, VALUE, mode_void);
			}
		}
		break;

		case TOK_SEND_INT:
		case TOK_SEND_FLOAT:
		case TOK_SEND_STRING:
		case TOK_RECEIVE_INT:
		case TOK_RECEIVE_FLOAT:
		case TOK_RECEIVE_STRING:
		{
			// новые функции Фадеева
			must_be_int(prs);
			if (func == TOK_SEND_INT || func == TOK_SEND_STRING)
			{
				token_expect_and_consume(prs, TOK_COMMA, no_comma_in_act_params_stanfunc);
				must_be_row_of_int(prs);
				operands_push(prs, VALUE, mode_void);
			}
			else if (func == TOK_SEND_FLOAT)
			{
				token_expect_and_consume(prs, TOK_COMMA, no_comma_in_act_params_stanfunc);
				must_be_row_of_float(prs);
				operands_push(prs, VALUE, mode_void);
			}
			else
			{
				operands_push(prs, VALUE, func == TOK_RECEIVE_INT
							  ? mode_integer : func == TOK_RECEIVE_FLOAT
							  ? mode_float : to_modetab(prs, mode_array, mode_character));
			}
		}
		break;

		case TOK_UPB:
		{
			must_be_int(prs);
			token_expect_and_consume(prs, TOK_COMMA, no_comma_in_act_params_stanfunc);
			must_be_row(prs);
			operands_push(prs, VALUE, mode_integer);
		}
		break;

		case TOK_MSG_SEND:
		case TOK_MSG_RECEIVE:
		case TOK_JOIN:
		case TOK_SLEEP:
		case TOK_SEMCREATE:
		case TOK_SEMWAIT:
		case TOK_SEMPOST:
		case TOK_CREATE:
		case TOK_INIT:
		case TOK_DESTROY:
		case TOK_EXIT:
		case TOK_GETNUM:
		{
			if (func == TOK_INIT || func == TOK_DESTROY || func == TOK_EXIT)
			{
				operands_push(prs, VALUE, mode_void);
			}
			else if (func == TOK_MSG_RECEIVE || func == TOK_GETNUM) // getnum int(), msgreceive msg_info()
			{
				operands_push(prs, VALUE, func == TOK_GETNUM ? mode_integer : mode_msg_info);
			}
			else
			{
				// MSGSEND void(msg_info)  CREATE int(void*(*func)(void*))
				// SEMCREATE int(int)  JOIN,  SLEEP,  SEMWAIT,  SEMPOST void(int)
				// у этих процедур 1 параметр

				if (func == TOK_CREATE)
				{
					if (!token_try_consume(prs, TOK_IDENTIFIER))
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
						to_tree(prs, ND_IDENTTOVAL);
						node_add_arg(&prs->nd, -displ);
					}
					else
					{
						to_tree(prs, ND_CONST);
						node_add_arg(&prs->nd, displ);
					}

					operands_push(prs, VALUE, mode_integer);
				}
				else
				{
					if (func == TOK_MSG_SEND)
					{
						parse_initializer(prs, &prs->nd, mode_msg_info);
						operands_push(prs, VALUE, mode_void);
					}
					else
					{
						must_be_int(prs);

						if (func == TOK_SEMCREATE)
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
		break;

		case TOK_RAND:
			operands_push(prs, VALUE, mode_float);
			break;

		case TOK_ROUND:
		{
			must_be_float(prs);
			operands_push(prs, VALUE, mode_integer);
		}
		break;

		case TOK_ABS:
		{
			parse_assignment_expression_internal(prs);
			to_value(prs);

			if (stack_pop(&prs->anonymous) == mode_integer)
			{
				operands_push(prs, VALUE, mode_integer);
				to_tree(prs, ND_ABSI);
				token_expect_and_consume(prs, TOK_RPAREN, no_rightbr_in_stand_func);
				return;
			}
			else
			{
				operands_push(prs, VALUE, mode_float);
			}
		}
		break;

		default:
		{
			must_be_float(prs);
			operands_push(prs, VALUE, mode_float);
		}
		break;
	}

	to_tree(prs, token_to_node(func));
	token_expect_and_consume(prs, TOK_RPAREN, no_rightbr_in_stand_func);
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

	to_tree(prs, ND_IDENT);

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
		case TOK_CHAR_CONST:
		{
			to_tree(prs, ND_CONST);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_character;
		}
		break;

		case TOK_INT_CONST:
		{
			to_tree(prs, ND_CONST);
			node_add_arg(&prs->nd, prs->lxr->num);
			mode = mode_integer;
		}
		break;

		case TOK_FLOAT_CONST:
		{
			to_tree(prs, ND_CONSTD);
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
 *		TOK_IDENTIFIER
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
		case TOK_IDENTIFIER:
			parse_identifier(prs);
			break;

		case TOK_CHAR_CONST:
		case TOK_INT_CONST:
		case TOK_FLOAT_CONST:
			parse_constant(prs);
			break;

		case TOK_STRING:
			parse_string_literal(prs, &prs->nd);
			break;

		case TOK_LPAREN:
		{
			token_consume(prs);
			if (token_try_consume(prs, TOK_VOID))
			{
				token_expect_and_consume(prs, TOK_STAR, no_mult_in_cast);

				parse_unary_expression(prs);
				const operand_t type = prs->last_type;
				const item_t mode = stack_pop(&prs->anonymous);
				if (!mode_is_pointer(prs->sx, mode))
				{
					parser_error(prs, not_pointer_in_cast);
				}

				token_expect_and_consume(prs, TOK_RPAREN, no_rightbr_in_cast);
				operands_push(prs, type, mode);
				to_value(prs);
				to_tree(prs, ND_EXPRESSION_END);
			}
			else
			{
				const size_t old_operators_size = operators_size(prs);
				parse_expression_internal(prs);
				token_expect_and_consume(prs, TOK_RPAREN, wait_rightbr_in_primary);
				while (old_operators_size < operators_size(prs))
				{
					binary_operation(prs, operators_pop(prs));
				}
			}
		}
		break;

		default:
			if (prs->token >= STANDARD_FUNC_START && prs->token <= STANDARD_FUNC_END)
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
	token_expect_and_consume(prs, TOK_IDENTIFIER, after_dot_must_be_ident);

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
 *	@param	function_id	Index of function TOK_IDENTIFIER in TOK_IDENTIFIERs table
 */
void parse_function_call(parser *const prs, const size_t function_id)
{
	const int old_in_assignment = prs->flag_in_assignment;
	const size_t function_mode = (size_t)stack_pop(&prs->anonymous);

	if (!mode_is_function(prs->sx, function_mode))
	{
		parser_error(prs, call_not_from_function);
		token_skip_until(prs, TOK_RPAREN | TOK_SEMICOLON);
		return;
	}

	const size_t expected_args = (size_t)mode_get(prs->sx, function_mode + 2);
	size_t ref_arg_mode = function_mode + 3;
	size_t actual_args = 0;

	to_tree(prs, ND_CALL1);
	node_add_arg(&prs->nd, expected_args);

	node nd_call;
	node_copy(&nd_call, &prs->nd);

	if (!token_try_consume(prs, TOK_RPAREN))
	{
		do
		{
			node_copy(&prs->nd, &nd_call);
			const item_t expected_arg_mode = mode_get(prs->sx, ref_arg_mode);

			if (mode_is_function(prs->sx, expected_arg_mode))
			{
				if (prs->token != TOK_IDENTIFIER)
				{
					parser_error(prs, act_param_not_ident);
					token_skip_until(prs, TOK_COMMA | TOK_RPAREN | TOK_SEMICOLON);
					continue;
				}

				const size_t id = parse_identifier(prs);
				if (ident_get_mode(prs->sx, id) != expected_arg_mode)
				{
					parser_error(prs, diff_formal_param_type_and_actual);
					token_skip_until(prs, TOK_COMMA | TOK_RPAREN | TOK_SEMICOLON);
					continue;
				}

				const item_t displ = ident_get_displ(prs->sx, id);
				node_set_type(&prs->nd, displ < 0 ? ND_IDENTTOVAL : ND_CONST);
				node_set_arg(&prs->nd, 0, llabs(displ));
				to_tree(prs, ND_EXPRESSION_END);
			}
			else if (mode_is_array(prs->sx, expected_arg_mode) && prs->token == TOK_LBRACE)
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
		} while (token_try_consume(prs, TOK_COMMA));

		token_expect_and_consume(prs, TOK_RPAREN, wrong_number_of_params);
	}

	if (expected_args != actual_args)
	{
		parser_error(prs, wrong_number_of_params);
	}

	prs->flag_in_assignment = old_in_assignment;
	node nd_call2 = node_add_child(&nd_call, ND_CALL2);
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

	if (token_try_consume(prs, TOK_LPAREN))
	{
		was_func = 1;
		parse_function_call(prs, last_id);
	}

	while (prs->token == TOK_LSQUARE || prs->token == TOK_ARROW || prs->token == TOK_PERIOD)
	{
		while (token_try_consume(prs, TOK_LSQUARE))
		{
			if (was_func)
			{
				parser_error(prs, slice_from_func);
			}

			if (prs->last_type == VARIABLE)
			{
				node_set_type(&prs->nd, ND_SLICEIDENT);
				node_set_arg(&prs->nd, 0, prs->operand_displ);
			}
			else
			{
				to_tree(prs, ND_SLICE);
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

			token_expect_and_consume(prs, TOK_RSQUARE, no_rightsqbr_in_slice);
			operands_push(prs, ADDRESS, elem_type);
		}

		while (prs->token == TOK_ARROW)
		{
			if (prs->last_type == VARIABLE)
			{
				node_set_type(&prs->nd, ND_IDENTTOVAL);
			}

			to_tree(prs, ND_SELECT);

			// Здесь мы ожидаем указатель, снимаем указатель со стека и кладем саму структуру
			const item_t type = stack_pop(&prs->anonymous);
			if (!(mode_is_pointer(prs->sx, type) && mode_is_struct(prs->sx, mode_get(prs->sx, (size_t)type + 1))))
			{
				parser_error(prs, get_field_not_from_struct_pointer);
			}

			operands_push(prs, ADDRESS, mode_get(prs->sx, (size_t)type + 1));
			prs->operand_displ = find_field(prs);
			while (prs->token == TOK_PERIOD)
			{
				prs->operand_displ += find_field(prs);
			}

			node_add_arg(&prs->nd, prs->operand_displ);

			// find_field вернула тип результата через стек, проверим его и вернем обратно
			const item_t field_type = stack_pop(&prs->anonymous);
			if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
			{
				to_tree(prs, ND_ADDRTOVAL);
			}

			operands_push(prs, ADDRESS, field_type);
		}

		if (prs->token == TOK_PERIOD)
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
				while (prs->token == TOK_PERIOD)
				{
					prs->operand_displ += find_field(prs);
				}

				const item_t field_type = stack_pop(&prs->anonymous);
				to_tree(prs, ND_COPYST);
				node_add_arg(&prs->nd, prs->operand_displ);
				node_add_arg(&prs->nd, (item_t)size_of(prs->sx, field_type));
				node_add_arg(&prs->nd, (item_t)length);
				operands_push(prs, VALUE, field_type);
			}
			else if (peek == VARIABLE)
			{
				const item_t sign = prs->operand_displ < 0 ? -1 : 1;
				operands_push(prs, VARIABLE, type);
				while (prs->token == TOK_PERIOD)
				{
					prs->operand_displ += sign * find_field(prs);
				}

				node_set_arg(&prs->nd, 0, prs->operand_displ);
			}
			else //if (peek == address)
			{
				to_tree(prs, ND_SELECT);
				operands_push(prs, VARIABLE, type);
				prs->operand_displ = 0;
				while (prs->token == TOK_PERIOD)
				{
					prs->operand_displ += find_field(prs);
				}

				node_add_arg(&prs->nd, prs->operand_displ);
				// find_field вернула тип результата через стек, проверим его и вернем обратно
				const item_t field_type = stack_pop(&prs->anonymous);
				if (mode_is_array(prs->sx, field_type) || mode_is_pointer(prs->sx, field_type))
				{
					to_tree(prs, ND_ADDRTOVAL);
				}

				operands_push(prs, ADDRESS, field_type);
			}
		}
	}

	if (prs->token == TOK_PLUSPLUS || prs->token == TOK_MINUSMINUS)
	{
		node_t operator = prs->token == TOK_PLUSPLUS ? ND_POSTINC : ND_POSTDEC;
		token_consume(prs);

		int is_variable = 0;
		if (prs->last_type == ADDRESS)
		{
			operator = node_at_operator(operator);
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
		case TOK_PLUSPLUS:
		case TOK_MINUSMINUS:
		{
			token_consume(prs);
			parse_unary_expression(prs);
			node_t node_type = token_to_node(operator);

			int is_variable = 0;
			if (prs->last_type == ADDRESS)
			{
				node_type = node_at_operator(node_type);
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
			float_operation(prs, type, node_type);

			if (is_variable)
			{
				node_add_arg(&prs->nd, ident_get_displ(prs->sx, prs->last_id));
			}
		}
		break;

		case TOK_EXCLAIM:
		case TOK_TILDE:
		case TOK_PLUS:
		case TOK_MINUS:
		case TOK_AMP:
		case TOK_STAR:
		{
			token_consume(prs);
			parse_unary_expression(prs);
			switch (operator)
			{
				case TOK_AMP:
				{
					if (prs->last_type == VALUE)
					{
						parser_error(prs, wrong_addr);
					}

					if (prs->last_type == VARIABLE)
					{
						node_set_type(&prs->nd, ND_IDENTTOADDR);
					}

					operands_push(prs, VALUE, to_modetab(prs, mode_pointer, stack_pop(&prs->anonymous)));
				}
				break;

				case TOK_STAR:
				{
					if (prs->last_type == VARIABLE)
					{
						node_set_type(&prs->nd, ND_IDENTTOVAL);
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
					if (operator == TOK_MINUS)
					{
						if (node_get_type(&prs->nd) == ND_CONST)
						{
							node_set_arg(&prs->nd, 0, -node_get_arg(&prs->nd, 0));
						}
						else if (node_get_type(&prs->nd) == ND_CONSTD)
						{
							node_set_double(&prs->nd, 0, -node_get_double(&prs->nd, 0));
						}
						else
						{
							const item_t type = stack_pop(&prs->anonymous);
							float_operation(prs, type, ND_UNMINUS);
							operands_push(prs, VALUE, type);
						}
					}
					else
					{
						if (operator != TOK_PLUS)
						{
							to_tree(prs, token_to_node(operator));
						}

						const item_t type = stack_pop(&prs->anonymous);
						if ((operator == TOK_TILDE || operator == TOK_EXCLAIM) && mode_is_float(type))
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
		case TOK_PIPEPIPE:			// '||'
			return 1;

		case TOK_AMPAMP:			// '&&'
			return 2;

		case TOK_PIPE:				// '|'
			return 3;

		case TOK_CARET:				// '^'
			return 4;

		case TOK_AMP:				// '&'
			return 5;

		case TOK_EQUALEQUAL:		// '=='
		case TOK_EXCLAIMEQUAL:		// '!='
			return 6;

		case TOK_LESS:				// '<'
		case TOK_GREATER:			// '<'
		case TOK_LESSEQUAL:			// '<='
		case TOK_GREATEREQUAL:		// '>='
			return 7;

		case TOK_LESSLESS:			// '<<'
		case TOK_GREATERGREATER:	// '>>'
			return 8;

		case TOK_PLUS:				// '+'
		case TOK_MINUS:				// '-'
			return 9;

		case TOK_STAR:				// '*'
		case TOK_SLASH:				// '/'
		case TOK_PERCENT:			// '%'
			return 10;

		default:
			return 0;
	}
}

int is_int_assignment_operator(const token_t operator)
{
	switch (operator)
	{
		case TOK_PERCENTEQUAL:			// '%='
		case TOK_LESSLESSEQUAL:			// '<<='
		case TOK_GREATERGREATEREQUAL:	// '>>='
		case TOK_AMPEQUAL:				// '&='
		case TOK_PIPEEQUAL:				// '|='
		case TOK_CARETEQUAL:			// '^='
			return 1;

		default:
			return 0;
	}
}


int is_assignment_operator(const token_t operator)
{
	switch (operator)
	{
		case TOK_EQUAL:					// '='
		case TOK_STAREQUAL:				// '*='
		case TOK_SLASHEQUAL:			// '/='
		case TOK_PLUSEQUAL:				// '+='
		case TOK_MINUSEQUAL:			// '-='
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
			to_tree(prs, priority == 1 ? ND_ADLOGOR : ND_ADLOGAND);
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

	if (prs->token == TOK_QUESTION)
	{
		item_t global_type = 0;
		item_t addr_if = ND_EXPRESSION_END;

		while (token_try_consume(prs, TOK_QUESTION))
		{
			to_value(prs);
			if (!mode_is_int(stack_pop(&prs->anonymous)))
			{
				parser_error(prs, float_in_condition);
			}

			to_tree(prs, ND_CONDITIONAL);
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
				if (addr_if == ND_EXPRESSION_END)
				{
					node_set_type(&prs->nd, ND_NULL);
				}

				const item_t old_addr_if = addr_if;
				addr_if = (item_t)node_save(&prs->nd);
				to_tree(prs, old_addr_if);
			}

			token_expect_and_consume(prs, TOK_COLON, no_colon_in_cond_expr);
			node_copy(&prs->nd, &nd_condexpr);
			parse_unary_expression(prs);
			parse_subexpression(prs); // logORexpr();	else or elif
		}

		to_value(prs);
		// Это особый случай, когда после ND_EXPRESSION_END мы храним дополнительную информацию
		prs->nd = node_add_child(&prs->nd, ND_EXPRESSION_END);

		if (mode_is_float(stack_pop(&prs->anonymous)))
		{
			global_type = mode_float;
		}
		else
		{
			if (addr_if == ND_EXPRESSION_END)
			{
				node_set_type(&prs->nd, ND_NULL);
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

		while (addr_if != ND_EXPRESSION_END)
		{
			node node_addr = node_load(&prs->sx->tree, (size_t)addr_if);
			node_set_type(&node_addr, mode_is_float(global_type) ? ND_WIDEN : ND_NULL);

			node_addr = node_get_child(&node_addr, 0);
			addr_if = node_get_type(&node_addr);
			node_set_type(&node_addr, ND_EXPRESSION_END);
		}
	}
}

void assignment_to_void(parser *const prs)
{
	node_set_type(&prs->nd, node_void_operator((node_t)node_get_type(&prs->nd)));
}

void parse_assignment_expression_internal(parser *const prs)
{
	if (prs->token == TOK_LBRACE)
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

		token_t operator = prs->token;
		node_t node_type = token_to_node(operator);
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

			if (operator != TOK_EQUAL) // в структуру можно присваивать только с помощью =
			{
				parser_error(prs, wrong_struct_ass);
			}

			if (right_type == VALUE)
			{
				node_type = left_type == VARIABLE ? ND_COPY0STASSIGN : ND_COPY1STASSIGN;
			}
			else
			{
				node_type = left_type == VARIABLE
					? right_type == VARIABLE
						? ND_COPY00 : ND_COPY01
					: right_type == VARIABLE
						? ND_COPY10 : ND_COPY11;
			}

			to_tree(prs, node_type);
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
			if (mode_is_pointer(prs->sx, left_mode) && operator != TOK_EQUAL)
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
				to_tree(prs, ND_WIDEN);
				result_mode = mode_float;
			}
			if (mode_is_pointer(prs->sx, left_mode) && mode_is_pointer(prs->sx, right_mode) && left_mode != right_mode)
			{
				// проверка нужна только для указателей
				parser_error(prs, type_missmatch);
			}

			if (left_type == ADDRESS)
			{
				node_type = node_at_operator(node_type);
			}
			float_operation(prs, result_mode, node_type);
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
	while (token_try_consume(prs, TOK_COMMA))
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
	to_tree(prs, ND_EXPRESSION_END);
	return stack_pop(&prs->anonymous);
}

item_t parse_assignment_expression(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_assignment_expression_internal(prs);
	to_value(prs);
	to_tree(prs, ND_EXPRESSION_END);
	return stack_pop(&prs->anonymous);
}

item_t parse_parenthesized_expression(parser *const prs, node *const parent)
{
	token_expect_and_consume(prs, TOK_LPAREN, cond_must_be_in_brkts);
	const item_t condition_type = parse_condition(prs, parent);
	token_expect_and_consume(prs, TOK_RPAREN, cond_must_be_in_brkts);
	return condition_type;
}

item_t parse_constant_expression(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_unary_expression(prs);
	parse_conditional_expression(prs);
	to_value(prs);
	to_tree(prs, ND_EXPRESSION_END);
	return stack_pop(&prs->anonymous);
}

item_t parse_condition(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	parse_expression_internal(prs);
	to_value(prs);
	to_tree(prs, ND_EXPRESSION_END);
	return stack_pop(&prs->anonymous);
}

void parse_string_literal(parser *const prs, node *const parent)
{
	node_copy(&prs->nd, parent);
	to_tree(prs, ND_STRING);
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
	// Сейчас последней нодой в поддереве выражения является ND_EXPRESSION_END, просто меняем ее тип
	node_set_type(&prs->nd, ND_WIDEN);
	to_tree(prs, ND_EXPRESSION_END);
}
