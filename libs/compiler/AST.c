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

#include "AST.h"
#include <string.h>


static node node_create(syntax *const sx, operation_t type)
{
	return node_add_child(&sx->nd, type);
}

static void node_set_child(const node *const parent, const node *const child)
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

/**
 *	Emit a semantics error
 *
 *	@param	sx			Syntax structure
 *	@param	loc			Error location
 *	@param	num			Error code
 */
static void semantics_error(syntax *const sx, const location_t loc, error_t num, ...)
{
	va_list args;
	va_start(args, num);

	const size_t prev_loc = in_get_position(sx->io);
	in_set_position(sx->io, loc.begin);

	verror(sx->io, num, args);
	sx->was_error = true;

	va_end(args);

	in_set_position(sx->io, prev_loc);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */

item_t expression_get_type(const expression expr)
{
	return expr.is_valid ? node_get_arg(&expr.nd, 0) : TYPE_UNDEFINED;
}

expression invalid_expression()
{
	return (expression){ .is_valid = false };
}

expression identifier_expression(syntax *const sx, const size_t name, const location_t loc)
{
	const item_t identifier = repr_get_reference(sx, name);

	if (identifier == ITEM_MAX)
	{
		semantics_error(sx, loc, undeclared_var_use, repr_get_name(sx, name));
		return invalid_expression();
	}

	const item_t type = ident_get_type(sx, (size_t)identifier);
	const category_t category = type_is_function(sx, type) ? RVALUE : LVALUE;

	node identifier_node = node_create(sx, OP_IDENTIFIER);
	node_add_arg(&identifier_node, type);					// Тип значения идентификатора
	node_add_arg(&identifier_node, category);				// Категория значения идентификатора
	node_add_arg(&identifier_node, identifier);				// Индекс в таблице идентификаторов

	return expr(identifier_node, loc);
}

expression integer_literal_expression(syntax *const sx, const int value, const location_t loc)
{
	node constant_node = node_create(sx, OP_CONSTANT);
	node_add_arg(&constant_node, TYPE_INTEGER);				// Тип значения константы
	node_add_arg(&constant_node, RVALUE);					// Категория значения константы
	node_add_arg(&constant_node, value);					// Значение константы

	return expr(constant_node, loc);
}

expression floating_literal_expression(syntax *const sx, const double value, const location_t loc)
{
	item_t temp;
	memcpy(&temp, &value, sizeof(double));

	node constant_node = node_create(sx, OP_CONSTANT);
	node_add_arg(&constant_node, TYPE_FLOATING);			// Тип значения константы
	node_add_arg(&constant_node, RVALUE);					// Категория значения константы
	node_add_arg(&constant_node, temp);						// Значение константы

	return expr(constant_node, loc);
}

expression string_literal_expression(syntax *const sx, const vector value, const location_t loc)
{
	const item_t type = type_array(sx, TYPE_INTEGER);

	node string_node = node_create(sx, OP_STRING);
	node_add_arg(&string_node, type);						// Тип строки
	node_add_arg(&string_node, LVALUE);						// Категория значения строки
	for (size_t i = 0, length = vector_size(&value); i < length; i++)
	{
		node_add_arg(&string_node, vector_get(&value, i));	// i-ый символ строки
	}

	return expr(string_node, loc);
}

expression subscript_expression(syntax *const sx, const expression base, const expression index
								, const location_t l_loc, const location_t r_loc)
{
	if (!base.is_valid || !index.is_valid)
	{
		return invalid_expression();
	}

	const item_t operand_type = node_get_arg(&base.nd, 0);
	if (!type_is_array(sx, operand_type))
	{
		semantics_error(sx, l_loc, typecheck_subscript_value);
		return invalid_expression();
	}

	if (!type_is_integer(node_get_arg(&index.nd, 0)))
	{
		semantics_error(sx, index.location, typecheck_subscript_not_integer);
		return invalid_expression();
	}

	const item_t element_type = type_get(sx, (size_t)operand_type + 1);

	node slice_node = node_create(sx, OP_SLICE);
	node_add_arg(&slice_node, element_type);				// Тип элемента массива
	node_add_arg(&slice_node, LVALUE);						// Категория значения вырезки
	node_set_child(&slice_node, &base.nd);					// Выражение-операнд
	node_set_child(&slice_node, &index.nd);					// Выражение-индекс

	return expr(slice_node, (location_t){ base.location.begin, r_loc.end });
}

expression call_expression(syntax *const sx, const expression callee, const expression_list *args
						   , const location_t l_loc, const location_t r_loc)
{
	if (!callee.is_valid)
	{
		return invalid_expression();
	}

	const item_t operand_type = node_get_arg(&callee.nd, 0);
	if (!type_is_function(sx, operand_type))
	{
		semantics_error(sx, l_loc, typecheck_call_not_function);
		return invalid_expression();
	}

	const item_t return_type = type_get(sx, (size_t)operand_type + 1);
	const size_t expected_args = (size_t)type_get(sx, (size_t)operand_type + 2);
	const size_t actual_args = args->length;

	if (expected_args != actual_args)
	{
		semantics_error(sx, r_loc, wrong_number_of_params, expected_args, actual_args);
		return invalid_expression();
	}

	size_t ref_arg_type = (size_t)operand_type + 3;

	for (unsigned i = 0; i < actual_args; i++)
	{
		if (!args->expressions[i].is_valid)
		{
			return invalid_expression();
		}

		const item_t expected_type = type_get(sx, ref_arg_type + i);
		const item_t actual_type = node_get_arg(&args->expressions[i].nd, 0);

		// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
		if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
		{
			semantics_error(sx, args->expressions[i].location, typecheck_convert_incompatible);
			return invalid_expression();
		}
	}

	node call_node = node_create(sx, OP_CALL);
	node_add_arg(&call_node, return_type);						// Тип возвращамого значения
	node_add_arg(&call_node, RVALUE);							// Категория значения вызова
	node_set_child(&call_node, &callee.nd);						// Операнд вызова
	for (unsigned i = 0; i < actual_args; i++)
	{
		node_set_child(&call_node, &args->expressions[i].nd);	// i-ый аргумент вызова
	}

	return expr(call_node, (location_t){ callee.location.begin, r_loc.end });
}

expression member_expression(syntax *const sx, const expression base, const bool is_arrow, const size_t name
							, const location_t op_loc, const location_t id_loc)
{
	if (!base.is_valid)
	{
		return invalid_expression();
	}

	const item_t operand_type = node_get_arg(&base.nd, 0);
	item_t struct_type;
	category_t category;

	if (!is_arrow)
	{
		if (!type_is_structure(sx, operand_type))
		{
			semantics_error(sx, op_loc, typecheck_member_reference_struct);
			return invalid_expression();
		}

		struct_type = operand_type;
		category = (category_t)node_get_arg(&base.nd, 1);
	}
	else
	{
		if (!type_is_struct_pointer(sx, operand_type))
		{
			semantics_error(sx, op_loc, typecheck_member_reference_arrow);
			return invalid_expression();
		}

		struct_type = type_get(sx, (size_t)operand_type + 1);
		category = LVALUE;
	}

	item_t member_displ = 0;
	const size_t record_length = (size_t)type_get(sx, (size_t)struct_type + 2);
	for (size_t i = 0; i < record_length; i += 2)
	{
		const item_t member_type = type_get(sx, (size_t)struct_type + 3 + i);
		if (name == (size_t)type_get(sx, (size_t)struct_type + 4 + i))
		{
			node select_node = node_create(sx, OP_SELECT);
			node_add_arg(&select_node, member_type);	// Тип значения поля
			node_add_arg(&select_node, category);		// Категория значения поля
			node_add_arg(&select_node, member_displ);	// Смещение поля структуры
			node_set_child(&select_node, &base.nd);		// Выражение-операнд

			return expr(select_node, (location_t){ base.location.begin, id_loc.end });
		}

		member_displ += (item_t)type_size(sx, member_type);
	}

	semantics_error(sx, id_loc, no_member, repr_get_name(sx, name));
	return invalid_expression();
}

expression unary_expression(syntax *const sx, const expression operand, const unary_t op_kind, const location_t op_loc)
{
	if (!operand.is_valid)
	{
		return invalid_expression();
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
				semantics_error(sx, op_loc, typecheck_illegal_increment, op_kind);
				return invalid_expression();
			}

			if (node_get_arg(&operand.nd, 1) != LVALUE)
			{
				semantics_error(sx, op_loc, typecheck_expression_not_lvalue);
				return invalid_expression();
			}

			result_type = operand_type;
			break;
		}

		case UN_ADDRESS:
		{
			if (node_get_arg(&operand.nd, 1) != LVALUE)
			{
				semantics_error(sx, op_loc, typecheck_invalid_lvalue_addrof);
				return invalid_expression();
			}

			result_type = type_pointer(sx, operand_type);
			break;
		}

		case UN_INDIRECTION:
		{
			if (!type_is_pointer(sx, operand_type))
			{
				semantics_error(sx, op_loc, typecheck_indirection_requires_pointer);
				return invalid_expression();
			}

			result_type = type_get(sx, (size_t)operand_type + 1);
			category = LVALUE;
			break;
		}

		case UN_ABS:
		case UN_PLUS:
		case UN_MINUS:
		{
			if (!type_is_arithmetic(operand_type))
			{
				semantics_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return invalid_expression();
			}

			result_type = operand_type;
			break;
		}

		case UN_NOT:
		{
			if (!type_is_integer(operand_type))
			{
				semantics_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return invalid_expression();
			}

			result_type = TYPE_INTEGER;
			break;
		}

		case UN_LOGNOT:
		{
			if (!type_is_scalar(sx, operand_type))
			{
				semantics_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return invalid_expression();
			}

			result_type = TYPE_INTEGER;
			break;
		}
	}

	node unary_node = node_create(sx, OP_UNARY);
	node_add_arg(&unary_node, result_type);		// Тип значения
	node_add_arg(&unary_node, category);		// Категория значения
	node_add_arg(&unary_node, op_kind);			// Тип унарного оператора
	node_set_child(&unary_node, &operand.nd);	// Выражение-операнд

	location_t location = is_prefix
						? (location_t){ op_loc.begin, operand.location.end }
						: (location_t){ operand.location.begin, op_loc.end };

	return expr(unary_node, location);
}

