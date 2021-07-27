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


static inline node node_create(syntax *const sx, operation_t type)
{
	return node_add_child(&sx->nd, type);
}

static inline void node_set_child(const node *const parent, const node *const child)
{
	node temp = node_add_child(parent, OP_NOP);
	node_swap(child, &temp);
	node_remove(&temp);
}

/**
 *	Emit a semantic error
 *
 *	@param	sx			Syntax structure
 *	@param	loc			Error location
 *	@param	num			Error code
 */
static void semantic_error(syntax *const sx, const location loc, error_t num, ...)
{
	const size_t prev_loc = in_get_position(sx->io);
	in_set_position(sx->io, loc.begin);

	va_list args;
	va_start(args, num);

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


node build_identifier_expression(syntax *const sx, const size_t name, const location loc)
{
	const item_t identifier = repr_get_reference(sx, name);

	if (identifier == ITEM_MAX)
	{
		semantic_error(sx, loc, undeclared_var_use, repr_get_name(sx, name));
		return build_broken_expression();
	}

	const item_t type = ident_get_type(sx, (size_t)identifier);
	const category_t category = type_is_function(sx, type) ? RVALUE : LVALUE;

	node nd = node_create(sx, OP_IDENTIFIER);
	node_add_arg(&nd, type);						// Тип значения идентификатора
	node_add_arg(&nd, category);					// Категория значения идентификатора
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция идентификатора
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция идентификатора
	node_add_arg(&nd, identifier);					// Индекс в таблице идентификаторов

	return nd;
}

node build_integer_literal_expression(syntax *const sx, const int value, const location loc)
{
	node nd = node_create(sx, OP_CONSTANT);
	node_add_arg(&nd, TYPE_INTEGER);				// Тип значения константы
	node_add_arg(&nd, RVALUE);						// Категория значения константы
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция константы
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция константы
	node_add_arg(&nd, value);						// Значение константы

	return nd;
}

node build_floating_literal_expression(syntax *const sx, const double value, const location loc)
{
	uint64_t num64;
	memcpy(&num64, &value, sizeof(int64_t));

	const item_t fst = num64 & 0x00000000ffffffff;
	const item_t snd = (num64 & 0xffffffff00000000) >> 32;

	node nd = node_create(sx, OP_CONSTANT);
	node_add_arg(&nd, TYPE_FLOATING);				// Тип значения константы
	node_add_arg(&nd, RVALUE);						// Категория значения константы
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция константы
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция константы
	node_add_arg(&nd, fst);							// Первая часть значения константы
	node_add_arg(&nd, snd);							// Вторая часть значения константы

	return nd;
}

node build_string_literal_expression(syntax *const sx, const vector *const value, const location loc)
{
	const item_t type = type_array(sx, TYPE_INTEGER);

	node nd = node_create(sx, OP_STRING);
	node_add_arg(&nd, type);						// Тип строки
	node_add_arg(&nd, LVALUE);						// Категория значения строки
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция строки
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция строки

	const size_t length = vector_size(value);
	for (size_t i = 0; i < length; i++)
	{
		node_add_arg(&nd, vector_get(value, i));	// i-ый символ строки
	}

	return nd;
}

node build_subscript_expression(syntax *const sx, const node *const nd_base, const node *const nd_index
	, const location l_loc, const location r_loc)
{
	if (!node_is_correct(nd_base) || !node_is_correct(nd_index))
	{
		return build_broken_expression();
	}

	const item_t base_type = expression_get_type(nd_base);
	if (!type_is_array(sx, base_type))
	{
		semantic_error(sx, l_loc, typecheck_subscript_value);
		return build_broken_expression();
	}

	const item_t index_type = expression_get_type(nd_index);
	if (!type_is_integer(index_type))
	{
		semantic_error(sx, expression_get_location(nd_index), typecheck_subscript_not_integer);
		return build_broken_expression();
	}

	const item_t element_type = type_get(sx, (size_t)base_type + 1);
	const size_t expr_start = expression_get_location(nd_base).begin;

	node nd = node_create(sx, OP_SLICE);
	node_add_arg(&nd, element_type);				// Тип элемента массива
	node_add_arg(&nd, LVALUE);						// Категория значения вырезки
	node_add_arg(&nd, (item_t)expr_start);			// Начальная позиция вырезки
	node_add_arg(&nd, (item_t)r_loc.end);			// Конечная позиция вырезки
	node_set_child(&nd, nd_base);					// Выражение-операнд
	node_set_child(&nd, nd_index);					// Выражение-индекс

	return nd;
}

node build_call_expression(syntax *const sx, const node *const nd_callee, const node_vector *args
	, const location l_loc, const location r_loc)
{
	if (!node_is_correct(nd_callee))
	{
		return build_broken_expression();
	}

	const item_t operand_type = expression_get_type(nd_callee);
	if (!type_is_function(sx, operand_type))
	{
		semantic_error(sx, l_loc, typecheck_call_not_function);
		return build_broken_expression();
	}

	const size_t expected_args = (size_t)type_get(sx, (size_t)operand_type + 2);
	const size_t actual_args = args != NULL ? node_vector_size(args) : 0;

	if (expected_args != actual_args)
	{
		semantic_error(sx, r_loc, wrong_number_of_params, expected_args, actual_args);
		return build_broken_expression();
	}

	const size_t ref_arg_type = (size_t)operand_type + 3;
	for (size_t i = 0; i < actual_args; i++)
	{
		const node nd_argument = node_vector_get(args, i);
		if (!node_is_correct(&nd_argument))
		{
			return build_broken_expression();
		}

		const item_t expected_type = type_get(sx, ref_arg_type + i);
		const item_t actual_type = expression_get_type(&nd_argument);

		// Несовпадение типов может быть только в случае, когда параметр - double, а аргумент - целочисленный
		if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
		{
			semantic_error(sx, expression_get_location(&nd_argument), typecheck_convert_incompatible);
			return build_broken_expression();
		}
	}

	const item_t return_type = type_get(sx, (size_t)operand_type + 1);
	const size_t expr_start = expression_get_location(nd_callee).begin;

	node nd = node_create(sx, OP_CALL);
	node_add_arg(&nd, return_type);					// Тип возвращамого значения
	node_add_arg(&nd, RVALUE);						// Категория значения вызова
	node_add_arg(&nd, (item_t)expr_start);			// Начальная позиция вызова
	node_add_arg(&nd, (item_t)r_loc.end);			// Конечная позиция вызова
	node_set_child(&nd, nd_callee);					// Операнд вызова
	for (size_t i = 0; i < actual_args; i++)
	{
		const node nd_argument = node_vector_get(args, i);
		node_set_child(&nd, &nd_argument);			// i-ый аргумент вызова
	}

	return nd;
}

node build_member_expression(syntax *const sx, const node *const nd_base, const bool is_arrow, const size_t name
	, const location op_loc, const location id_loc)
{
	if (!node_is_correct(nd_base))
	{
		return build_broken_expression();
	}

	const item_t operand_type = expression_get_type(nd_base);
	item_t struct_type;
	category_t category;

	if (!is_arrow)
	{
		if (!type_is_structure(sx, operand_type))
		{
			semantic_error(sx, op_loc, typecheck_member_reference_struct);
			return build_broken_expression();
		}

		struct_type = operand_type;
		category = expression_is_lvalue(nd_base) ? LVALUE : RVALUE;
	}
	else
	{
		if (!type_is_struct_pointer(sx, operand_type))
		{
			semantic_error(sx, op_loc, typecheck_member_reference_arrow);
			return build_broken_expression();
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
			const size_t expr_start = expression_get_location(nd_base).begin;

			node nd = node_create(sx, OP_SELECT);
			node_add_arg(&nd, member_type);			// Тип значения поля
			node_add_arg(&nd, category);			// Категория значения выборкм
			node_add_arg(&nd, (item_t)expr_start);	// Начальная позиция выборкм
			node_add_arg(&nd, (item_t)id_loc.end);	// Конечная позиция выборкм
			node_add_arg(&nd, member_displ);		// Смещение поля структуры
			node_set_child(&nd, nd_base);			// Выражение-операнд

			return nd;
		}

		member_displ += (item_t)type_size(sx, member_type);
	}

	semantic_error(sx, id_loc, no_member, repr_get_name(sx, name));
	return build_broken_expression();
}

node build_upb_expression(syntax *const sx, const node *const nd_fst, const node *const nd_snd)
{
	if (!node_is_correct(nd_fst) || !node_is_correct(nd_snd))
	{
		return build_broken_expression();
	}

	const item_t fst_type = expression_get_type(nd_fst);
	if (!type_is_integer(fst_type))
	{
		semantic_error(sx, expression_get_location(nd_fst), not_int_in_stanfunc);
		return build_broken_expression();
	}

	const item_t snd_type = expression_get_type(nd_snd);
	if (!type_is_array(sx, snd_type))
	{
		semantic_error(sx, expression_get_location(nd_snd), not_array_in_stanfunc);
		return build_broken_expression();
	}

	node nd = node_create(sx, OP_UPB);
	node_add_arg(&nd, TYPE_INTEGER);				// Тип значения поля
	node_add_arg(&nd, RVALUE);						// Категория значения поля
	node_add_arg(&nd, (item_t)expression_get_location(nd_fst).begin);
	node_add_arg(&nd, (item_t)expression_get_location(nd_snd).begin);
	node_set_child(&nd, nd_fst);					// Выражение-операнд
	node_set_child(&nd, nd_snd);					// Выражение-операнд

	return nd;
}

node build_unary_expression(syntax *const sx, const node *const nd_operand
	, const unary_t op_kind, const location op_loc)
{
	if (!node_is_correct(nd_operand))
	{
		return build_broken_expression();
	}

	const item_t operand_type = expression_get_type(nd_operand);

	item_t result_type = 0;
	category_t category = RVALUE;
	bool is_prefix = true;

	switch (op_kind)
	{
		case UN_POSTINC:
		case UN_POSTDEC:
			is_prefix = false;
			// fallthrough
		case UN_PREINC:
		case UN_PREDEC:
		{
			if (!type_is_arithmetic(operand_type))
			{
				semantic_error(sx, op_loc, typecheck_illegal_increment, op_kind);
				return build_broken_expression();
			}

			if (!expression_is_lvalue(nd_operand))
			{
				semantic_error(sx, op_loc, typecheck_expression_not_lvalue);
				return build_broken_expression();
			}

			result_type = operand_type;
		}
		break;

		case UN_ADDRESS:
		{
			if (!expression_is_lvalue(nd_operand))
			{
				semantic_error(sx, op_loc, typecheck_invalid_lvalue_addrof);
				return build_broken_expression();
			}

			result_type = type_pointer(sx, operand_type);
		}
		break;

		case UN_INDIRECTION:
		{
			if (!type_is_pointer(sx, operand_type))
			{
				semantic_error(sx, op_loc, typecheck_indirection_requires_pointer);
				return build_broken_expression();
			}

			result_type = type_get(sx, (size_t)operand_type + 1);
			category = LVALUE;
		}
		break;

		case UN_ABS:
		case UN_PLUS:
		case UN_MINUS:
		{
			if (!type_is_arithmetic(operand_type))
			{
				semantic_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return build_broken_expression();
			}

			result_type = operand_type;
		}
		break;

		case UN_NOT:
		{
			if (!type_is_integer(operand_type))
			{
				semantic_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return build_broken_expression();
			}

			result_type = TYPE_INTEGER;
		}
		break;

		case UN_LOGNOT:
		{
			if (!type_is_scalar(sx, operand_type))
			{
				semantic_error(sx, op_loc, typecheck_unary_expr, operand_type);
				return build_broken_expression();
			}

			result_type = TYPE_INTEGER;
		}
		break;
	}

	const location loc = is_prefix
		? (location){ op_loc.begin, expression_get_location(nd_operand).end }
		: (location){ expression_get_location(nd_operand).begin, op_loc.end };

	node nd = node_create(sx, OP_UNARY);
	node_add_arg(&nd, result_type);					// Тип значения выражения
	node_add_arg(&nd, category);					// Категория значения выражения
	node_add_arg(&nd, (item_t)loc.begin);			// Начальная позиция выражения
	node_add_arg(&nd, (item_t)loc.end);				// Конечная позиция выражения
	node_add_arg(&nd, op_kind);						// Тип унарного оператора
	node_set_child(&nd, nd_operand);				// Выражение-операнд

	return nd;
}

node build_binary_expression(syntax *const sx, const node *const nd_left, const node *const nd_right
	, const binary_t op_kind, const location op_loc)
{
	if (!node_is_correct(nd_left) || !node_is_correct(nd_right))
	{
		return build_broken_expression();
	}

	const item_t left_type = expression_get_type(nd_left);
	const item_t right_type = expression_get_type(nd_right);

	if (operation_is_assignment(op_kind)
		&& (!expression_is_lvalue(nd_left) || (type_is_floating(right_type) && type_is_integer(left_type))))
	{
		semantic_error(sx, op_loc, unassignable);
		return build_broken_expression();
	}

	item_t result_type;
	if (op_kind == BIN_COMMA)
	{
		result_type = right_type;
	}
	else if (op_kind == BIN_ASSIGN)
	{
		// Особый случай, так как тут могут быть операции с агрегатными типами

		// Несовпадение типов может быть только в случае, когда слева вещественный, а справа целочисленный
		if (left_type != right_type && !(type_is_floating(left_type) && type_is_integer(right_type)))
		{
			semantic_error(sx, op_loc, typecheck_convert_incompatible);
			return build_broken_expression();
		}

		result_type = left_type;
	}
	else
	{
		if (!type_is_arithmetic(left_type) || !type_is_arithmetic(right_type))
		{
			semantic_error(sx, op_loc, typecheck_binary_expr);
			return build_broken_expression();
		}

		if (operation_is_assignment(op_kind) && !expression_is_lvalue(nd_left))
		{
			semantic_error(sx, op_loc, unassignable);
			return build_broken_expression();
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
					semantic_error(sx, op_loc, int_op_for_float);
					return build_broken_expression();
				}

				result_type = TYPE_INTEGER;
			}
			break;

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

	node nd = node_create(sx, OP_BINARY);
	node_add_arg(&nd, result_type);					// Тип значения
	node_add_arg(&nd, RVALUE);						// Категория значения
	node_add_arg(&nd, (item_t)expression_get_location(nd_left).begin);
	node_add_arg(&nd, (item_t)expression_get_location(nd_right).end);
	node_add_arg(&nd, op_kind);						// Вид оператора
	node_set_child(&nd, nd_left);					// Первый операнд
	node_set_child(&nd, nd_right);					// Второй операнд

	return nd;
}

node build_ternary_expression(syntax *const sx, const node *const nd_left, const node *const nd_middle
	, const node *const nd_right, const location op_loc)
{
	if (!node_is_correct(nd_left) || !node_is_correct(nd_middle) || !node_is_correct(nd_right))
	{
		return build_broken_expression();
	}

	const item_t left_type = expression_get_type(nd_left);
	const item_t middle_type = expression_get_type(nd_middle);
	const item_t right_type = expression_get_type(nd_right);

	if (!type_is_scalar(sx, left_type))
	{
		semantic_error(sx, expression_get_location(nd_left), typecheck_statement_requires_scalar);
		return build_broken_expression();
	}

	item_t result_type = middle_type;

	if ((type_is_floating(middle_type) && type_is_integer(right_type))
		|| (type_is_integer(middle_type) && type_is_floating(right_type)))
	{
		result_type = TYPE_FLOATING;
	}
	else if (middle_type != right_type)
	{
		semantic_error(sx, op_loc, typecheck_cond_incompatible_operands);
		return build_broken_expression();
	}

	node nd = node_create(sx, OP_TERNARY);
	node_add_arg(&nd, result_type);					// Тип значения
	node_add_arg(&nd, RVALUE);						// Категория значения
	node_add_arg(&nd, (item_t)expression_get_location(nd_left).begin);
	node_add_arg(&nd, (item_t)expression_get_location(nd_right).begin);
	node_set_child(&nd, nd_left);					// Первый операнд
	node_set_child(&nd, nd_middle);					// Второй операнд
	node_set_child(&nd, nd_right);					// Третий операнд

	return nd;
}

node build_init_list_expression(syntax *const sx, const node_vector *vec, const item_t type
	, const location l_loc, const location r_loc)
{
	const size_t actual_inits = node_vector_size(vec);
	if (actual_inits == 0)
	{
		semantic_error(sx, l_loc, empty_init);
		return build_broken_expression();
	}

	if (type_is_structure(sx, type))
	{
		const size_t expected_inits = (size_t)(type_get(sx, (size_t)type + 2) / 2);

		if (expected_inits != actual_inits)
		{
			semantic_error(sx, r_loc, wrong_init_in_actparam, expected_inits, actual_inits);
			return build_broken_expression();
		}

		size_t ref_next_field = (size_t)type + 3;
		for (size_t i = 0; i < actual_inits; i++)
		{
			const node nd_initializer = node_vector_get(vec, i);
			if (!node_is_correct(&nd_initializer))
			{
				return build_broken_expression();
			}

			const item_t expected_type = type_get(sx, ref_next_field);
			const item_t actual_type = expression_get_type(&nd_initializer);

			// Несовпадение типов может быть только в случае, когда параметр - вещественный, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantic_error(sx, expression_get_location(&nd_initializer), typecheck_convert_incompatible);
				return build_broken_expression();
			}

			ref_next_field += 2;
		}
	}
	else if (type_is_array(sx, type))
	{
		const item_t expected_type = type_get(sx, (size_t)type + 1);
		for (size_t i = 0; i < actual_inits; i++)
		{
			const node nd_initializer = node_vector_get(vec, i);
			if (!node_is_correct(&nd_initializer))
			{
				return build_broken_expression();
			}

			const item_t actual_type = expression_get_type(&nd_initializer);

			// Несовпадение типов может быть только в случае, когда параметр - вещественный, а аргумент - целочисленный
			if (expected_type != actual_type && !(type_is_floating(expected_type) && type_is_integer(actual_type)))
			{
				semantic_error(sx, expression_get_location(&nd_initializer), typecheck_convert_incompatible);
				return build_broken_expression();
			}
		}
	}
	else
	{
		semantic_error(sx, l_loc, wrong_init);
		return build_broken_expression();
	}

	node nd = node_create(sx, OP_LIST);
	node_add_arg(&nd, type);							// Тип возвращамого значения
	node_add_arg(&nd, RVALUE);							// Категория значения вызова
	node_add_arg(&nd, (item_t)l_loc.begin);
	node_add_arg(&nd, (item_t)r_loc.end);
	for (size_t i = 0; i < actual_inits; i++)
	{
		const node nd_initializer = node_vector_get(vec, i);
		node_set_child(&nd, &nd_initializer);			// i-ый инициализатор в списке
	}

	return nd;
}

node build_broken_expression()
{
	return node_load(NULL, SIZE_MAX);
}
