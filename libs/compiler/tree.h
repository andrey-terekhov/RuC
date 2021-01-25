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

#include "syntax.h"
#include <limits.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Tree reference */
typedef struct tree
{
	int *array;				/**< Reference to table */
	size_t *size;			/**< Reference to size */
} tree;

/** Tree node */
typedef struct node
{
	tree tree;				/**< Tree reference */
	size_t type;			/**< Node type */

	size_t argv;			/**< Reference to arguments */
	size_t argc;			/**< Number of arguments */

	size_t children;		/**< Reference to children */
	size_t amount;			/**< Amount of children */
} node;


/**
 *	Get tree root node
 *
 *	@param	sx		Syntax structure
 *
 *	@return	Root node
 */
node node_get_root(syntax *const sx);

/**
 *	Get child from node by index
 *
 *	@param	nd		Parent node
 *	@param	index	Child number
 *
 *	@return	Child node
 */
node node_get_child(node *const nd, const size_t index);

/**
 *	Get next node from tree traversal in pre-order (NLR)
 *
 *	@param	nd		Current node
 *
 *	@return	Next node
 */
node node_get_next(node *const nd);


/**
 *	Get amount of children
 *
 *	@param	nd		Node structure
 *
 *	@return	Amount of children
 */
size_t node_get_amount(const node *const nd);

/**
 *	Get type of node
 *
 *	@param	nd		Node structure
 *
 *	@return	Node type, @c INT_MAX on failure
 */
int node_get_type(const node *const nd);

/**
 *	Get argument from node by index
 *
 *	@param	nd		Node structure
 *	@param	index	Argument number
 *
 *	@return	Argument, @c INT_MAX on failure
 */
int node_get_arg(const node *const nd, const size_t index);


/**
 *	Set node type
 *
 *	@param	nd		Node structure
 *	@param	type	Node type
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on trying to reset the root node,
 *			@c -3 on trying to set non-empty node
 */
int node_set_type(node *const nd, const int type);

/**
 *	Add new node argument
 *
 *	@param	nd		Node structure
 *	@param	arg		Node argument
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on root or types not set node,
 *			@c -3 on node with children
 */
int node_add_arg(node *const nd, const int arg);

/**
 *	Set node argument by index
 *
 *	@param	nd		Node structure
 *	@param	index	Argument number
 *	@param	arg		Node argument
 *
 *	@return	@c  0 on success,
 *			@c -1 on failure,
 *			@c -2 on root or types not set node
 */
int node_set_arg(node *const nd, const size_t index, const int arg);

/**
 *	Set child node
 *
 *	@param	nd		Current node
 *
 *	@return	Child node
 */
node node_set_child(node *const nd);


/**
 *	Check that node is correct
 *
 *	@param	nd		Node structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
int node_is_correct(const node *const nd);


/**
 *	Test tree building
 *
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test(syntax *const sx);

/**
 *	Test tree building by node_get_next
 *
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test_next(syntax *const sx);

/**
 *	Test tree building from tree traversal
 *
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test_recursive(syntax *const sx);

/**
 *	Test tree copying by new interface
 *
 *	@param	sx		Syntax structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int tree_test_copy(syntax *const sx);

#ifdef __cplusplus
} /* extern "C" */
#endif
