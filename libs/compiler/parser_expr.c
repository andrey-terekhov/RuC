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

static node create_node(parser *const prs, operation_t type)
{
	return node_add_child(&prs->nd, type);
}

static void node_set_child(node *const parent, node *const child)
{
	node temp = node_add_child(parent, OP_NOP);
	node_swap(child, &temp);
	node_remove(&temp);
}

/** Return valid expression from AST node */
static expression expr(const node expr_node, const location_t location)
{
	return (expression){ .is_valid = true, .location = location, .nd = expr_node };
}

/** Return invalid expression */
static expression expr_broken(void)
{
	return (expression){ .is_valid = false };
}

/**
 *	Make unary expression
 *
 *	@param	prs			Parser
 *	@param	operand		Operand of unary operator
 *	@param	op_kind		Operator kind
 *	@param	loc			Operator location
 *
 *	@return	Unary expression
 */
static expression make_unary_expression(parser *const prs, expression operand, unary_t op_kind, location_t loc)
{
	if (!operand.is_valid)
	{
	   return expr_broken();
	}

	const item_t operand_type = node_get_arg(&operand.nd, 0);

	item_t result_type = 0;
	category_t category = RVALUE;
	bool is_prefix = true;

	switch (op_kind)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
			is_prefix = false;
			// fallthrough;
		case UN_PREINC:
		case UN_PREDEC:
		{
			if (!type_is_arithmetic(operand_type))
			{
				semantics_error(prs, loc, typecheck_illegal_increment, op_kind);
				return expr_broken();
			}

			if (node_get_arg(&operand.nd, 1) != LVALUE)
			{
				semantics_error(prs, loc, typecheck_expression_not_lvalue);
				return expr_broken();
			}

			result_type = operand_type;
			break;
		}

		case UN_ADDRESS:
		{
			if (node_get_arg(&operand.nd, 1) != LVALUE)
			{
				semantics_error(prs, loc, typecheck_invalid_lvalue_addrof);
				return expr_broken();
			}

			result_type = (item_t)type_add(prs->sx, (item_t[]){ TYPE_POINTER, operand_type }, 2);
			break;
		}

		case UN_INDIRECTION:
		{
			if (!type_is_pointer(prs->sx, operand_type))
			{
				semantics_error(prs, loc, typecheck_indirection_requires_pointer);
				return expr_broken();
			}

			result_type = type_get(prs->sx, (size_t)operand_type + 1);
			category = LVALUE;
			break;
		}

		case UN_ABS:
		case UN_PLUS:
		case UN_MINUS:
		{
			if (!type_is_arithmetic(operand_type))
			{
				semantics_error(prs, loc, typecheck_unary_expr, operand_type);
				return expr_broken();
			}

			result_type = operand_type;
			break;
		}

		case UN_NOT:
		{
			if (!type_is_integer(operand_type))
			{
				semantics_error(prs, loc, typecheck_unary_expr, operand_type);
				return expr_broken();
			}

			result_type = TYPE_INTEGER;
			break;
		}

		case UN_LOGNOT:
		{
			if (!type_is_scalar(prs->sx, operand_type))
			{
				semantics_error(prs, loc, typecheck_unary_expr, operand_type);
				return expr_broken();
			}

			result_type = TYPE_INTEGER;
			break;
		}
	}

	node unary_node = create_node(prs, OP_UNARY);
	node_add_arg(&unary_node, result_type);		// Тип значения
	node_add_arg(&unary_node, category);		// Категория значения
	node_add_arg(&unary_node, op_kind);			// Тип унарного оператора
	node_set_child(&unary_node, &operand.nd);	// Выражение-операнд

	location_t location = is_prefix
						? (location_t){ loc.begin, operand.location.end }
						: (location_t){ operand.location.begin, loc.end };

	return expr(unary_node, location);
}

/**
 *	Make binary expression
 *
 *	@param	prs					Parser
 *	@param	left				Left operand
 *	@param	right				Right operand
 *	@param	operator_kind		Operator kind
 *	@param	operator_location	Operator location
 *
 *	@return	Binary expression
 */
