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

#include <stdbool.h>
#include <stddef.h>
#include "dll.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Strings structure */
typedef struct strings
{
	char *all_strings;				/**< Strings storage */
	size_t all_strings_size;		/**< Size of strings storage */
	size_t all_strings_alloc;		/**< Allocated size of strings storage */

	size_t *indexes;				/**< Indexes array */
	size_t indexes_size;			/**< Size of indexes */
	size_t indexes_alloc;			/**< Allocated size of indexes */
} strings;


/**
 *	Create new strings structure
 *
 *	@param	alloc			Initializer of allocated size
 *
 *	@return	Vector structure
 */
EXPORTED strings strings_create(const size_t alloc);


/**
 *	Add new value
 *
 *	@param	vec				Strings structure
 *	@param	value			Value
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t strings_add(strings *const vec, const char *const value);

/**
 *	Get string
 *
 *	@param	vec				Strings structure
 *	@param	index			Index
 *
 *	@return	String, @c NULL on failure
 */
EXPORTED const char *strings_get(const strings *const vec, const size_t index);

/**
 *	Remove last string
 *
 *	@param	vec				Strings structure
 *
 *	@return	Deleted string, @c NULL on failure
 */
EXPORTED strings strings_remove(strings *const vec);


/**
 *	Get strings structure size
 *
 *	@param	vec				Strings structure
 *
 *	@return	Size of strings structure, @c SIZE_MAX on failure
 */
EXPORTED size_t strings_size(const strings *const vec);

/**
 *	Check that strings structure is correct
 *
 *	@param	vec				Strings structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool strings_is_correct(const strings *const vec);


/**
 *	Free allocated memory
 *
 *	@param	vec				Strings structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int strings_clear(strings *const vec);

#ifdef __cplusplus
} /* extern "C" */
#endif
