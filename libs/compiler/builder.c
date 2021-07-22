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

#include "builder.h"
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

/**
 *	Emit a semantics error
 *
 *	@param	sx			Syntax structure
 *	@param	loc			Error location
 *	@param	num			Error code
 */
static void semantics_error(syntax *const sx, const location loc, error_t num, ...)
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


expression build_identifier_expression(syntax *const sx, const size_t name, const location loc)
{
	const item_t identifier = repr_get_reference(sx, name);

	if (identifier == ITEM_MAX)
	{
		semantics_error(sx, loc, undeclared_var_use, repr_get_name(sx, name));
		return expression_broken();
	}

	const item_t type = ident_get_type(sx, (size_t)identifier);
	const category_t category = type_is_function(sx, type) ? RVALUE : LVALUE;

	node nd = node_create(sx, OP_IDENTIFIER);
	node_add_arg(&nd, type);					// Тип значения идентификатора
	node_add_arg(&nd, category);				// Категория значения идентификатора
	node_add_arg(&nd, (item_t)loc.begin);		// Начальная позиция идентификатора
	node_add_arg(&nd, (item_t)loc.end);			// Конечная позиция идентификатора
	node_add_arg(&nd, identifier);				// Индекс в таблице идентификаторов

	return expression_create(nd);
}

expression build_integer_literal_expression(syntax *const sx, const int value, const location loc)
{
	node nd = node_create(sx, OP_CONSTANT);
	node_add_arg(&nd, TYPE_INTEGER);			// Тип значения константы
	node_add_arg(&nd, RVALUE);					// Категория значения константы
	node_add_arg(&nd, (item_t)loc.begin);		// Начальная позиция константы
	node_add_arg(&nd, (item_t)loc.end);			// Конечная позиция константы
	node_add_arg(&nd, value);					// Значение константы

	return expression_create(nd);
}

expression build_floating_literal_expression(syntax *const sx, const double value, const location loc)
{
	item_t temp;
	memcpy(&temp, &value, sizeof(double));

	node nd = node_create(sx, OP_CONSTANT);
	node_add_arg(&nd, TYPE_FLOATING);			// Тип значения константы
	node_add_arg(&nd, RVALUE);					// Категория значения константы
	node_add_arg(&nd, (item_t)loc.begin);		// Начальная позиция константы
	node_add_arg(&nd, (item_t)loc.end);			// Конечная позиция константы
	node_add_arg(&nd, temp);					// Значение константы

	return expression_create(nd);
}

expression build_string_literal_expression(syntax *const sx, const vector value, const location loc)
{
	const item_t type = type_array(sx, TYPE_INTEGER);

	node nd = node_create(sx, OP_STRING);
	node_add_arg(&nd, type);						// Тип строки
	node_add_arg(&nd, LVALUE);						// Категория значения строки
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция строки
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция строки
	for (size_t i = 0, length = vector_size(&value); i < length; i++)
	{
		node_add_arg(&nd, vector_get(&value, i));	// i-ый символ строки
	}

	return expression_create(nd);
}

expression build_subscript_expression(syntax *const sx, const expression base, const expression index
									  , const location l_loc, const location r_loc)
{
	if (!expression_is_valid(base) || !expression_is_valid(index))
	{
		return expression_broken();
	}

	const item_t base_type = expression_get_type(base);
	if (!type_is_array(sx, base_type))
	{
		semantics_error(sx, l_loc, typecheck_subscript_value);
		return expression_broken();
	}

	const item_t index_type = expression_get_type(index);
	if (!type_is_integer(index_type))
	{
		semantics_error(sx, expression_get_location(index), typecheck_subscript_not_integer);
		return expression_broken();
	}

	const item_t element_type = type_get(sx, (size_t)base_type + 1);
	const size_t expr_start = expression_get_location(base).begin;
	const node nd_base = expression_get_node(base);
	const node nd_index = expression_get_node(index);

	node nd = node_create(sx, OP_SLICE);
	node_add_arg(&nd, element_type);				// Тип элемента массива
	node_add_arg(&nd, LVALUE);						// Категория значения вырезки
	node_add_arg(&nd, (item_t)expr_start);			// Начальная позиция вырезки
	node_add_arg(&nd, (item_t)r_loc.end);			// Конечная позиция вырезки
	node_set_child(&nd, &nd_base);					// Выражение-операнд
	node_set_child(&nd, &nd_index);					// Выражение-индекс

	return expression_create(nd);
}

