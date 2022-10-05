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

#include <string.h> 
#include "builder.h"
#include "AST.h"

#pragma once

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
node create_printf_node_by_vector(builder *bldr, const vector *str, node_vector *args, location l_loc, location r_loc);

/**
 * Create nodes in AST for complicated type -- structure, array (not string) or pointer
 * 
 * @param   bldr            AST builder
 * @param   argument        Source node
 * @param   l_loc           Left source location
 * @param   r_loc           Right source location
 * @param   tab_deep        Tabulation deep for structure printig
 * 
 * @return  Created nodes
 */
node create_complicated_type_str(builder *bldr, node *argument, location l_loc, location r_loc, size_t tab_deep);

/**
 * Create specifier string for simple type -- int, bool, char, float, string, null pointer
 * 
 * @param   type            Type
 * 
 * @return  Created specifier 
 */
const char *create_simple_type_str(item_t type); 

/**
 * Add string to the end a vector
 * 
 * @param   dst             Destination vector
 * @param   src             Source string
 * 
 * @return @c 1 on success, @c 0 on failure
 */
bool vector_add_str(vector *dst, const char *src);