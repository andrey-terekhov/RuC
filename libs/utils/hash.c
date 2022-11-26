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


extern item_t hash_get_key(const hash *const hs, const size_t index);
extern size_t hash_get_amount_by_index(const hash *const hs, const size_t index);

extern item_t hash_get_by_index(const hash *const hs, const size_t index, const size_t num);
extern double hash_get_double_by_index(const hash *const hs, const size_t index, const size_t num);
extern int64_t hash_get_int64_by_index(const hash *const hs, const size_t index, const size_t num);

extern int hash_set_by_index(hash *const hs, const size_t index, const size_t num, const item_t value);
extern size_t hash_set_double_by_index(hash *const hs, const size_t index, const size_t num, const double value);
extern size_t hash_set_int64_by_index(hash *const hs, const size_t index, const size_t num, const int64_t value);

extern int hash_remove_by_index(hash *const hs, const size_t index);

extern bool hash_is_correct(const hash *const hs);
extern int hash_clear(hash *const hs);


static inline size_t get_hash(const item_t key)
{
	return (size_t)key % MAX_HASH;
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
	vector_increase(&hs, MAX_HASH);	// All set by zero
	return hs;
}


size_t hash_add(hash *const hs, const item_t key, const size_t amount)
{
	if (!hash_is_correct(hs))
	{
		return SIZE_MAX;
	}

	size_t prev = get_hash(key);
	item_t index = vector_get(hs, prev);

	if (index != 0)
	{
		item_t record = ITEM_MAX;
		while (index != ITEM_MAX && index != 0)
		{
			const item_t temp = vector_get(hs, (size_t)index + 1);
			if (record == ITEM_MAX && temp == ITEM_MAX && (size_t)vector_get(hs, (size_t)index + 2) == amount)
			{
				record = index;
			}
			else if (temp == key)
			{
				return SIZE_MAX;
			}

			prev = (size_t)index;
			index = vector_get(hs, prev);
		}

		if (record != ITEM_MAX)
		{
			index = record;
			for (size_t i = 0; i < amount; i++)
			{
				vector_set(hs, (size_t)index + 3 + i, 0);
			}
		}
		else if (index == ITEM_MAX)
		{
			return SIZE_MAX;
		}
	}

	if (index == 0)
	{
		index = (item_t)vector_size(hs);
		vector_set(hs, prev, index);
		vector_increase(hs, 3 + amount);	// New elements set by zero
		vector_set(hs, (size_t)index + 2, (item_t)amount);
	}

	vector_set(hs, (size_t)index + 1, key);
	return (size_t)index;
}


size_t hash_get_index(const hash *const hs, const item_t key)
{
	if (!hash_is_correct(hs))
	{
		return SIZE_MAX;
	}

	size_t prev = get_hash(key);
	item_t index = vector_get(hs, prev);

	while (index != ITEM_MAX && index != 0)
	{
		if (vector_get(hs, (size_t)index + 1) == key)
		{
			return (size_t)index;
		}

		prev = (size_t)index;
		index = vector_get(hs, prev);
	}

	return SIZE_MAX;
}

size_t hash_get_amount(const hash *const hs, const item_t key)
{
	return hash_get_amount_by_index(hs, hash_get_index(hs, key));
}


item_t hash_get(const hash *const hs, const item_t key, const size_t num)
{
	return hash_get_by_index(hs, hash_get_index(hs, key), num);
}

double hash_get_double(const hash *const hs, const item_t key, const size_t num)
{
	return hash_get_double_by_index(hs, hash_get_index(hs, key), num);
}

int64_t hash_get_int64(const hash *const hs, const item_t key, const size_t num)
{
	return hash_get_int64_by_index(hs, hash_get_index(hs, key), num);
}


size_t hash_set(hash *const hs, const item_t key, const size_t num, const item_t value)
{
	const size_t index = hash_get_index(hs, key);
	return hash_set_by_index(hs, hash_get_index(hs, key), num, value) == 0 ? index : SIZE_MAX;
}

size_t hash_set_double(hash *const hs, const item_t key, const size_t num, const double value)
{
	const size_t index = hash_get_index(hs, key);
	return hash_set_double_by_index(hs, hash_get_index(hs, key), num, value) != DBL_MAX ? index : SIZE_MAX;
}

size_t hash_set_int64(hash *const hs, const item_t key, const size_t num, const int64_t value)
{
	const size_t index = hash_get_index(hs, key);
	return hash_set_int64_by_index(hs, hash_get_index(hs, key), num, value) != LLONG_MAX ? index : SIZE_MAX;
}


int hash_remove(hash *const hs, const item_t key)
{
	return hash_remove_by_index(hs, hash_get_index(hs, key));
}
