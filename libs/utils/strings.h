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
#include "utf8.h"
#include "vector.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Strings vector */
typedef struct strings
{
	char *all_strings;				/**< Strings storage */
	size_t all_strings_size;		/**< Size of strings storage */
	size_t all_strings_alloc;		/**< Allocated size of strings storage */

	size_t *indexes;				/**< Indexes array */
	size_t indexes_size;			/**< Size of indexes array */
	size_t indexes_alloc;			/**< Allocated size of indexes array */
} strings;


/**
 *	Create new strings vector
 *
 *	@param	alloc			Initializer of allocated size
 *
 *	@return	Strings vector
 */
EXPORTED strings strings_create(const size_t alloc);


/**
 *	Add new string
 *
 *	@param	vec				Strings vector
 *	@param	str				String
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t strings_add(strings *const vec, const char *const str);

/**
 *	Add new UTF-8 string
 *
 *	@param	vec				Strings vector
 *	@param	str				UTF-8 string
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t strings_add_by_utf8(strings *const vec, const char32_t *const str);

/**
 *	Add new dynamic UTF-8 string
 *
 *	@param	vec				Strings vector
 *	@param	str				Dynamic UTF-8 string
 *
 *	@return	Index, @c SIZE_MAX on failure
 */
EXPORTED size_t strings_add_by_vector(strings *const vec, const vector *const str);


/**
 *	Get string
 *
 *	@param	vec				Strings vector
 *	@param	index			Index
 *
 *	@return	String, @c NULL on failure
 */
EXPORTED const char *strings_get(const strings *const vec, const size_t index);

/**
 *	Get string length
 *
 *	@param	vec				Strings vector
 *	@param	index			Index
 *
 *	@return	String length, @c 0 on failure
 */
EXPORTED size_t strings_get_length(const strings *const vec, const size_t index);


/**
 *	Remove last string
 *
 *	@param	vec				Strings vector
 *
 *	@return	Deleted string, @c NULL on failure
 */
EXPORTED const char *strings_remove(strings *const vec);


/**
 *	Get strings vector size
 *
 *	@param	vec				Strings vector
 *
 *	@return	Size of strings vector, @c SIZE_MAX on failure
 */
EXPORTED size_t strings_size(const strings *const vec);

/**
 *	Check that strings vector is correct
 *
 *	@param	vec				Strings vector
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool strings_is_correct(const strings *const vec);


/**
 *	Free allocated memory
 *
 *	@param	vec				Strings vector
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int strings_clear(strings *const vec);

#ifdef __cplusplus
} /* extern "C" */
#endif