static expression make_binary_expression(parser *const prs, expression left, expression right
	, const binary_t operator_kind, const location_t operator_location)
{
	if (!left.is_valid || !right.is_valid)
	{
		return expr_broken();
	}

	const item_t left_type = node_get_arg(&left.nd, 0);
	const item_t right_type = node_get_arg(&right.nd, 0);

	if (operation_is_assignment(operator_kind)
		&& ((node_get_arg(&left.nd, 1) != LVALUE)
			|| (type_is_floating(right_type) && type_is_integer(left_type))))
	{
		semantics_error(prs, operator_location, unassignable);
		return expr_broken();
	}

	item_t result_type;
	if (operator_kind == BIN_COMMA)
	{
		result_type = right_type;
	}
	else if (operator_kind == BIN_ASSIGN)
	{
		// Особый случай, так как тут могут быть операции с агрегатными типами

		// Несовпадение типов может быть только в случае, когда слева floating, а справа integer
		if (left_type != right_type && !(type_is_floating(left_type) && type_is_integer(right_type)))
		{
			semantics_error(prs, operator_location, typecheck_convert_incompatible);
			return expr_broken();
		}

		result_type = left_type;
	}
	else
	{
		if (!type_is_arithmetic(left_type) || !type_is_arithmetic(right_type))
		{
			semantics_error(prs, operator_location, typecheck_binary_expr);
			return expr_broken();
		}

		if (operation_is_assignment(operator_kind) && (node_get_arg(&left.nd, 1) != LVALUE))
		{
			semantics_error(prs, operator_location, unassignable);
			return expr_broken();
		}

		switch (operator_kind)
		{
			case BIN_REM:
			case BIN_LOG_OR:
			case BIN_LOG_AND:
			case BIN_OR:
			case BIN_AND:
			case BIN_SHL:
			case BIN_SHR:
			case BIN_REM_ASSIGN:
			case BIN_OR_ASSIGN:
			case BIN_AND_ASSIGN:
			case BIN_SHL_ASSIGN:
			case BIN_SHR_ASSIGN:
			{
				if (!type_is_integer(left_type) || !type_is_integer(right_type))
				{
					semantics_error(prs, operator_location, int_op_for_float);
					return expr_broken();
				}

				result_type = TYPE_INTEGER;
				break;
			}

			default:
				result_type = type_is_floating(left_type) ? TYPE_FLOATING : right_type;
				break;
		}
	}

	node binary_node = create_node(prs, OP_BINARY);
	node_add_arg(&binary_node, result_type);		// Тип значения
	node_add_arg(&binary_node, RVALUE);				// Категория значения
	node_add_arg(&binary_node, operator_kind);		// Вид оператора
	node_set_child(&binary_node, &left.nd);			// Второй операнд
	node_set_child(&binary_node, &right.nd);		// Третий операнд

	return expr(binary_node, (location_t){ left.location.begin, right.location.end });
}

/**
 *	Make ternary expression
 *
 *	@param	prs					Parser
 *	@param	left				First operand
 *	@param	middle				Second operand
 *	@param	right				Third operand
 *	@param	colon_location		Operator location
 *
 *	@return	Ternary expression
 */
