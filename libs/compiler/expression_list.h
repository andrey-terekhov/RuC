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

#include "expression.h"
#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Expression list structure */
typedef struct expression_list
{
	vector *tree;			/**< Tree */
	vector flags_is_valid;	/**< Set if is valid */
	vector saved_nodes;		/**< Node in AST */
} expression_list;

/**
 *	Create empty expression list
 *
 *	@return	Expression list
 */
expression_list expression_list_create(void);

/**
 *	Add new expression
 *
 *	@param	exprs			Expression list
 *	@param	expr			Expression
 */
void expression_list_add(expression_list *exprs, const expression expr);

/**
 *	Get expression
 *
 *	@param	exprs			Expression list
 *	@param	index			Index
 *
 *	@return	Expression
 */
expression expression_list_get(const expression_list *exprs, const size_t index);

/**
 *	Get expression list size
 *
 *	@param	exprs			Expression list
 *
 *	@return	Size of list, @c SIZE_MAX on failure
 */
size_t expression_list_size(const expression_list *exprs);

/**
 *	Free allocated memory
 *
 *	@param	exprs			Expression list
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
int expression_list_clear(expression_list *exprs);

#ifdef __cplusplus
extern "C" }
#endif
