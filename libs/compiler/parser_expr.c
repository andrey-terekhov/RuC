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


/** Binary/ternary operator precedence levels */
typedef enum PRECEDENCE
{
	PREC_UNKWOWN,			/**< Not binary operator */
	PREC_COMMA,				/**< Comma operator precedence */
	PREC_ASSIGNMENT,		/**< Assignment operator precedence */
	PREC_CONDITIONAL,		/**< Conditional operator precedence */
	PREC_LOGICAL_OR,		/**< Logical OR operator precedence */
	PREC_LOGICAL_AND,		/**< Logical AND operator precedence */
	PREC_OR,				/**< Bitwise OR operator precedence */
	PREC_XOR,				/**< Bitwise XOR operator precedence */
	PREC_AND,				/**< Bitwise AND operator precedence */
	PREC_EQUALITY,			/**< Equality operators precedence */
	PREC_RELATIONAL,		/**< Relational operators precedence */
	PREC_SHIFT,				/**< Shift operators precedence */
	PREC_ADDITIVE,			/**< Additive operators precedence */
	PREC_MULTIPLICATIVE,	/**< Multiplicative operators precedence */
} precedence_t;


/** Return the precedence of the specified binary/ternary operator token */
static precedence_t get_operator_precedence(const token_t token)
{
	switch (token)
	{
		case TK_COMMA:
			return PREC_COMMA;

		case TK_EQUAL:
		case TK_STAR_EQUAL:
		case TK_SLASH_EQUAL:
		case TK_PERCENT_EQUAL:
		case TK_PLUS_EQUAL:
		case TK_MINUS_EQUAL:
		case TK_LESS_LESS_EQUAL:
		case TK_GREATER_GREATER_EQUAL:
		case TK_AMP_EQUAL:
		case TK_CARET_EQUAL:
		case TK_PIPE_EQUAL:
			return PREC_ASSIGNMENT;

		case TK_QUESTION:
			return PREC_CONDITIONAL;

		case TK_PIPE_PIPE:
			return PREC_LOGICAL_OR;

		case TK_AMP_AMP:
			return PREC_LOGICAL_AND;

		case TK_PIPE:
			return PREC_OR;

		case TK_CARET:
			return PREC_XOR;

		case TK_AMP:
			return PREC_AND;

		case TK_EQUAL_EQUAL:
		case TK_EXCLAIM_EQUAL:
			return PREC_EQUALITY;

		case TK_GREATER_EQUAL:
		case TK_LESS_EQUAL:
		case TK_GREATER:
		case TK_LESS:
			return PREC_RELATIONAL;

		case TK_LESS_LESS:
		case TK_GREATER_GREATER:
			return PREC_SHIFT;

		case TK_PLUS:
		case TK_MINUS:
			return PREC_ADDITIVE;

		case TK_STAR:
		case TK_SLASH:
		case TK_PERCENT:
			return PREC_MULTIPLICATIVE;

		default:
			return PREC_UNKWOWN;
	}
}


/**
 *	Parse primary expression
 *
 *	primary-expression:
 *		identifier
 *		constant
 *		string-literal
 *		'(' expression ')'
 *
 *	@param	prs			Parser
 *
 *	@return	Primary expression node
 */
static node parse_primary_expression(parser *const prs)
{
	switch (prs->token)
	{
		case TK_IDENTIFIER:
		{
			const size_t name = prs->lxr.repr;
			const location loc = token_consume(prs);

			return build_identifier_expression(prs->sx, name, loc);
		}

		case TK_CHAR_CONST:
		case TK_INT_CONST:
		{
			const int value = prs->lxr.num;
			const location loc = token_consume(prs);

			return build_integer_literal_expression(prs->sx, value, loc);
		}

		case TK_FLOAT_CONST:
		{
			const double value = prs->lxr.num_double;
			const location loc = token_consume(prs);

			return build_floating_literal_expression(prs->sx, value, loc);
		}

		case TK_STRING:
		{
			const vector *const value = &prs->lxr.lexstr;
			const location loc = token_consume(prs);

			return build_string_literal_expression(prs->sx, value, loc);
		}

		case TK_L_PAREN:
		{
			const location l_loc = token_consume(prs);
			const node nd_result = parse_expression(prs);

			if (!token_try_consume(prs, TK_R_PAREN))
			{
				parser_error(prs, expected_r_paren, l_loc);
				return build_broken_expression();
			}

			return nd_result;
		}

		default:
			parser_error(prs, expected_expression);
			return build_broken_expression();
	}
}

