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

#include "expression.h"


inline expression expression_create(const node expr)
{
	return (expression){ .is_valid = true, .nd = expr };
}

inline expression expression_broken(void)
{
	return (expression){ .is_valid = false };
}

inline bool expression_is_valid(const expression expr)
{
   return expr.is_valid;
}

inline item_t expression_get_type(const expression expr)
{
   return node_get_arg(&expr.nd, 0);
}

inline bool expression_is_lvalue(const expression expr)
{
   return node_get_arg(&expr.nd, 1) == LVALUE;
}

inline location expression_get_location(const expression expr)
{
   return (location){ (size_t)node_get_arg(&expr.nd, 2), (size_t)node_get_arg(&expr.nd, 3) };
}

inline node expression_get_node(const expression expr)
{
	return expr.nd;
}
