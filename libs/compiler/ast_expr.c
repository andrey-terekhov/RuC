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

#include "ast.h"
#include <string.h>

/** Return valid expression from AST node */
static expression expr(const node expr_node, const location_t location)
{
	return (expression){ .is_valid = true, .location = location, .nd = expr_node };
}

static node create_node(syntax *const sx, operation_t type)
{
	return node_add_child(&sx->nd, type);
}

static void node_set_child(const node *const parent, const node *const child)
{
	node temp = node_add_child(parent, OP_NOP);
	node_swap(child, &temp);
	node_remove(&temp);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


expression invalid_expression()
{
	return (expression){ .is_valid = false };
}

expression identifier_expression(syntax *const sx, const item_t identifier, const location_t location)
{
	if (identifier == ITEM_MAX)
	{
		const size_t representation = (size_t)ident_get_repr(sx, (size_t)identifier);
		semantics_error(sx, location, undeclared_var_use, repr_get_name(sx, representation));
		return invalid_expression();
	}

	const item_t type = ident_get_type(sx, (size_t)identifier);
	const category_t category = type_is_function(sx, type) ? RVALUE : LVALUE;

	node identifier_node = create_node(sx, OP_IDENTIFIER);
	node_add_arg(&identifier_node, type);					// Тип значения идентификатора
	node_add_arg(&identifier_node, category);				// Категория значения идентификатора
	node_add_arg(&identifier_node, identifier);				// Индекс в таблице идентификаторов

	return expr(identifier_node, location);
}

expression integer_literal_expression(syntax *const sx, const int32_t value, const location_t location)
{
	node constant_node = create_node(sx, OP_CONSTANT);
	node_add_arg(&constant_node, TYPE_INTEGER);				// Тип значения константы
	node_add_arg(&constant_node, RVALUE);					// Категория значения константы
	node_add_arg(&constant_node, value);					// Значение константы

	return expr(constant_node, location);
}

expression floating_literal_expression(syntax *const sx, const double value, const location_t location)
{
	item_t temp;
	memcpy(&temp, &value, sizeof(double));

	node constant_node = create_node(sx, OP_CONSTANT);
	node_add_arg(&constant_node, TYPE_FLOATING);			// Тип значения константы
	node_add_arg(&constant_node, RVALUE);					// Категория значения константы
	node_add_arg(&constant_node, temp);						// Значение константы

	return expr(constant_node, location);
}

expression string_literal_expression(syntax *const sx, const vector value, const location_t location)
{
	const item_t type = type_array(sx, TYPE_INTEGER);

	node string_node = create_node(sx, OP_STRING);
	node_add_arg(&string_node, type);						// Тип строки
	node_add_arg(&string_node, LVALUE);						// Категория значения строки
	for (size_t i = 0, length = vector_size(&value); i < length; i++)
	{
		node_add_arg(&string_node, vector_get(&value, i));	// i-ый символ строки
	}

	return expr(string_node, location);
}

expression subscripting_expression(syntax *const sx, const expression base, const expression index
								  , const location_t l_square_location, const location_t r_square_location)
{
	if (!base.is_valid || !index.is_valid)
	{
		return invalid_expression();
	}

	const item_t operand_type = node_get_arg(&base.nd, 0);
	if (!type_is_array(sx, operand_type))
	{
		semantics_error(sx, l_square_location, typecheck_subscript_value);
		return invalid_expression();
	}

	if (!type_is_integer(node_get_arg(&index.nd, 0)))
	{
		semantics_error(sx, index.location, typecheck_subscript_not_integer);
		return invalid_expression();
	}

	const item_t element_type = type_get(sx, (size_t)operand_type + 1);

	node slice_node = create_node(sx, OP_SLICE);
	node_add_arg(&slice_node, element_type);				// Тип элемента массива
	node_add_arg(&slice_node, LVALUE);						// Категория значения вырезки
	node_set_child(&slice_node, &base.nd);					// Выражение-операнд
	node_set_child(&slice_node, &index.nd);					// Выражение-индекс

	return expr(slice_node, (location_t){ base.loc.begin, r_square_location.end });
}


expression member_expression(syntax *const sx, const expression base, const bool is_arrow, const size_t identifier
							, const location_t operator_loc, const location_t identifier_loc)
{
	if (!base.is_valid)
	{
		return invalid_expression();
	}

	const item_t operand_type = node_get_arg(&operand.nd, 0);
	item_t struct_type;
	category_t category;

	if (!is_arrow)
	{
		if (!type_is_structure(sx, operand_type))
		{
			semantics_error(sx, operator_location, typecheck_member_reference_struct);
			return invalid_expression();
		}

		struct_type = operand_type;
		category = (category_t)node_get_arg(&base.nd, 1);
	}
	else
	{
		if (!type_is_struct_pointer(sx, operand_type))
		{
			semantics_error(sx, operator_location, typecheck_member_reference_arrow);
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
		if (member_name == (size_t)type_get(sx, (size_t)struct_type + 4 + i))
		{
			node select_node = create_node(sx, OP_SELECT);
			node_add_arg(&select_node, member_type);	// Тип значения поля
			node_add_arg(&select_node, category);		// Категория значения поля
			node_add_arg(&select_node, member_displ);	// Смещение поля структуры
			node_set_child(&select_node, &operand.nd);	// Выражение-операнд

			return expr(select_node, (location_t){ base.location.begin, member_location.end });
		}

		member_displ += (item_t)type_size(sx, member_type);
	}

	semantics_error(sx, member_location, no_member, repr_get_name(sx, member_name));
	return invalid_expression();
}