expression binary_expression(syntax *const sx, const expression left, const expression right
									, const binary_t op_kind, const location_t op_loc)
{
	if (!left.is_valid || !right.is_valid)
	{
		return invalid_expression();
	}

	const item_t left_type = node_get_arg(&left.nd, 0);
	const item_t right_type = node_get_arg(&right.nd, 0);

	if (operation_is_assignment(op_kind)
		&& ((node_get_arg(&left.nd, 1) != LVALUE)
			|| (type_is_floating(right_type) && type_is_integer(left_type))))
	{
		semantics_error(sx, op_loc, unassignable);
		return invalid_expression();
	}

	item_t result_type;
	if (op_kind == BIN_COMMA)
	{
		result_type = right_type;
	}
	else if (op_kind == BIN_ASSIGN)
	{
		// Особый случай, так как тут могут быть операции с агрегатными типами

		// Несовпадение типов может быть только в случае, когда слева floating, а справа integer
		if (left_type != right_type && !(type_is_floating(left_type) && type_is_integer(right_type)))
		{
			semantics_error(sx, op_loc, typecheck_convert_incompatible);
			return invalid_expression();
		}

		result_type = left_type;
	}
	else
	{
		if (!type_is_arithmetic(left_type) || !type_is_arithmetic(right_type))
		{
			semantics_error(sx, op_loc, typecheck_binary_expr);
			return invalid_expression();
		}

		if (operation_is_assignment(op_kind) && (node_get_arg(&left.nd, 1) != LVALUE))
		{
			semantics_error(sx, op_loc, unassignable);
			return invalid_expression();
		}

		switch (op_kind)
		{
			case BIN_REM:
			case BIN_LOG_OR:
			case BIN_LOG_AND:
			case BIN_OR:
			case BIN_XOR:
			case BIN_AND:
			case BIN_SHL:
			case BIN_SHR:
			case BIN_REM_ASSIGN:
			case BIN_OR_ASSIGN:
			case BIN_XOR_ASSIGN:
			case BIN_AND_ASSIGN:
			case BIN_SHL_ASSIGN:
			case BIN_SHR_ASSIGN:
			{
				if (!type_is_integer(left_type) || !type_is_integer(right_type))
				{
					semantics_error(sx, op_loc, int_op_for_float);
					return invalid_expression();
				}

				result_type = TYPE_INTEGER;
				break;
			}
			case BIN_EQ:
			case BIN_NE:
			case BIN_LE:
			case BIN_GE:
			case BIN_LT:
			case BIN_GT:
				result_type = TYPE_INTEGER;
				break;

			default:
				result_type = type_is_floating(left_type) ? TYPE_FLOATING : right_type;
				break;
		}
	}

	node binary_node = node_create(sx, OP_BINARY);
	node_add_arg(&binary_node, result_type);		// Тип значения
	node_add_arg(&binary_node, RVALUE);				// Категория значения
	node_add_arg(&binary_node, op_kind);			// Вид оператора
	node_set_child(&binary_node, &left.nd);			// Второй операнд
	node_set_child(&binary_node, &right.nd);		// Третий операнд

	return expr(binary_node, (location_t){ left.location.begin, right.location.end });
}

