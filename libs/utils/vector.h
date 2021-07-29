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

#include "dll.h"
#include "item.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Vector structure */
typedef struct vector
{
	item_t *array;				/**< Vector array */
	size_t size;				/**< Size of vector */
	size_t size_alloc;			/**< Allocated size of vector */
} vector;


/**
 *	Create new vector
 *
 *	@param	alloc			Initializer of allocated size
 *
 *	@return	Vector structure
 */
EXPORTED vector vector_create(const size_t alloc);


/**
 *	Add new value
 *
 *	@param	vec				Vector structure
 *	@param	value			Value
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_add(vector *const vec, const item_t value);

/**
 *	Add new double value
 *
 *	@param	vec				Vector structure
 *	@param	value			Double value
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_add_double(vector *const vec, const double value);

/**
 *	Add new 64-bit value
 *
 *	@param	vec				Vector structure
 *	@param	value			64-bit value
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_add_int64(vector *const vec, const int64_t value);


/**
 *	Set new value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *	@param	value			New value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int vector_set(vector *const vec, const size_t index, const item_t value);

/**
 *	Set new double value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *	@param	value			New double value
 *
 *	@return	Number of used elements, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_set_double(vector *const vec, const size_t index, const double value);

/**
 *	Set new 64-bit value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *	@param	value			New 64-bit value
 *
 *	@return	Number of used elements, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_set_int64(vector *const vec, const size_t index, const int64_t value);


/**
 *	Get value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t vector_get(const vector *const vec, const size_t index);

/**
 *	Get double value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *
 *	@return	Value, @c DBL_MAX on failure
 */
EXPORTED double vector_get_double(const vector *const vec, const size_t index);

/**
 *	Get 64-bit value
 *
 *	@param	vec				Vector structure
 *	@param	index			Index
 *
 *	@return	Value, @c LLONG_MAX on failure
 */
EXPORTED int64_t vector_get_int64(const vector *const vec, const size_t index);


/**
 *	Remove last value
 *
 *	@param	vec				Vector structure
 *
 *	@return	Deleted value, @c ITEM_MAX on failure
 */
EXPORTED item_t vector_remove(vector *const vec);

/**
 *	Remove last double value
 *
 *	@param	vec				Vector structure
 *
 *	@return	Deleted value, @c DBL_MAX on failure
 */
EXPORTED double vector_remove_double(vector *const vec);

/**
 *	Remove last 64-bit value
 *
 *	@param	vec				Vector structure
 *
 *	@return	Deleted value, @c LLONG_MAX on failure
 */
EXPORTED int64_t vector_remove_int64(vector *const vec);


/**
 *	Increase vector size
 *
 *	@param	vec				Vector structure
 *	@param	size			Size to increase
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int vector_increase(vector *const vec, const size_t size);

/**
 *	Change vector size
 *
 *	@param	vec				Vector structure
 *	@param	size			New vector size
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int vector_resize(vector *const vec, const size_t size);

/**
 *	Get vector size
 *
 *	@param	vec				Vector structure
 *
 *	@return	Size of vector, @c SIZE_MAX on failure
 */
EXPORTED size_t vector_size(const vector *const vec);

/**
 *	Check that vector is correct
 *
 *	@param	vec				Vector structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool vector_is_correct(const vector *const vec);


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
