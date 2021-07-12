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

#include "vector.h"


#define MAX_HASH 256
#define VALUE_SIZE 4


#ifdef __cplusplus
extern "C" {
#endif

/** Hash table */
typedef vector hash;


/**
 *	Create new hash table
 *
 *	@param	alloc			Initializer of allocated size
 *
 *	@return	Hash table
 */
EXPORTED hash hash_create(const size_t alloc);


/**
 *	Add new key
 *
 *	@param	hs				Hash table
 *	@param	key				Unique key
 *	@param	amount			Values amount
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t hash_add(hash *const hs, const item_t key, const size_t amount);

/**
 *	Get index of record by key
 *
 *	@param	hs				Hash table
 *	@param	key				Unique key
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t hash_get_index(const hash *const hs, const item_t key);

/**
 *	Get values amount by key
 *
 *	@param	hs				Hash table
 *	@param	key				Unique key
 *
 *	@return	Values amount, @c 0 on failure
 */
EXPORTED size_t hash_get_amount(const hash *const hs, const item_t key);

/**
 *	Get value by key
 *
 *	@param	hs				Hash table
 *	@param	key				Unique key
 *	@param	num				Value number
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
EXPORTED item_t hash_get(const hash *const hs, const item_t key, const size_t num);

/**
 *	Set new value by existing key
 *
 *	@param	hs				Hash table
 *	@param	key				Unique key
 *	@param	num				Value number
 *	@param	value			New value
 *
 *	@return	Index of record, @c SIZE_MAX on failure
 */
EXPORTED size_t hash_set(hash *const hs, const item_t key, const size_t num, const item_t value);


/**
 *	Return key by index
 *
 *	@param	hs				Hash table
 *	@param	index			Record index
 *
 *	@return	Key, @c ITEM_MAX on failure
 */
inline item_t hash_get_key(const hash *const hs, const size_t index)
{
	return vector_get(hs, index + 1);
}

/**
 *	Get values amount by index
 *
 *	@param	hs				Hash table
 *	@param	index			Record index
 *
 *	@return	Values amount, @c 0 on failure
 */
inline size_t hash_get_amount_by_index(const hash *const hs, const size_t index)
{
	const item_t amount = vector_get(hs, index + 2);
	return amount != ITEM_MAX ? (size_t)amount : 0;
}

/**
 *	Get value by index
 *
 *	@param	hs				Hash table
 *	@param	index			Record index
 *	@param	num				Value number
 *
 *	@return	Value, @c ITEM_MAX on failure
 */
inline item_t hash_get_by_index(const hash *const hs, const size_t index, const size_t num)
{
	return num < hash_get_amount_by_index(hs, index) ? vector_get(hs, index + 3 + num) : ITEM_MAX;
}

/**
 *	Set new value by index
 *
 *	@param	hs				Hash table
 *	@param	index			Record index
 *	@param	num				Value number
 *	@param	value			New value
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int hash_set_by_index(hash *const hs, const size_t index, const size_t num, const item_t value)
{
	return num < hash_get_amount_by_index(hs, index) ? vector_set(hs, index + 3 + num, value) : -1;
}


/**
 *	Check that hash is correct
 *
 *	@param	hs				Hash table
 *
 *	@return	@c 1 on true, @c 0 on false
 */
inline bool hash_is_correct(const hash *const hs)
{
	return vector_is_correct(hs) && vector_size(hs) >= MAX_HASH;
}


/**
 *	Free allocated memory
 *
 *	@param	hs				Hash table
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
inline int hash_clear(hash *const hs)
{
	return vector_clear(hs);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