expression ternary_expression(syntax *const sx, const expression left, const expression middle
							, const expression right, const location_t op_loc)
{
	if (!left.is_valid || !middle.is_valid || !right.is_valid)
	{
		return invalid_expression();
	}

	const item_t left_type = node_get_arg(&left.nd, 0);
	const item_t middle_type = node_get_arg(&middle.nd, 0);
	const item_t right_type = node_get_arg(&right.nd, 0);

	if (!type_is_scalar(sx, left_type))
	{
		semantics_error(sx, left.location, typecheck_statement_requires_scalar);
		return invalid_expression();
	}

	item_t result_type = middle_type;

	if ((type_is_floating(middle_type) && type_is_integer(right_type))
		|| (type_is_integer(middle_type) && type_is_floating(right_type)))
	{
		result_type = TYPE_FLOATING;
	}
	else if (middle_type != right_type)
	{
		semantics_error(sx, op_loc, typecheck_cond_incompatible_operands);
		return invalid_expression();
	}

	node ternary_node = node_create(sx, OP_TERNARY);
	node_add_arg(&ternary_node, result_type);		// Тип значения
	node_add_arg(&ternary_node, RVALUE);			// Категория значения
	node_set_child(&ternary_node, &left.nd);		// Первый операнд
	node_set_child(&ternary_node, &middle.nd);		// Второй операнд
	node_set_child(&ternary_node, &right.nd);		// Третий операнд

	return expr(ternary_node, (location_t){ left.location.begin, right.location.end });
}

