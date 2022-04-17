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

#pragma once

#include "tree.h"


#define NODE_VECTOR_SIZE 12


#ifdef __cplusplus
extern "C" {
#endif

/** Node vector structure */
typedef struct node_vector
{
	vector *tree;			/**< Tree */
	vector nodes;			/**< Nodes in AST */
} node_vector;


/**
 *	Create empty node vector
 *
 *	@return	Node vector
 */
node_vector node_vector_create(void);

/**
 *	Add new node
 *
 *	@param	vec				Node vector
 *	@param	nd				New node
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
inline size_t node_vector_add(node_vector *const vec, const node *const nd)
{
	vec->tree = vec->tree != NULL ? vec->tree : nd->tree;
	return vec->tree == nd->tree
		? vector_add(&vec->nodes, (item_t)node_save(nd))
		: SIZE_MAX;
}

/**
 *	Set new node
 *
 *	@param	vec				Node vector
 *	@param	index			Index	
 *	@param	nd				New node
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int node_vector_set(node_vector *const vec, const size_t index, const node *const nd)
{
	return vec->tree == nd->tree
		? vector_set(&vec->nodes, index, (item_t)node_save(nd))
		: -1;
}

/**
 *	Get node by index
 *
 *	@param	vec				Node vector
 *	@param	index			Index
 *
 *	@return	Expression
 */
inline node node_vector_get(const node_vector *const vec, const size_t index)
{
	return node_load(vec->tree, (size_t)vector_get(&vec->nodes, index));
}

/**
 *	Get node vector size
 *
 *	@param	vec				Node vector
 *
 *	@return	Size of node vector, @c SIZE_MAX on failure
 */
inline size_t node_vector_size(const node_vector *const vec)
{
	return vec == NULL ? 0 : vector_size(&vec->nodes);
}

/**
 *	Check that vector is correct
 *
 *	@param	vec				Node vector
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool node_vector_is_correct(const node_vector *const vec)
{
	return vec != NULL && vector_is_correct(vec->tree) && vector_is_correct(&vec->nodes);
}

/**
 *	Free allocated memory
 *
 *	@param	vec				Node vector
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int node_vector_clear(node_vector *const vec)
{
	return vector_clear(&vec->nodes);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
