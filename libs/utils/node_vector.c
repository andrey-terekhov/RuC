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

#include "node_vector.h"


extern node_vector node_vector_create(void);
extern size_t node_vector_add(node_vector *const vec, const node *const nd);
extern int node_vector_set(node_vector *const vec, const size_t index, const node *const nd);
extern node node_vector_get(const node_vector *const vec, const size_t index);
extern size_t node_vector_size(const node_vector *const vec);
extern bool node_vector_is_correct(const node_vector *const vec);
extern int node_vector_clear(node_vector *const vec);


node_vector node_vector_create(void)
{
    return (node_vector){ .tree = NULL, .nodes = vector_create(NODE_VECTOR_SIZE) };
}