/**
 *	Parse expression list
 *
 *	expression-list:
 *		initializer
 *		expression-list ',' initializer
 *
 *	@param	prs			Parser
 *	@param	type		Target type
 *
 *	@return	Expression nodes vector
 */
static node_vector parse_expression_list(parser *const prs, const item_t type)
{
	unsigned i = 0;
	node_vector result = node_vector_create();

	do
	{
		item_t item_type = TYPE_UNDEFINED;
		if (type_is_structure(prs->sx, type))
		{
			item_type = type_get(prs->sx, (size_t)type + + 3 + 2 * i);
		}
		else if (type_is_function(prs->sx, type))
		{
			item_type = type_get(prs->sx, (size_t)type + 3 + i);
		}
		else if (type_is_array(prs->sx, type))
		{
			item_type = type_get(prs->sx, (size_t)type + 1);
		}

		const node initializer = parse_initializer(prs, item_type);
		node_vector_add(&result, &initializer);
		i++;
	} while (token_try_consume(prs, TK_COMMA));

	return result;
}

/**
 *	Parse postfix expression
 *
 *	postfix-expression:
 *		primary-expression
 *		postfix-expression '[' expression ']'
 *		postfix-expression '(' expression-list[opt] ')'
 *		postfix-expression '.' identifier
 *		postfix-expression '->' identifier
 *		postfix-expression '++'
 *		postfix-expression '--'
 *
 *	@param	prs			Parser
 *
 *	@return	Postfix expression node
 */
static node parse_postfix_expression(parser *const prs)
{
	node nd_operand = parse_primary_expression(prs);

	while (true)
	{
		switch (prs->token)
		{
			default:
				return nd_operand;

			case TK_L_SQUARE:
			{
				const location l_loc = token_consume(prs);
				const node index = parse_expression(prs);

				if (prs->token == TK_R_SQUARE)
				{
					const location r_loc = token_consume(prs);
					nd_operand = build_subscript_expression(prs->sx, &nd_operand, &index, l_loc, r_loc);
				}
				else
				{
					parser_error(prs, expected_r_square, l_loc);
					token_skip_until(prs, TK_R_SQUARE | TK_SEMICOLON);
					token_try_consume(prs, TK_R_SQUARE);
					nd_operand = build_broken_expression();
				}

				continue;
			}

			case TK_L_PAREN:
			{
				const location l_loc = token_consume(prs);

				if (prs->token == TK_R_PAREN)
				{
					const location r_loc = token_consume(prs);
					nd_operand = build_call_expression(prs->sx, &nd_operand, NULL, l_loc, r_loc);

					continue;
				}

				node_vector args = parse_expression_list(prs, expression_get_type(&nd_operand));
				if (prs->token == TK_R_PAREN)
				{
					const location r_loc = token_consume(prs);
					nd_operand = build_call_expression(prs->sx, &nd_operand, &args, l_loc, r_loc);
				}
				else
				{
					parser_error(prs, expected_r_paren, l_loc);
					token_skip_until(prs, TK_R_PAREN | TK_SEMICOLON);
					token_try_consume(prs, TK_R_PAREN);
					nd_operand = build_broken_expression();
				}

				node_vector_clear(&args);
				continue;
			}

			case TK_PERIOD:
			case TK_ARROW:
			{
				const bool is_arrow = prs->token == TK_ARROW;
				const location op_loc = token_consume(prs);

				if (prs->token == TK_IDENTIFIER)
				{
					const size_t name = prs->lxr.repr;
					const location id_loc = token_consume(prs);

					nd_operand = build_member_expression(prs->sx, &nd_operand, name, is_arrow, op_loc, id_loc);
				}
				else
				{
					parser_error(prs, expected_identifier);
					nd_operand = build_broken_expression();
				}

				continue;
			}

			case TK_PLUS_PLUS:
			{
				const location op_loc = token_consume(prs);
				nd_operand = build_unary_expression(prs->sx, &nd_operand, UN_POSTINC, op_loc);
				continue;
			}

			case TK_MINUS_MINUS:
			{
				const location op_loc = token_consume(prs);
				nd_operand = build_unary_expression(prs->sx, &nd_operand, UN_POSTDEC, op_loc);
				continue;
			}
		}
	}
}

/**
 *	Parse unary expression
 *
 *	unary-expression:
 *		postfix-expression
 *		'++' unary-expression
 *		'--' unary-expression
 *		unary-operator unary-expression
 *
 *	unary-operator: one of
 *		'&', '*', '+', '-', '~', '!', 'abs'
 *
 *	@param	prs			Parser
 *
 *	@return	Unary expression node
 */