expression build_call_expression(syntax *const sx, const expression callee, const expression_list *args
								, const location l_loc, const location r_loc)
{
	if (!expression_is_valid(callee))
	{
		return expression_broken();
	}

	const item_t operand_type = expression_get_type(callee);
	if (!type_is_function(sx, operand_type))
	{
		semantics_error(sx, l_loc, typecheck_call_not_function);
		return expression_broken();
	}

	const size_t expected_args = (size_t)type_get(sx, (size_t)operand_type + 2);
	const size_t actual_args = args == NULL ? 0 : expression_list_size(args);

	if (expected_args != actual_args)
	{
		semantics_error(sx, r_loc, wrong_number_of_params, expected_args, actual_args);
		return expression_broken();
	}

	size_t ref_arg_type = (size_t)operand_type + 3;

	for (size_t i = 0; i < actual_args; i++)
	{
		const expression argument = expression_list_get(args, i);
		if (!expression_is_valid(argument))
		{
			return expression_broken();
		}

		const item_t expected_type = type_get(sx, ref_arg_type + i);
		const item_t actual_type = expression_get_type(argument);

		// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
		if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
		{
			semantics_error(sx, expression_get_location(argument), typecheck_convert_incompatible);
			return expression_broken();
		}
	}

	const item_t return_type = type_get(sx, (size_t)operand_type + 1);
	const size_t expr_start = expression_get_location(callee).begin;
	const node nd_callee = expression_get_node(callee);

	node nd = node_create(sx, OP_CALL);
	node_add_arg(&nd, return_type);						// Тип возвращамого значения
	node_add_arg(&nd, RVALUE);							// Категория значения вызова
	node_add_arg(&nd, (item_t)expr_start);				// Начальная позиция вызова
	node_add_arg(&nd, (item_t)r_loc.end);				// Конечная позиция вызова
	node_set_child(&nd, &nd_callee);					// Операнд вызова
	for (size_t i = 0; i < actual_args; i++)
	{
		const expression argument = expression_list_get(args, i);
		const node nd_argument = expression_get_node(argument);
		node_set_child(&nd, &nd_argument);				// i-ый аргумент вызова
	}

	return expression_create(nd);
}

expression build_member_expression(syntax *const sx, const expression base, const bool is_arrow, const size_t name
								   , const location op_loc, const location id_loc)
{
	if (!expression_is_valid(base))
	{
		return expression_broken();
	}

	const item_t operand_type = expression_get_type(base);
	item_t struct_type;
	category_t category;

	if (!is_arrow)
	{
		if (!type_is_structure(sx, operand_type))
		{
			semantics_error(sx, op_loc, typecheck_member_reference_struct);
			return expression_broken();
		}

		struct_type = operand_type;
		category = (category_t)node_get_arg(&base.nd, 1);
	}
	else
	{
		if (!type_is_struct_pointer(sx, operand_type))
		{
			semantics_error(sx, op_loc, typecheck_member_reference_arrow);
			return expression_broken();
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
			const size_t expr_start = expression_get_location(base).begin;
			const node nd_base = expression_get_node(base);

			node nd = node_create(sx, OP_SELECT);
			node_add_arg(&nd, member_type);				// Тип значения поля
			node_add_arg(&nd, category);				// Категория значения выборкм
			node_add_arg(&nd, (item_t)expr_start);		// Начальная позиция выборкм
			node_add_arg(&nd, (item_t)id_loc.end);		// Конечная позиция выборкм
			node_add_arg(&nd, member_displ);			// Смещение поля структуры
			node_set_child(&nd, &nd_base);				// Выражение-операнд

			return expression_create(nd);
		}

		member_displ += (item_t)type_size(sx, member_type);
	}

	semantics_error(sx, id_loc, no_member, repr_get_name(sx, name));
	return expression_broken();
}

