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


#ifdef __cplusplus
extern "C" {
#endif

const map_size_t MAP_SIZE_MAX = USHRT_MAX;
typedef uint16_t map_size_t;

/** Hash table */
typedef struct map_hash map_hash;

/** Associative array (Dictionary) */
typedef struct map
{
	char *keys;					/**< Keys storage */
	size_t keys_size;			/**< Size of keys storage */
	size_t keys_alloc;			/**< Allocated size of keys storage */

	map_hash *values;			/**< Values storage */
	map_size_t values_size;		/**< Size of values storage */
	size_t values_alloc;		/**< Allocated size of values storage */
} map;


/**
 *	Create map structure
 *
 *	@param	values			Initializer of values count
 *
 *	@return	Map structure
 */
map map_create(const map_size_t values);

/**
 *	Add new node argument
 *
 *	@param	as				Node structure
 *	@param	key				Node argument
 *	@param	value			Node argument
 *
 *	@return	
 */
const char *map_add(map *const as, const char *const key, const int value);
const char *map_set(map *const as, const char *const key, const int value);
int map_get(const map *const as, const char *const key);

int map_is_correct(const map *const as);

int map_clear();

#ifdef __cplusplus
} /* extern "C" */
#endif