static node parse_unary_expression(parser *const prs)
{
	switch (prs->token)
	{
		default:
			return parse_postfix_expression(prs);

		case TK_PLUS_PLUS:
		case TK_MINUS_MINUS:
		case TK_AMP:
		case TK_STAR:
		case TK_PLUS:
		case TK_MINUS:
		case TK_TILDE:
		case TK_EXCLAIM:
		case TK_ABS:
		{
			const unary_t operator = token_to_unary(prs->token);
			const location op_loc = token_consume(prs);
			const node nd_operand = parse_unary_expression(prs);

			return build_unary_expression(prs->sx, &nd_operand, operator, op_loc);
		}

		case TK_UPB:
		{
			token_consume(prs);
			token_expect_and_consume(prs, TK_L_PAREN, no_leftbr_in_stand_func);
			const node nd_dimension = parse_assignment_expression(prs);
			token_expect_and_consume(prs, TK_COMMA, no_comma_in_act_params_stanfunc);
			const node nd_array = parse_assignment_expression(prs);
			token_expect_and_consume(prs, TK_R_PAREN, no_rightbr_in_stand_func);
			return build_upb_expression(prs->sx, &nd_dimension, &nd_array);
		}
	}
}

/**
 *	Parse right hand side of binary expression
 *
 *	@param	prs			Parser
 *	@param	LHS			Start of a binary expression
 *	@param	min_prec	Minimal precedence level
 *
 *	@return Binary expression node
 */
static node parse_RHS_of_binary_expression(parser *const prs, node *const LHS, const precedence_t min_prec)
{
	precedence_t next_token_prec = get_operator_precedence(prs->token);
	while (next_token_prec >= min_prec)
	{
		const token_t op_token = prs->token;
		location op_loc = token_consume(prs);

		bool is_binary = true;
		node middle = build_broken_expression();
		if (next_token_prec == PREC_CONDITIONAL)
		{
			is_binary = false;
			middle = parse_expression(prs);

			if (prs->token != TK_COLON)
			{
				parser_error(prs, expected_colon_in_conditional, op_loc);
			}

			op_loc = token_consume(prs);
		}

		node RHS = (prs->token == TK_L_BRACE)
			? parse_initializer(prs, expression_get_type(LHS))
			: parse_unary_expression(prs);

		const precedence_t this_prec = next_token_prec;
		next_token_prec = get_operator_precedence(prs->token);

		const bool is_right_associative = this_prec == PREC_CONDITIONAL || this_prec == PREC_ASSIGNMENT;
		if (this_prec < next_token_prec || (this_prec == next_token_prec && is_right_associative))
		{
			RHS = parse_RHS_of_binary_expression(prs, &RHS, (this_prec + !is_right_associative));
			next_token_prec = get_operator_precedence(prs->token);
		}

		if (is_binary)
		{
			const binary_t op_kind = token_to_binary(op_token);
			*LHS = build_binary_expression(prs->sx, LHS, &RHS, op_kind, op_loc);
		}
		else
		{
			*LHS = build_ternary_expression(prs->sx, LHS, &middle, &RHS, op_loc);
		}
	}

	return *LHS;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


node parse_assignment_expression(parser *const prs)
{
	node LHS = parse_unary_expression(prs);
	return parse_RHS_of_binary_expression(prs, &LHS, PREC_ASSIGNMENT);
}

node parse_expression(parser *const prs)
{
	node LHS = parse_assignment_expression(prs);
	return parse_RHS_of_binary_expression(prs, &LHS, PREC_COMMA);
}

node parse_constant_expression(parser *const prs)
{
	node LHS = parse_unary_expression(prs);
	return parse_RHS_of_binary_expression(prs, &LHS, PREC_CONDITIONAL);
}

node parse_initializer(parser *const prs, const item_t type)
{
	if (prs->token == TK_L_BRACE)
	{
		const location l_loc = token_consume(prs);

		if (token_try_consume(prs, TK_R_BRACE))
		{
			token_consume(prs);
			parser_error(prs, empty_init);
			return build_broken_expression();
		}

		node_vector inits = parse_expression_list(prs, type);
		node nd_result;
		if (prs->token == TK_R_BRACE)
		{
			const location r_loc = token_consume(prs);
			nd_result = build_init_list_expression(prs->sx, &inits, type, l_loc, r_loc);
		}
		else
		{
			parser_error(prs, expected_r_brace, l_loc);
			token_skip_until(prs, TK_R_BRACE | TK_SEMICOLON);
			token_try_consume(prs, TK_R_BRACE);
			nd_result = build_broken_expression();
		}

		node_vector_clear(&inits);
		return nd_result;
	}

	return parse_assignment_expression(prs);
}
