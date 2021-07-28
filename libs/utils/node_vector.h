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


#ifdef __cplusplus
extern "C" {
#endif


static const size_t NODE_VECTOR_SIZE = 12;


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
EXPORTED node_vector node_vector_create();

/**
 *	Add new node
 *
 *	@param	vec				Node vector
 *	@param	nd				New node
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t node_vector_add(node_vector *const vec, const node *const nd);

/**
 *	Get node
 *
 *	@param	vec				Node vector
 *	@param	index			Index
 *
 *	@return	Expression
 */
EXPORTED node node_vector_get(const node_vector *const vec, const size_t index);

/**
 *	Get node vector size
 *
 *	@param	vec				Node vector
 *
 *	@return	Size of node vector, @c SIZE_MAX on failure
 */
EXPORTED size_t node_vector_size(const node_vector *const vec);

/**
 *	Free allocated memory
 *
 *	@param	vec				Node vector
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int node_vector_clear(node_vector *const vec);

#ifdef __cplusplus
} /* extern "C" */
#endif
