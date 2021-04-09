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
EXPORTED stack stack_create(const size_t alloc);


/**
 *	Push new value
 *
 *	@param	stk				Stack structure
 *	@param	value			Value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int stack_push(stack *const stk, const item_t value);

/**
 *	Pop value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t stack_pop(stack *const stk);

/**
 *	Peek value
 *
 *	@param	stk				Stack structure
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t stack_peek(const stack *const stk);


/**
 *	Reset stack size to zero
 *
 *	@param	stk				Stack structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int stack_reset(stack *const stk);

/**
 *	Get stack size
 *
 *	@param	stk				Stack structure
 *
 *	@return	Size of stack, @c SIZE_MAX on failure
 */
EXPORTED size_t stack_size(const stack *const stk);

/**
 *	Check that stack is correct
 *
 *	@param	stk				Stack structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED int stack_is_correct(const stack *const stk);


/**
 *	Free allocated memory
 *
 *	@param	stk				Stack structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int stack_clear(stack *const stk);

#ifdef __cplusplus
} /* extern "C" */
#endif
