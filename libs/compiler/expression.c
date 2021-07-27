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



inline item_t expression_get_type(const node *const nd)
{
   return node_get_arg(nd, 0);
}

inline bool expression_is_lvalue(const node *const nd)
{
   return node_get_arg(nd, 1) == LVALUE;
}

inline location expression_get_location(const node *const nd)
{
   return (location){ (size_t)node_get_arg(nd, 2), (size_t)node_get_arg(nd, 3) };
}
