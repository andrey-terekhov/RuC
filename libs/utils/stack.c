/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev
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

#include "stack.h"


stack stack_create(const size_t alloc)
{
	return vector_create(alloc);
}


int stack_push(stack *const stk, const item_t value)
{
	return vector_add(stk, value) != SIZE_MAX ? 0 : -1;
}

item_t stack_pop(stack *const stk)
{
	return vector_remove(stk);
}

item_t stack_peek(const stack *const stk)
{
	return vector_get(stk, vector_size(stk) - 1);
}


int stack_reset(stack *const stk)
{
	return vector_resize(stk, 0);
}

size_t stack_size(const stack *const stk)
{
	return vector_size(stk);
}

int stack_is_correct(const stack *const stk)
{
	return vector_is_correct(stk);
}


int stack_clear(stack *const stk)
{
	return vector_clear(stk);
}
