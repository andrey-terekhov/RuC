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

#include "expression_list.h"


expression_list expression_list_create(void)
{
	return (expression_list){ .flags_is_valid = vector_create(12), .saved_nodes = vector_create(12) };
}

void expression_list_add(expression_list *exprs, const expression expr)
{
	const bool is_valid = expression_is_valid(expr);
	const node nd_expr = expression_get_node(expr);
	exprs->tree = nd_expr.tree;	// Пока ждем node_get_tree()
	vector_add(&exprs->flags_is_valid, is_valid);
	vector_add(&exprs->saved_nodes, (item_t)node_save(&nd_expr));
}

expression expression_list_get(const expression_list *exprs, const size_t index)
{
	bool is_valid = vector_get(&exprs->flags_is_valid, index) != 0;

	if (is_valid)
	{
		node nd = node_load(exprs->tree, (size_t)vector_get(&exprs->saved_nodes, index));
		return expression_create(nd);
	}

	return expression_broken();
}

size_t expression_list_size(const expression_list *exprs)
{
	return vector_size(&exprs->flags_is_valid);
}

int expression_list_clear(expression_list *exprs)
{
	return vector_clear(&exprs->flags_is_valid)
		|| vector_clear(&exprs->saved_nodes);
}
