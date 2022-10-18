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

#include <string.h> 
#include "builder.h"
#include "AST.h"


/**
 * Create node for printf function with string as a vector
 * 
 * @param   bldr            AST builder
 * @param   str             String (as a vector)
 * @param   args            Argument list 
 * @param   l_loc           Left source location
 * @param   r_loc           Right source location
 * 
 * @return  Created node
 */
node create_printf_node_by_vector(builder *bldr, const vector *const str, const node_vector *const args, const location l_loc, const location r_loc);

/**
 * Add nodes for complicated type to node vector
 * 
 * @param   bldr            AST builder
 * @param   argument        Source node
 * @param   stmts           Node vector to add
 * @param   l_loc           Left source location
 * @param   r_loc           Right source location
 * @param   tab_deep        Tabulation deep for structure printing
 * 
 * @return	Node vector size, @c SIZE_MAX on failure
 */
size_t create_complicated_type_str(builder *bldr, node *const argument, node_vector *stmts
    , const location l_loc, const location r_loc, const size_t tab_deep);

/**
 * Create specifier string for simple type -- int, bool, char, float, string, null pointer
 * 
 * @param   type            Type
 * 
 * @return  Created specifier 
 */
const char *create_simple_type_str(const item_t type); 

/**
 * Add string to the end a vector
 * 
 * @param   dst             Destination vector
 * @param   src             Source string
 * 
 * @return @c 1 on success, @c 0 on failure
 */
bool vector_add_str(vector *dst, const char *src);

/**
 * Check if the type is complicated (structure, array (not string), pointer or boolean)
 * 
 * @param   type            Type
 * 
 * @return @c 1 on success, @c 0 on failure
*/
bool type_is_complicated(const syntax *const sx, const item_t type);