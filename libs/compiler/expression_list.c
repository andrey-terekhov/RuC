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
	return (expression_list){ .nodes = vector_create(12) };
}

void expression_list_add(expression_list *exprs, const node *const nd_expr)
{
	exprs->tree = nd_expr->tree;	// TODO: node_get_tree()
	vector_add(&exprs->nodes, (item_t)node_save(nd_expr));
}

node expression_list_get(const expression_list *exprs, const size_t index)
{
	return node_load(exprs->tree, (size_t)vector_get(&exprs->nodes, index));
}

size_t expression_list_size(const expression_list *exprs)
{
	return vector_size(&exprs->nodes);
}

int expression_list_clear(expression_list *exprs)
{
	return vector_clear(&exprs->nodes);
}
