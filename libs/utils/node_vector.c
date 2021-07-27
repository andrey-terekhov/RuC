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

#include "node_vector.h"


static const size_t NODE_VECTOR_SIZE = 12;


node_vector node_vector_create()
{
	return (node_vector){ .tree = NULL, .nodes = vector_create(NODE_VECTOR_SIZE) };
}

inline size_t node_vector_add(node_vector *const vec, const node *const nd)
{
	vec->tree = vec->tree != NULL ? vec->tree : nd->tree;
	return vec->tree == nd->tree
		? vector_add(&vec->nodes, (item_t)node_save(nd))
		: SIZE_MAX;
}

node node_vector_get(const node_vector *const vec, const size_t index)
{
	return node_load(vec->tree, (size_t)vector_get(&vec->nodes, index));
}

size_t node_vector_size(const node_vector *const vec)
{
	return vector_size(&vec->nodes);
}

int node_vector_clear(node_vector *const vec)
{
	return vector_clear(&vec->nodes);
}