expression init_list_expression(syntax *const sx, const expression_list *inits, const item_t type
								, const location_t l_loc, const location_t r_loc)
{
	const size_t actual_inits = inits->length;
	if (actual_inits == 0)
	{
		semantics_error(sx, l_loc, empty_init);
		return invalid_expression();
	}

	if (type_is_structure(sx, type))
	{
		const size_t expected_inits = (size_t)(type_get(sx, (size_t)type + 2) / 2);

		if (expected_inits != actual_inits)
		{
			semantics_error(sx, r_loc, wrong_init_in_actparam, expected_inits, actual_inits);
			return invalid_expression();
		}

		size_t ref_next_field = (size_t)type + 3;
		for (unsigned i = 0; i < actual_inits; i++)
		{
			if (!inits->expressions[i].is_valid)
			{
				return invalid_expression();
			}

			const item_t expected_type = type_get(sx, ref_next_field);
			const item_t actual_type = node_get_arg(&inits->expressions[i].nd, 0);

			// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantics_error(sx, inits->expressions[i].location, typecheck_convert_incompatible);
				return invalid_expression();
			}

			ref_next_field += 2;
		}
	}
	else if (type_is_array(sx, type))
	{
		const item_t expected_type = type_get(sx, (size_t)type + 1);
		for (unsigned i = 0; i < actual_inits; i++)
		{
			if (!inits->expressions[i].is_valid)
			{
				return invalid_expression();
			}

			const item_t actual_type = node_get_arg(&inits->expressions[i].nd, 0);

			// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantics_error(sx, inits->expressions[i].location, typecheck_convert_incompatible);
				return invalid_expression();
			}
		}
	}
	else
	{
		semantics_error(sx, l_loc, wrong_init);
		return invalid_expression();
	}

	node init_list_node = node_create(sx, OP_LIST);
	node_add_arg(&init_list_node, type);							// Тип возвращамого значения
	node_add_arg(&init_list_node, RVALUE);							// Категория значения вызова
	for (unsigned i = 0; i < actual_inits; i++)
	{
		node_set_child(&init_list_node, &inits->expressions[i].nd);	// i-ый аргумент вызова
	}

	return expr(init_list_node, (location_t){ l_loc.begin, r_loc.end });
}
