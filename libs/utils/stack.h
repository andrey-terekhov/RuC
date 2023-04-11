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

#pragma once

#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Stack structure */
typedef vector stack;


/**
 *	Create new stack
 *
 *	@param	alloc			Initializer of allocated size
 *
 *	@return	Stack structure
 */
inline stack stack_create(const size_t alloc)
{
    return vector_create(alloc);
}


/**
 *	Push new value
 *
 *	@param	stk				Stack structure
 *	@param	value			Value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int stack_push(stack *const stk, const item_t value)
{
    return vector_add(stk, value) != SIZE_MAX ? 0 : -1;
}

/**
 *	Push new double value
 *
 *	@param	stk				Stack structure
 *	@param	value			Double value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int stack_push_double(stack *const stk, const double value)
{
    return vector_add_double(stk, value) != SIZE_MAX ? 0 : -1;
}

/**
 *	Push new 64-bit value
 *
 *	@param	stk				Stack structure
 *	@param	value			64-bit value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int stack_push_int64(stack *const stk, const int64_t value)
{
    return vector_add_int64(stk, value) != SIZE_MAX ? 0 : -1;
}


/**
 *	Pop value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
inline item_t stack_pop(stack *const stk)
{
    return vector_remove(stk);
}

/**
 *	Pop double value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c DBL_MAX on failure
 */
inline double stack_pop_double(stack *const stk)
{
    return vector_remove_double(stk);
}

/**
 *	Pop 64-bit value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c LLONG_MAX on failure
 */
inline int64_t stack_pop_int64(stack *const stk)
{
    return vector_remove_int64(stk);
}


/**
 *	Peek value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
inline item_t stack_peek(const stack *const stk)
{
    return vector_get(stk, vector_size(stk) - 1);
}

/**
 *	Peek double value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c DBL_MAX on failure
 */
inline double stack_peek_double(const stack *const stk)
{
    return vector_get_double(stk, vector_size(stk) - DOUBLE_SIZE);
}
/**
 *	Peek 64-bit value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c LLONG_MAX on failure
 */
inline int64_t stack_peek_int64(const stack *const stk)
{
    return vector_get_int64(stk, vector_size(stk) - INT64_SIZE);
}


/**
 *	Reset stack size to zero
 *
 *	@param	stk				Stack structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int stack_reset(stack *const stk)
{
    return vector_resize(stk, 0);
}

/**
 *	Get stack size
 *
 *	@param	stk				Stack structure
 *
 *	@return	Size of stack, @c SIZE_MAX on failure
 */
inline size_t stack_size(const stack *const stk)
{
    return vector_size(stk);
}

/**
 *	Check that stack is correct
 *
 *	@param	stk				Stack structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool stack_is_correct(const stack *const stk)
{
    return vector_is_correct(stk);
}


/**
 *	Free allocated memory
 *
 *	@param	stk				Stack structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int stack_clear(stack *const stk)
{
    return vector_clear(stk);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