expression build_upb_expression(syntax *const sx, const expression dimension, const expression array)
{
	if (!expression_is_valid(dimension) || !expression_is_valid(array))
	{
		return expression_broken();
	}

	const item_t dimension_type = expression_get_type(dimension);
	if (!type_is_integer(dimension_type))
	{
		semantics_error(sx, expression_get_location(dimension), not_int_in_stanfunc);
		return expression_broken();
	}

	const item_t array_type = expression_get_type(array);
	if (!type_is_array(sx, array_type))
	{
		semantics_error(sx, expression_get_location(dimension), not_array_in_stanfunc);
		return expression_broken();
	}

	const node nd_dimension = expression_get_node(dimension);
	const node nd_array = expression_get_node(array);

	node nd = node_create(sx, OP_UPB);
	node_add_arg(&nd, TYPE_INTEGER);		// Тип значения поля
	node_add_arg(&nd, RVALUE);				// Категория значения поля
	node_add_arg(&nd, (item_t)expression_get_location(dimension).begin);
	node_add_arg(&nd, (item_t)expression_get_location(array).begin);
	node_set_child(&nd, &nd_dimension);		// Выражение-операнд
	node_set_child(&nd, &nd_array);			// Выражение-операнд

	return expression_create(nd);
}

expression build_unary_expression(syntax *const sx, const expression operand
								  , const unary_t op_kind, const location op_loc)
{
	if (!expression_is_valid(operand))
	{
		return expression_broken();
	}

	const item_t operand_type = expression_get_type(operand);

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
				return expression_broken();
			}

			if (!expression_is_lvalue(operand))
			{
				semantics_error(sx, op_loc, typecheck_expression_not_lvalue);
				return expression_broken();
			}

			result_type = operand_type;
			break;
		}

		case UN_ADDRESS:
		{
			if (!expression_is_lvalue(operand))
			{
				semantics_error(sx, op_loc, typecheck_invalid_lvalue_addrof);
				return expression_broken();
			}

			result_type = type_pointer(sx, operand_type);
			break;
		}

		case UN_INDIRECTION:
		{
			if (!type_is_pointer(sx, operand_type))
			{
				semantics_error(sx, op_loc, typecheck_indirection_requires_pointer);
				return expression_broken();
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
				return expression_broken();
			}

			result_type = operand_type;
			break;
		}

		case UN_NOT:
		{
			if (!type_is_integer(operand_type))
			{
				semantics_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return expression_broken();
			}

			result_type = TYPE_INTEGER;
			break;
		}

		case UN_LOGNOT:
		{
			if (!type_is_scalar(sx, operand_type))
			{
				semantics_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return expression_broken();
			}

			result_type = TYPE_INTEGER;
			break;
		}
	}

	const node nd_operand = expression_get_node(operand);
	const location loc = is_prefix
		? (location){ op_loc.begin, expression_get_location(operand).end }
		: (location){ expression_get_location(operand).begin, op_loc.end };

	node nd = node_create(sx, OP_UNARY);
	node_add_arg(&nd, result_type);						// Тип значения выражения
	node_add_arg(&nd, category);						// Категория значения выражения
	node_add_arg(&nd, (item_t)loc.begin);				// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);					// Конечная позиция выражения
	node_add_arg(&nd, op_kind);							// Тип унарного оператора
	node_set_child(&nd, &nd_operand);					// Выражение-операнд

	return expression_create(nd);
}

