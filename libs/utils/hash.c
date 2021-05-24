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

#include "hash.h"


extern int hash_set_by_index(hash *const hs, const size_t index, const size_t num, const item_t value);

extern item_t hash_get_by_index(hash *const hs, const size_t index, const size_t num);
extern item_t hash_get_key(const hash *const hs, const size_t index);

extern int hash_is_correct(const hash *const hs);
extern int hash_clear(hash *const hs);


static inline size_t get_hash(const item_t key)
{
	return (size_t)key % MAX_HASH;
}

static size_t get_index(const hash *const hs, const item_t key)
{
	if (!hash_is_correct(hs))
	{
		return SIZE_MAX;
	}

	size_t index = get_hash(key);
	item_t next = vector_get(hs, index);
	if (next == 0)
	{
		return index;
	}

	index = SIZE_MAX;
	while (next != ITEM_MAX && next != 0)
	{
		index = (size_t)next;
		if (vector_get(hs, index + 1) == key)
		{
			return index;
		}

		next = vector_get(hs, index);
	}

	return index;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


hash hash_create(const size_t alloc)
{
	hash hs = vector_create(MAX_HASH + alloc * (3 + VALUE_SIZE));
	vector_resize(&hs, MAX_HASH);	// All set by zero
	return hs;
}


size_t hash_add(hash *const hs, const item_t key, const size_t amount)
{
	const size_t index = get_index(hs, key);
	if (vector_get(hs, index) != 0)
	{
		return SIZE_MAX;
	}

	const size_t size = vector_size(hs);
	vector_set(hs, index, size);
	vector_resize(hs, size + 3 + amount);	// New elements set by zero

	vector_set(hs, size + 1, key);
	vector_set(hs, size + 2, amount);
	return size;
}

size_t hash_set(hash *const hs, const item_t key, const size_t num, const item_t value)
{
	const size_t index = get_index(hs, key);
	if (index == SIZE_MAX || vector_get(hs, index) == 0)
	{
		return SIZE_MAX;
	}

	return hash_set_by_index(hs, index, num, value) == 0 ? index : SIZE_MAX;
}

item_t hash_get(hash *const hs, const item_t key, const size_t num)
{
	const size_t index = get_index(hs, key);
	if (index == SIZE_MAX || vector_get(hs, index) == 0)
	{
		return SIZE_MAX;
	}

	return hash_get_by_index(hs, index, num);
}
