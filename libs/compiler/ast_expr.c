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

static void node_set_child(node *const parent, node *const child)
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


expression expr_broken()
{
	return (expression){ .is_valid = false };
}

expression identifier_expression(syntax *const sx, const item_t identifier, const location_t location)
{
	if (identifier == ITEM_MAX)
	{
		const size_t representation = (size_t)ident_get_repr(sx, (size_t)identifier);
		semantics_error(sx, location, undeclared_var_use, repr_get_name(sx, representation));
		return expr_broken();
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
