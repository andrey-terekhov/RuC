/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

#include <limits.h>
#include <stddef.h>
#include "vector.h"


//#define BUFFERING


#ifdef __cplusplus
extern "C" {
#endif

/** Tree node */
typedef struct node
{
	vector *tree;			/**< Tree reference */
	size_t index;			/**< Node index */
} node;


/**
 *	Get tree root node
 *
 *	@param	tree		Tree table
 *
 *	@return	Root node
 */
EXPORTED node node_get_root(vector *const tree);

/**
 *	Get child from node by index
 *
 *	@param	nd			Parent node
 *	@param	index		Child number
 *
 *	@return	Child node
 */
EXPORTED node node_get_child(node *const nd, const size_t index);

/**
 *	Get parent of node
 *
 *	@param	nd			Parent node
 *
 *	@return	Parent node
 */
EXPORTED node node_get_parent(node *const nd);


/**
 *	Get type of node
 *
 *	@param	nd			Node structure
 *
 *	@return	Node type, @c ITEM_MAX on failure
 */
EXPORTED item_t node_get_type(const node *const nd);

/**
 *	Get amount of arguments
 *
 *	@param	nd			Node structure
 *
 *	@return	Amount of arguments
 */
EXPORTED size_t node_get_argc(const node *const nd);

/**
 *	Get argument from node by index
 *
 *	@param	nd			Node structure
 *	@param	index		Argument number
 *
 *	@return	Argument, @c ITEM_MAX on failure
 */
EXPORTED item_t node_get_arg(const node *const nd, const size_t index);

/**
 *	Get amount of children
 *
 *	@param	nd			Node structure
 *
 *	@return	Amount of children
 */
EXPORTED size_t node_get_amount(const node *const nd);


/**
 *	Get next node from tree traversal in pre-order (NLR)
 *
 *	@param	nd			Current node
 *
 *	@return	Next node
 */
EXPORTED node node_get_next(node *const nd);

/**
 *	Set next node to the same one from tree traversal in pre-order (NLR)
 *
 *	@param	nd			Current node
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int node_set_next(node *const nd);


/**
 *	Add child node
 *
 *	@param	nd			Current node
 *	@param	type		Child node type
 *
 *	@return	Child node
 */
EXPORTED node node_add_child(node *const nd, const item_t type);

/**
 *	Set node type
 *
 *	@param	nd			Node structure
 *	@param	type		Node type
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on trying to reset the root node
 */
EXPORTED int node_set_type(node *const nd, const item_t type);

/**
 *	Add new node argument
 *
 *	@param	nd			Node structure
 *	@param	arg			Node argument
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on node with children
 */
EXPORTED int node_add_arg(node *const nd, const item_t arg);

/**
 *	Set node argument by index
 *
 *	@param	nd			Node structure
 *	@param	index		Argument number
 *	@param	arg			Node argument
 *
 *	@return	@c  0 on success, @c -1 on failure
 */
EXPORTED int node_set_arg(node *const nd, const size_t index, const item_t arg);


/**
 *	Copy source node to destination
 *
 *	@param	dest		Destination node
 *	@param	src			Source node
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int node_copy(node *const dest, const node *const src);

/**
 *	Save internal node index
 *
 *	@param	nd			Node structure
 *
 *	@return	Internal index, @c SIZE_MAX on failure
 */
EXPORTED size_t node_save(const node *const nd);

/**
 *	Rebuild node by internal index
 *
 *	@param	tree		Tree table
 *	@param	index		Internal index
 *
 *	@return	Rebuilt node
 */
EXPORTED node node_load(vector *const tree, const size_t index);

/**
 *	Change only node order
 *
 *	@param	fst			First parent node
 *	@param	fst_index	First child number
 *	@param	snd			Second parent node
 *	@param	snd_index	Second child number
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int node_order(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index);

/**
 *	Swap two nodes with children
 *
 *	@param	fst			First parent node
 *	@param	fst_index	First child number
 *	@param	snd			Second parent node
 *	@param	snd_index	Second child number
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int node_swap(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index);

/**
 *	Remove child node by index
 *
 *	@param	nd			Parrent node
 *	@param	index		Child number
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int node_remove(node *const nd, const size_t index);

/**
 *	Check that node is correct
 *
 *	@param	nd			Node structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED int node_is_correct(const node *const nd);

#ifdef __cplusplus
} /* extern "C" */
#endif