static expression make_ternary_expression(parser *const prs, expression left, expression middle, expression right
	, location_t colon_location)
{
	if (!left.is_valid || !middle.is_valid || !right.is_valid)
	{
		return expr_broken();
	}

	const item_t left_type = node_get_arg(&left.nd, 0);
	const item_t middle_type = node_get_arg(&middle.nd, 0);
	const item_t right_type = node_get_arg(&right.nd, 0);

	if (!type_is_scalar(prs->sx, left_type))
	{
		semantics_error(prs, left.location, typecheck_statement_requires_scalar);
		return expr_broken();
	}

	item_t result_type = middle_type;

	if ((type_is_floating(middle_type) && type_is_integer(right_type))
		|| (type_is_integer(middle_type) && type_is_floating(right_type)))
	{
		result_type = TYPE_FLOATING;
	}
	else if (middle_type != right_type)
	{
		semantics_error(prs, colon_location, typecheck_cond_incompatible_operands);
		return expr_broken();
	}

	node ternary_node = create_node(prs, OP_TERNARY);
	node_add_arg(&ternary_node, result_type);		// Тип значения
	node_add_arg(&ternary_node, RVALUE);			// Категория значения
	node_set_child(&ternary_node, &left.nd);		// Первый операнд
	node_set_child(&ternary_node, &middle.nd);		// Второй операнд
	node_set_child(&ternary_node, &right.nd);		// Третий операнд

	return expr(ternary_node, (location_t){ left.location.begin, right.location.end });
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
 *	@return	Primary expression
 */
static expression parse_primary_expression(parser *const prs)
{
	switch (prs->token)
	{
		case TK_IDENTIFIER:
		{
			const size_t representation = prs->lxr->repr;
			const item_t identifier = repr_get_reference(prs->sx, representation);
			const location_t location = token_consume(prs);

			if (identifier == ITEM_MAX)
			{
				semantics_error(prs, location, undeclared_var_use, repr_get_name(prs->sx, representation));
				return expr_broken();
			}

			const item_t type = ident_get_type(prs->sx, (size_t)identifier);
			const category_t category = type_is_function(prs->sx, type) ? RVALUE : LVALUE;

			node identifier_node = create_node(prs, OP_IDENTIFIER);
			node_add_arg(&identifier_node, type);			// Тип значения идентификатора
			node_add_arg(&identifier_node, category);		// Категория значения идентификатора
			node_add_arg(&identifier_node, identifier);		// Индекс в таблице идентификаторов

			return expr(identifier_node, location);
		}

		case TK_CHAR_CONST:
		case TK_INT_CONST:
		{
			const item_t value = prs->lxr->num;
			const location_t location = token_consume(prs);

			node constant_node = create_node(prs, OP_CONSTANT);
			node_add_arg(&constant_node, TYPE_INTEGER);		// Тип значения константы
			node_add_arg(&constant_node, RVALUE);			// Категория значения константы
			node_add_arg(&constant_node, value);			// Значение константы

			return expr(constant_node, location);
		}

		case TK_FLOAT_CONST:
		{
			item_t value;
			memcpy(&value, &prs->lxr->num_double, sizeof(double));
			const location_t location = token_consume(prs);

			node constant_node = create_node(prs, OP_CONSTANT);
			node_add_arg(&constant_node, TYPE_FLOATING);	// Тип значения константы
			node_add_arg(&constant_node, RVALUE);			// Категория значения константы
			node_add_arg(&constant_node, value);			// Значение константы

			return expr(constant_node, location);
		}

		case TK_STRING:
		{
			const char32_t* string = prs->lxr->lexstr;
			const size_t length = (size_t)prs->lxr->num;
			const item_t type = type_array(prs->sx, TYPE_INTEGER);
			const location_t location = token_consume(prs);

			node string_node = create_node(prs, OP_STRING);
			node_add_arg(&string_node, type);				// Тип строки
			node_add_arg(&string_node, LVALUE);				// Категория значения строки
			for (size_t i = 0; i < length; i++)
			{
				node_add_arg(&string_node, string[i]);		// i-ый символ строки
			}

			return expr(string_node, location);
		}

		case TK_L_PAREN:
		{
			const location_t l_paren_location = token_consume(prs);
			expression result = parse_expression(prs);
			if (!token_try_consume(prs, TK_R_PAREN))
			{
				parser_error(prs, expected_r_paren, l_paren_location);
				return expr_broken();
			}

			return result;
		}

		default:
			parser_error(prs, expected_expression);
			return expr_broken();
	}
}

/**
 *	Parse subscripting expression suffix
 *
 *	postfix-expression:
 *		postfix-expression '[' expression ']'
 *
 *	@param	prs			Parser
 *	@param	operand		Expression for subscripting
 *
 *	@return	Subscripting expression
 */
static expression parse_subscripting_expression_suffix(parser *const prs, expression operand)
{
	const location_t l_square_location = token_consume(prs);
	expression index = parse_expression(prs);

	if (prs->token != TK_R_SQUARE)
	{
		parser_error(prs, expected_r_square, l_square_location);
		return expr_broken();
	}

	const location_t r_square_location = token_consume(prs);

	if (!operand.is_valid)
	{
		return expr_broken();
	}

	const item_t operand_type = node_get_arg(&operand.nd, 0);
	if (!type_is_array(prs->sx, operand_type))
	{
		semantics_error(prs, l_square_location, typecheck_subscript_value);
		return expr_broken();
	}

	if (!type_is_integer(node_get_arg(&index.nd, 0)))
	{
		semantics_error(prs, index.location, typecheck_subscript_not_integer);
		return expr_broken();
	}

	const item_t element_type = type_get(prs->sx, (size_t)operand_type + 1);

	node slice_node = create_node(prs, OP_SLICE);
	node_add_arg(&slice_node, element_type);				// Тип элемента массива
	node_add_arg(&slice_node, LVALUE);						// Категория значения вырезки
	node_set_child(&slice_node, &operand.nd);				// Выражение-операнд
	node_set_child(&slice_node, &index.nd);					// Выражение-индекс

	return expr(slice_node, (location_t){ operand.location.begin, r_square_location.end });
}

/**
 *	Parse call expression suffix
 *
 *	postfix-expression:
 *		postfix-expression '(' argument-expression-list[opt] ')'
 *
 *	argument-expression-list:
 *		assignment-expression
 *		argument-expression-list ',' assignment-expression
 *
 *	@param	prs			Parser
 *	@param	operand		Expression for call
 *
 *	@return	Call expression
 */
static expression parse_call_expression_suffix(parser *const prs, expression operand)
{
	const location_t l_paren_location = token_consume(prs);

	const item_t operand_type = node_get_arg(&operand.nd, 0);
	if (!type_is_function(prs->sx, operand_type))
	{
		semantics_error(prs, l_paren_location, typecheck_call_not_function);
		return expr_broken();
	}

	const item_t return_type = type_get(prs->sx, (size_t)operand_type + 1);

	node call_node = create_node(prs, OP_CALL);
	node_add_arg(&call_node, return_type);					// Тип возвращамого значения
	node_add_arg(&call_node, RVALUE);						// Категория значения вызова
	node_set_child(&call_node, &operand.nd);				// Операнд вызова

	const size_t expected_args = (size_t)type_get(prs->sx, (size_t)operand_type + 2);
	size_t ref_arg_type = (size_t)operand_type + 3;
	size_t actual_args = 0;

	if (prs->token != TK_R_PAREN)
	{
		do
		{
			expression argument = parse_initializer(prs, type_get(prs->sx, ref_arg_type));
			if (!argument.is_valid)
			{
				return expr_broken();
			}

			const item_t expected_type = type_get(prs->sx, ref_arg_type);
			const item_t actual_type = node_get_arg(&argument.nd, 0);

			// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantics_error(prs, argument.location, typecheck_convert_incompatible);
			}

			node_set_child(&call_node, &argument.nd);		// i-ый аргумент вызова
			actual_args++;
			ref_arg_type++;
		} while (token_try_consume(prs, TK_COMMA) && expected_args != actual_args);

		if (prs->token != TK_R_PAREN)
		{
			parser_error(prs, expected_r_paren, l_paren_location);
			return expr_broken();
		}
	}

	const location_t r_paren_location = token_consume(prs);

	if (expected_args != actual_args)
	{
		semantics_error(prs, r_paren_location, wrong_number_of_params, expected_args, actual_args);
		return expr_broken();
	}

	return expr(call_node, (location_t){ operand.location.begin, r_paren_location.end });
}