expression build_binary_expression(syntax *const sx, const expression left, const expression right
								   , const binary_t op_kind, const location op_loc)
{
	if (!expression_is_valid(left) || !expression_is_valid(right))
	{
		return expression_broken();
	}

	const item_t left_type = expression_get_type(left);
	const item_t right_type = expression_get_type(right);

	if (operation_is_assignment(op_kind)
		&& (!expression_is_lvalue(left) || (type_is_floating(right_type) && type_is_integer(left_type))))
	{
		semantics_error(sx, op_loc, unassignable);
		return expression_broken();
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
			return expression_broken();
		}

		result_type = left_type;
	}
	else
	{
		if (!type_is_arithmetic(left_type) || !type_is_arithmetic(right_type))
		{
			semantics_error(sx, op_loc, typecheck_binary_expr);
			return expression_broken();
		}

		if (operation_is_assignment(op_kind) && !expression_is_lvalue(left))
		{
			semantics_error(sx, op_loc, unassignable);
			return expression_broken();
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
					return expression_broken();
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

	const node nd_left = expression_get_node(left);
	const node nd_right = expression_get_node(right);

	node nd = node_create(sx, OP_BINARY);
	node_add_arg(&nd, result_type);		// Тип значения
	node_add_arg(&nd, RVALUE);			// Категория значения
	node_add_arg(&nd, (item_t)expression_get_location(left).begin);
	node_add_arg(&nd, (item_t)expression_get_location(right).end);
	node_add_arg(&nd, op_kind);			// Вид оператора
	node_set_child(&nd, &nd_left);		// Второй операнд
	node_set_child(&nd, &nd_right);		// Третий операнд

	return expression_create(nd);
}

expression build_ternary_expression(syntax *const sx, const expression left, const expression middle
									, const expression right, const location op_loc)
{
	if (!expression_is_valid(left) || !expression_is_valid(middle) || !expression_is_valid(right))
	{
		return expression_broken();
	}

	const item_t left_type = expression_get_type(left);
	const item_t middle_type = expression_get_type(middle);
	const item_t right_type = expression_get_type(right);

	if (!type_is_scalar(sx, left_type))
	{
		semantics_error(sx, expression_get_location(left), typecheck_statement_requires_scalar);
		return expression_broken();
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
		return expression_broken();
	}

	const node nd_left = expression_get_node(left);
	const node nd_middle = expression_get_node(middle);
	const node nd_right = expression_get_node(right);

	node nd = node_create(sx, OP_TERNARY);
	node_add_arg(&nd, result_type);		// Тип значения
	node_add_arg(&nd, RVALUE);			// Категория значения
	node_add_arg(&nd, (item_t)expression_get_location(left).begin);
	node_add_arg(&nd, (item_t)expression_get_location(right).begin);
	node_set_child(&nd, &nd_left);		// Первый операнд
	node_set_child(&nd, &nd_middle);		// Второй операнд
	node_set_child(&nd, &nd_right);		// Третий операнд

	return expression_create(nd);
}

expression build_init_list_expression(syntax *const sx, const expression_list *inits, const item_t type
									  , const location l_loc, const location r_loc)
{
	const size_t actual_inits = expression_list_size(inits);
	if (actual_inits == 0)
	{
		semantics_error(sx, l_loc, empty_init);
		return expression_broken();
	}

	if (type_is_structure(sx, type))
	{
		const size_t expected_inits = (size_t)(type_get(sx, (size_t)type + 2) / 2);

		if (expected_inits != actual_inits)
		{
			semantics_error(sx, r_loc, wrong_init_in_actparam, expected_inits, actual_inits);
			return expression_broken();
		}

		size_t ref_next_field = (size_t)type + 3;
		for (size_t i = 0; i < actual_inits; i++)
		{
			const expression initializer = expression_list_get(inits, i);
			if (!expression_is_valid(initializer))
			{
				return expression_broken();
			}

			const item_t expected_type = type_get(sx, ref_next_field);
			const item_t actual_type = expression_get_type(initializer);

			// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantics_error(sx, expression_get_location(initializer), typecheck_convert_incompatible);
				return expression_broken();
			}

			ref_next_field += 2;
		}
	}
	else if (type_is_array(sx, type))
	{
		const item_t expected_type = type_get(sx, (size_t)type + 1);
		for (size_t i = 0; i < actual_inits; i++)
		{
			const expression initializer = expression_list_get(inits, i);
			if (!expression_is_valid(initializer))
			{
				return expression_broken();
			}

			const item_t actual_type = expression_get_type(initializer);

			// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantics_error(sx, expression_get_location(initializer), typecheck_convert_incompatible);
				return expression_broken();
			}
		}
	}
	else
	{
		semantics_error(sx, l_loc, wrong_init);
		return expression_broken();
	}

	node nd = node_create(sx, OP_LIST);
	node_add_arg(&nd, type);							// Тип возвращамого значения
	node_add_arg(&nd, RVALUE);							// Категория значения вызова
	node_add_arg(&nd, (item_t)l_loc.begin);
	node_add_arg(&nd, (item_t)r_loc.end);
	for (size_t i = 0; i < actual_inits; i++)
	{
		const expression initializer = expression_list_get(inits, i);
		const node nd_initializer = expression_get_node(initializer);
		node_set_child(&nd, &nd_initializer);			// i-ый аргумент вызова
	}

	return expression_create(nd);
}
