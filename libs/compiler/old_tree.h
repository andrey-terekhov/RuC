/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

//#define OLD_TREE

#ifndef OLD_TREE
	#include "tree.h"
#else
	#include <limits.h>
	#include <stddef.h>
	#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Tree node */
typedef struct node
{
	vector *tree;			/**< Tree reference */
	size_t type;			/**< Node type */

	size_t argv;			/**< Reference to arguments */
	size_t argc;			/**< Number of arguments */

	size_t children;		/**< Reference to children */
	size_t amount;			/**< Amount of children */
} node;


/**
 *	Get tree root node
 *
 *	@param	tree		Tree table
 *
 *	@return	Root node
 */
node node_get_root(vector *const tree);

/**
 *	Get child from node by index
 *
 *	@param	nd			Parent node
 *	@param	index		Child number
 *
 *	@return	Child node
 */
node node_get_child(node *const nd, const size_t index);


/**
 *	Get amount of children
 *
 *	@param	nd			Node structure
 *
 *	@return	Amount of children
 */
size_t node_get_amount(const node *const nd);

/**
 *	Get type of node
 *
 *	@param	nd			Node structure
 *
 *	@return	Node type, @c ITEM_MAX on failure
 */
item_t node_get_type(const node *const nd);

/**
 *	Get argument from node by index
 *
 *	@param	nd			Node structure
 *	@param	index		Argument number
 *
 *	@return	Argument, @c ITEM_MAX on failure
 */
item_t node_get_arg(const node *const nd, const size_t index);


/**
 *	Get next node from tree traversal in pre-order (NLR)
 *
 *	@param	nd			Current node
 *
 *	@return	Next node
 */
node node_get_next(node *const nd);

/**
 *	Set next node to the same one from tree traversal in pre-order (NLR)
 *
 *	@param	nd			Current node
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int node_set_next(node *const nd);


/**
 *	Add child node
 *
 *	@param	nd			Current node
 *	@param	type		Child node type
 *
 *	@return	Child node
 */
node node_add_child(node *const nd, const item_t type);

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
int node_set_type(node *const nd, const item_t type);

/**
 *	Add new node argument
 *
 *	@param	nd			Node structure
 *	@param	arg			Node argument
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on root,
 *			@c -3 on node with children
 */
int node_add_arg(node *const nd, const item_t arg);

/**
 *	Set node argument by index
 *
 *	@param	nd			Node structure
 *	@param	index		Argument number
 *	@param	arg			Node argument
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on root
 */
int node_set_arg(node *const nd, const size_t index, const item_t arg);


/**
 *	Copy source node to destination
 *
 *	@param	dest		Destination node
 *	@param	src			Source node
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int node_copy(node *const dest, const node *const src);

/**
 *	Save internal node index
 *
 *	@param	nd			Node structure
 *
 *	@return	Internal index, @c SIZE_MAX on failure
 */
size_t node_save(const node *const nd);

/**
 *	Rebuild node by internal index
 *
 *	@param	tree		Tree table
 *	@param	index		Internal index
 *
 *	@return	Rebuilt node
 */
node node_load(vector *const tree, const size_t index);

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
int node_order(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index);

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
int node_swap(node *const fst, const size_t fst_index, node *const snd, const size_t snd_index);

/**
 *	Remove child node by index
 *
 *	@param	nd			Parrent node
 *	@param	index		Child number
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int node_remove(node *const nd, const size_t index);

/**
 *	Check that node is correct
 *
 *	@param	nd			Node structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int node_is_correct(const node *const nd);


/**
 *	Test tree building
 *
 *	@param	tree		Tree table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test(vector *const tree);

/**
 *	Test tree building by node_get_next
 *
 *	@param	tree		Tree table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test_next(vector *const tree);

/**
 *	Test tree building from tree traversal
 *
 *	@param	tree		Tree table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test_recursive(vector *const tree);

/**
 *	Test tree copying by new interface
 *
 *	@param	tree		Tree table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test_copy(vector *const tree);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