/**
 *	Parse member expression suffix
 *
 *	postfix-expression:
 *		postfix-expression '.' identifier
 *		postfix-expression '->' identifier
 *
 *	@param	prs			Parser
 *	@param	operand		Expression for accessing a member
 *
 *	@return	Member expression
 */
static expression parse_member_expression_suffix(parser *const prs, expression operand)
{
	const token_t operator_token = prs->token;
	const location_t operator_location = token_consume(prs);

	if (prs->token != TK_IDENTIFIER)
	{
		parser_error(prs, expected_identifier);
		return expr_broken();
	}

	const size_t member_name = prs->lxr->repr;
	const location_t member_location = token_consume(prs);

	if (!operand.is_valid)
	{
		return expr_broken();
	}

	const item_t operand_type = node_get_arg(&operand.nd, 0);
	item_t struct_type;
	category_t category;

	if (operator_token == TK_PERIOD)
	{
		if (!type_is_structure(prs->sx, operand_type))
		{
			semantics_error(prs, operator_location, typecheck_member_reference_struct);
			return expr_broken();
		}

		struct_type = operand_type;
		category = (category_t)node_get_arg(&operand.nd, 1);
	}
	else // if (operator_token == TK_ARROW)
	{
		if (!type_is_struct_pointer(prs->sx, operand_type))
		{
			semantics_error(prs, operator_location, typecheck_member_reference_arrow);
			return expr_broken();
		}

		struct_type = type_get(prs->sx, (size_t)operand_type + 1);
		category = LVALUE;
	}

	item_t member_displ = 0;
	const size_t record_length = (size_t)type_get(prs->sx, (size_t)struct_type + 2);
	for (size_t i = 0; i < record_length; i += 2)
	{
		const item_t member_type = type_get(prs->sx, (size_t)struct_type + 3 + i);
		if (member_name == (size_t)type_get(prs->sx, (size_t)struct_type + 4 + i))
		{
			node select_node = create_node(prs, OP_SELECT);
			node_add_arg(&select_node, member_type);	// Тип значения поля
			node_add_arg(&select_node, category);		// Категория значения поля
			node_add_arg(&select_node, member_displ);	// Смещение поля структуры
			node_set_child(&select_node, &operand.nd);	// Выражение-операнд

			return expr(select_node, (location_t){ operand.location.begin, member_location.end });
		}

		member_displ += (item_t)type_size(prs->sx, member_type);
	}

	semantics_error(prs, member_location, no_member, repr_get_name(prs->sx, member_name));
	return expr_broken();
}

