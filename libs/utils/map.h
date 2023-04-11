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
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

static const size_t MAP_HASH_MAX = 256;
static const size_t MAP_KEY_SIZE = 8;


/** Hash table */
typedef struct map_hash map_hash;

/** Associative array (Dictionary) */
typedef struct map
{
    char *keys;          /**< Keys storage */
    size_t keys_size;    /**< Size of keys storage */
    size_t keys_next;    /**< Next size position */
    size_t keys_alloc;   /**< Allocated size of keys storage */

    map_hash *values;    /**< Values storage */
    size_t values_size;  /**< Size of values storage */
    size_t values_alloc; /**< Allocated size of values storage */
} map;


/**
 *	Create map structure
 *
 *	@param	alloc			Initializer of allocated size
 *
 *	@return	Map structure
 */
EXPORTED map map_create(const size_t alloc);


/**
 *	Reserve new key or return existing
 *
 *	@param	as				Map structure
 *	@param	key				Unique string key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_reserve(map *const as, const char *const key);

/**
 *	Reserve new UTF-8 key or return existing
 *
 *	@param	as				Map structure
 *	@param	key				Unique UTF-8 string key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_reserve_by_utf8(map *const as, const char32_t *const key);

/**
 *	Reserve new key by reading it from io or return existing
 *
 *	@param	as				Map structure
 *	@param	io				Universal io structure
 *	@param	last			Next character after key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_reserve_by_io(map *const as, universal_io *const io, char32_t *const last);


/**
 *	Add new key-value pair
 *
 *	@param	as				Map structure
 *	@param	key				Unique string key
 *	@param	value			Value
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_add(map *const as, const char *const key, const item_t value);

/**
 *	Add new UTF-8 key-value pair
 *
 *	@param	as				Map structure
 *	@param	key				Unique UTF-8 string key
 *	@param	value			Value
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_add_by_utf8(map *const as, const char32_t *const key, const item_t value);

/**
 *	Add new pair by reading key from io
 *
 *	@param	as				Map structure
 *	@param	io				Universal io structure
 *	@param	value			Value
 *	@param	last			Next character after key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_add_by_io(map *const as, universal_io *const io, const item_t value, char32_t *const last);


/**
 *	Set new value by existing key
 *
 *	@param	as				Map structure
 *	@param	key				Unique string key
 *	@param	value			New value
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_set(map *const as, const char *const key, const item_t value);

/**
 *	Set new value by existing UTF-8 key
 *
 *	@param	as				Map structure
 *	@param	key				Unique UTF-8 string key
 *	@param	value			New value
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_set_by_utf8(map *const as, const char32_t *const key, const item_t value);

/**
 *	Set new value by reading existing key from io
 *
 *	@param	as				Map structure
 *	@param	io				Universal io structure
 *	@param	value			New value
 *	@param	last			Next character after key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_set_by_io(map *const as, universal_io *const io, const item_t value, char32_t *const last);

/**
 *	Set new value by index
 *
 *	@param	as				Map structure
 *	@param	index			Record index
 *	@param	value			New value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int map_set_by_index(map *const as, const size_t index, const item_t value);


/**
 *	Get index of record by key
 *
 *	@param	as				Map structure
 *	@param	key				Unique string key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_get_index(map *const as, const char *const key);

/**
 *	Get index of record by UTF-8 key
 *
 *	@param	as				Map structure
 *	@param	key				Unique UTF-8 string key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_get_index_by_utf8(map *const as, const char32_t *const key);

/**
 *	Get index of record by reading key from io
 *
 *	@param	as				Map structure
 *	@param	io				Universal io structure
 *	@param	last			Next character after key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t map_get_index_by_io(map *const as, universal_io *const io, char32_t *const last);


/**
 *	Get value by key
 *
 *	@param	as				Map structure
 *	@param	key				Unique string key
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t map_get(map *const as, const char *const key);

/**
 *	Get value by UTF-8 key
 *
 *	@param	as				Map structure
 *	@param	key				Unique UTF-8 string key
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t map_get_by_utf8(map *const as, const char32_t *const key);

/**
 *	Get value by reading key from io
 *
 *	@param	as				Map structure
 *	@param	io				Universal io structure
 *	@param	last			Next character after key
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t map_get_by_io(map *const as, universal_io *const io, char32_t *const last);

/**
 *	Get value by index
 *
 *	@param	as				Map structure
 *	@param	index			Value index
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t map_get_by_index(const map *const as, const size_t index);


/**
 *	Return key by index
 *
 *	@param	as				Map structure
 *	@param	index			Key index
 *
 *	@return	Key, @c NULL on failure
 */
EXPORTED const char *map_to_string(const map *const as, const size_t index);

/**
 *	Return the last read key
 *
 *	@param	as				Map structure
 *
 *	@return	Key, @c NULL on failure
 */
EXPORTED const char *map_last_read(const map *const as);

/**
 *	Check that map is correct
 *
 *	@param	as				Map structure
 *
 *	@return	@c 1 on true, @c 0 on false
 */
EXPORTED bool map_is_correct(const map *const as);


/**
 *	Free allocated memory
 *
 *	@param	as				Map structure
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
EXPORTED int map_clear(map *const as);

#ifdef __cplusplus
} /* extern "C" */
#endif
