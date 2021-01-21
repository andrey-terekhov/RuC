/*
 *	Copyright 2021 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "dll.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Vector structure */
typedef struct vector
{
	int64_t *array;				/**< Vector array */
	size_t size;				/**< Size of vector */
	size_t size_alloc;			/**< Allocated size of vector */
} vector;


/**
 *	Create new vector
 *
 *	@param	values			Initializer of values count
 *
 *	@return	Vector structure
 */
EXPORTED vector vector_create(const size_t values);


/**
 *	Add new value
 *
 *	@param	vec				Vector structure
 *	@param	value			Value
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t map_add(vector *const vec, const int64_t value);

/**
 *	Set new value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *	@param	value			New value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int vector_set(vector *const vec, const size_t index, const int64_t value);

/**
 *	Get value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *
 *	@return	Value, @c LLONG_MAX on failure
 */
EXPORTED int64_t vector_get(const vector *const vec, const size_t index);


/**
 *	Get vector size
 *
 *	@param	vec				Vector structure
 *
 *	@return	Size of vector, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_size(vector *const vec);

/**
 *	Check that vector is correct
 *
 *	@param	vec				Vector structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED int vector_is_correct(const vector *const vec);


/**
 *	Free allocated memory
 *
 *	@param	vec				Vector structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int vector_clear(vector *const vec);

#ifdef __cplusplus
} /* extern "C" */
#endif