/**
 *	Parse postfix expression suffix
 *
 *	postfix-expression:
 *		primary-expression
 *		postfix-expression '[' expression ']'
 *		postfix-expression '(' argument-expression-list[opt] ')'
 *		postfix-expression '.' identifier
 *		postfix-expression '->' identifier
 *		postfix-expression '++'
 *		postfix-expression '--'
 *
 *	@param	prs			Parser
 *	@param	operand		Operand of postfix expression
 *
 *	@return	Postfix expression
 */
static expression parse_postfix_expression_suffix(parser *const prs, expression operand)
{
	while (true)
	{
		switch (prs->token)
		{
			default:
				return operand;

			case TK_L_SQUARE:
				operand = parse_subscripting_expression_suffix(prs, operand);
				break;

			case TK_L_PAREN:
				operand = parse_call_expression_suffix(prs, operand);
				break;

			case TK_PERIOD:
			case TK_ARROW:
				operand = parse_member_expression_suffix(prs, operand);
				break;

			case TK_PLUS_PLUS:
				operand = make_unary_expression(prs, operand, UN_POSTINC, token_consume(prs));
				break;

			case TK_MINUS_MINUS:
				operand = make_unary_expression(prs, operand, UN_POSTDEC, token_consume(prs));
				break;
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
 *		'&' '*' '+' '-' '~' '!' 'abs'
 *
 *	@param	prs			Parser
 *
 *	@return	Unary expression
 */
static expression parse_unary_expression(parser *const prs)
{
	switch (prs->token)
	{
		default:
		{
			const expression operand = parse_primary_expression(prs);
			return parse_postfix_expression_suffix(prs, operand);
		}

		case TK_PLUS_PLUS:
		case TK_MINUS_MINUS:
		case TK_AMP:
		case TK_STAR:
		case TK_PLUS:
		case TK_MINUS:
		case TK_TILDE:
		case TK_EXCLAIM:
		{
			const unary_t operator = token_to_unary(prs->token);
			const location_t operator_location = token_consume(prs);
			const expression operand = parse_unary_expression(prs);

			return make_unary_expression(prs, operand, operator, operator_location);
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
 *	@return Binary expression
 */
static expression parse_RHS_of_binary_expression(parser *const prs, expression LHS, const precedence_t min_prec)
{
	precedence_t next_token_prec = get_operator_precedence(prs->token);
	while (next_token_prec >= min_prec)
	{
		const token_t operator_token = prs->token;
		location_t operator_location = token_consume(prs);

		bool is_binary = true;
		expression middle = expr_broken();
		if (next_token_prec == PREC_CONDITIONAL)
		{
			is_binary = false;
			middle = parse_expression(prs);

			if (prs->token != TK_COLON)
			{
				parser_error(prs, expected_colon_in_conditional, operator_location);
			}

			operator_location = token_consume(prs);
		}

		expression RHS = (prs->token == TK_L_BRACE)
			? parse_initializer(prs, node_get_arg(&LHS.nd, 0))
			: parse_unary_expression(prs);

		const precedence_t this_prec = next_token_prec;
		next_token_prec = get_operator_precedence(prs->token);

		const bool is_right_associative = this_prec == PREC_CONDITIONAL || this_prec == PREC_ASSIGNMENT;
		if (this_prec < next_token_prec || (this_prec == next_token_prec && is_right_associative))
		{
			RHS = parse_RHS_of_binary_expression(prs, RHS, (this_prec + !is_right_associative));
			next_token_prec = get_operator_precedence(prs->token);
		}

		if (is_binary)
		{
			const binary_t operator = token_to_binary(operator_token);
			LHS = make_binary_expression(prs, LHS, RHS, operator, operator_location);
		}
		else
		{
			LHS = make_ternary_expression(prs, LHS, middle, RHS, operator_location);
		}
	}

	return LHS;
}

/**
 *	Parse structure initializer
 *
 *	@param	prs			Parser
 *	@param	type		Expected type
 *
 *	@return	Structure initializer
 */
static expression parse_struct_initializer(parser *const prs, const item_t type)
{
	const location_t l_brace_location = token_consume(prs);

	const size_t expected_fields = (size_t)(type_get(prs->sx, (size_t)type + 2) / 2);
	size_t actual_fields = 0;
	size_t ref_next_field = (size_t)type + 3;

	node list_node = create_node(prs, OP_LIST);
	node_add_arg(&list_node, type);					// Тип возвращамого значения
	node_add_arg(&list_node, RVALUE);				// Категория значения вызова

	do
	{
		expression initializer = parse_initializer(prs, type_get(prs->sx, ref_next_field));
		if (!initializer.is_valid)
		{
			return expr_broken();
		}

		const item_t expected_type = type_get(prs->sx, ref_next_field);
		const item_t actual_type = node_get_arg(&initializer.nd, 0);

		// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
		if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
		{
			semantics_error(prs, initializer.location, typecheck_convert_incompatible);
		}

		node_set_child(&list_node, &initializer.nd);		// i-ый инициализатор списка
		ref_next_field += 2;
		actual_fields++;
	} while (token_try_consume(prs, TK_COMMA) && expected_fields != actual_fields);

	if (prs->token != TK_R_BRACE)
	{
		parser_error(prs, expected_r_brace, l_brace_location);
		return expr_broken();
	}

	const location_t r_brace_location = token_consume(prs);

	if (expected_fields != actual_fields)
	{
		//semantics_error(prs, r_paren_location, wrong_number_of_params, expected_args, actual_args);
		return expr_broken();
	}

	return expr(list_node, (location_t){ l_brace_location.begin, r_brace_location.end });
}

/**
 *	Parse array initializer
 *
 *	@param	prs			Parser
 *	@param	type		Expected type
 *
 *	@return Array initializer
 */
static expression parse_array_initializer(parser *const prs, const item_t type)
{
	const location_t l_brace_location = token_consume(prs);

	size_t list_length = 0;

	node list_node = create_node(prs, OP_LIST);
	node_add_arg(&list_node, type);					// Тип возвращамого значения
	node_add_arg(&list_node, RVALUE);				// Категория значения вызова

	do
	{
		expression initializer = parse_initializer(prs, type_get(prs->sx, (size_t)type + 1));
		if (!initializer.is_valid)
		{
			return expr_broken();
		}

		const item_t expected_type = type_get(prs->sx, (size_t)type + 1);
		const item_t actual_type = node_get_arg(&initializer.nd, 0);

		// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
		if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
		{
			semantics_error(prs, initializer.location, typecheck_convert_incompatible);
		}

		node_set_child(&list_node, &initializer.nd);	// i-ый инициализатор списка
		list_length++;
	} while (token_try_consume(prs, TK_COMMA));

	if (prs->token != TK_R_BRACE)
	{
		parser_error(prs, expected_r_brace, l_brace_location);
		return expr_broken();
	}

	const location_t r_brace_location = token_consume(prs);

	return expr(list_node, (location_t){ l_brace_location.begin, r_brace_location.end });
}

static expression parse_braced_initializer(parser *const prs, const item_t expected_type)
{
	if (type_is_array(prs->sx, expected_type))
	{
		return parse_array_initializer(prs, expected_type);
	}
	else if (type_is_structure(prs->sx, expected_type))
	{
		return parse_struct_initializer(prs, expected_type);
	}
	else
	{
		parser_error(prs, wrong_init);
		return expr_broken();
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


expression parse_assignment_expression(parser *const prs)
{
	const expression LHS = parse_unary_expression(prs);
	return parse_RHS_of_binary_expression(prs, LHS, PREC_ASSIGNMENT);
}

expression parse_expression(parser *const prs)
{
	const expression LHS = parse_assignment_expression(prs);
	return parse_RHS_of_binary_expression(prs, LHS, PREC_COMMA);
}

expression parse_constant_expression(parser *const prs)
{
	const expression LHS = parse_unary_expression(prs);
	return parse_RHS_of_binary_expression(prs, LHS, PREC_CONDITIONAL);
}

expression parse_initializer(parser *const prs, const item_t expected_type)
{
	if (prs->token == TK_L_BRACE)
	{
		return parse_braced_initializer(prs, expected_type);
	}

	return parse_assignment_expression(prs);
}
