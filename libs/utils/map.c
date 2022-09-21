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

#include "map.h"
#include <stdlib.h>
#include <string.h>
#include "uniscanner.h"
#include "utf8.h"


struct map_hash
{
	size_t next;
	size_t ref;
	item_t value;
};


static int map_add_key_symbol(map *const as, const char32_t ch)
{
	if (MAX_SYMBOL_SIZE <= as->keys_alloc - as->keys_next)
	{
		as->keys_next += utf8_to_string(&as->keys[as->keys_next], ch);
		return 0;
	}

	char *keys_new = realloc(as->keys, 2 * as->keys_alloc * sizeof(char));
	if (keys_new == NULL)
	{
		return -1;
	}

	as->keys_alloc *= 2;
	as->keys = keys_new;
	return map_add_key_symbol(as, ch);
}

static size_t map_get_hash(map *const as, const char *const key)
{
	if (!map_is_correct(as) || key == NULL || key[0] == '\0')
	{
		return SIZE_MAX;
	}

	as->keys_next = as->keys_size;

	char32_t ch = utf8_convert(key);
	if (map_add_key_symbol(as, ch))
	{
		return SIZE_MAX;
	}

	size_t hash = ch;
	while (key[as->keys_next - as->keys_size] != '\0')
	{
		ch = utf8_convert(&key[as->keys_next - as->keys_size]);
		if (map_add_key_symbol(as, ch))
		{
			return SIZE_MAX;
		}

		hash += ch;
	}

	return hash % MAP_HASH_MAX;
}

static size_t map_get_hash_by_utf8(map *const as, const char32_t *const key)
{
	if (!map_is_correct(as) || key == NULL || key[0] == '\0')
	{
		return SIZE_MAX;
	}

	as->keys_next = as->keys_size;

	size_t hash = 0;
	for (size_t i = 0; key[i] != '\0'; i++)
	{
		if (map_add_key_symbol(as, key[i]))
		{
			return SIZE_MAX;
		}

		hash += key[i];
	}

	return hash % MAP_HASH_MAX;
}

static size_t map_get_hash_by_io(map *const as, universal_io *const io, char32_t *const last)
{
	if (!map_is_correct(as) || !in_is_correct(io) || last == NULL)
	{
		return SIZE_MAX;
	}

	as->keys_next = as->keys_size;

	*last = uni_scan_char(io);
	if (!utf8_is_letter(*last) && *last != '#')
	{
		return SIZE_MAX;
	}

	if (map_add_key_symbol(as, *last))
	{
		return SIZE_MAX;
	}

	size_t hash = *last;
	*last = uni_scan_char(io);
	while (utf8_is_letter(*last) || utf8_is_digit(*last))
	{
		if (map_add_key_symbol(as, *last))
		{
			return SIZE_MAX;
		}

		hash += *last;
		*last = uni_scan_char(io);
	}

	return hash % MAP_HASH_MAX;
}


static inline int map_cmp_key(const map *const as, const size_t index)
{
	return strcmp(&as->keys[as->values[index].ref], &as->keys[as->keys_size]);
}

static inline size_t map_get_index_by_hash(const map *const as, const size_t hash)
{
	if (hash == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	size_t index = hash;
	while (as->values[index].next != SIZE_MAX)
	{
		if (map_cmp_key(as, index) == 0)
		{
			return index;
		}
		index = as->values[index].next;
	}

	if (as->values[index].ref == SIZE_MAX || map_cmp_key(as, index) != 0)
	{
		return SIZE_MAX;
	}

	return index;
}

static size_t map_add_by_hash(map *const as, const size_t hash, const item_t value)
{
	if (hash == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	size_t index = hash;
	while (as->values[index].next != SIZE_MAX)
	{
		if (map_cmp_key(as, index) == 0)
		{
			return value == ITEM_MAX ? index : SIZE_MAX;
		}
		index = as->values[index].next;
	}

	if (as->values[index].ref == SIZE_MAX)
	{
		as->values[index].ref = as->keys_size;
		as->keys_size = as->keys_next + 1;
		as->values[index].value = value;
		return index;
	}
	else if (map_cmp_key(as, index) == 0)
	{
		return value == ITEM_MAX ? index : SIZE_MAX;
	}

	if (as->values_size == as->values_alloc)
	{
		map_hash *values_new = realloc(as->values, 2 * as->values_alloc * sizeof(map_hash));
		if (values_new == NULL)
		{
			return SIZE_MAX;
		}

		as->values_alloc *= 2;
		as->values = values_new;
	}

	as->values[index].next = as->values_size;
	index = as->values_size++;

	as->values[index].next = SIZE_MAX;
	as->values[index].ref = as->keys_size;
	as->keys_size = as->keys_next + 1;
	as->values[index].value = value;
	return index;
}

static size_t map_set_by_hash(map *const as, const size_t hash, const item_t value)
{
	const size_t index = map_get_index_by_hash(as, hash);
	if (index == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	as->values[index].value = value;
	return index;
}

static item_t map_get_by_hash(const map *const as, const size_t hash)
{
	const size_t index = map_get_index_by_hash(as, hash);
	if (index == SIZE_MAX)
	{
		return ITEM_MAX;
	}

	return as->values[index].value;
}


static inline map map_broken()
{
	map as;
	as.values = NULL;
	as.keys = NULL;
	return as;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


map map_create(const size_t alloc)
{
	map as;

	as.values_size = MAP_HASH_MAX;
	as.values_alloc = as.values_size + alloc;

	as.values = malloc(as.values_alloc * sizeof(map_hash));
	if (as.values == NULL)
	{
		return map_broken();
	}

	for (size_t i = 0; i < as.values_size; i++)
	{
		as.values[i].next = SIZE_MAX;
		as.values[i].ref = SIZE_MAX;
	}

	as.keys_size = 0;
	as.keys_alloc = as.values_alloc * MAP_KEY_SIZE;

	as.keys = malloc(as.keys_alloc * sizeof(char));
	if (as.keys == NULL)
	{
		free(as.values);
		return map_broken();
	}

	return as;
}


size_t map_reserve(map *const as, const char *const key)
{
	return map_add_by_hash(as, map_get_hash(as, key), ITEM_MAX);
}

size_t map_reserve_by_utf8(map *const as, const char32_t *const key)
{
	return map_add_by_hash(as, map_get_hash_by_utf8(as, key), ITEM_MAX);
}

size_t map_reserve_by_io(map *const as, universal_io *const io, char32_t *const last)
{
	return map_add_by_hash(as, map_get_hash_by_io(as, io, last), ITEM_MAX);
}


size_t map_add(map *const as, const char *const key, const item_t value)
{
	return map_add_by_hash(as, map_get_hash(as, key), value);
}

size_t map_add_by_utf8(map *const as, const char32_t *const key, const item_t value)
{
	return map_add_by_hash(as, map_get_hash_by_utf8(as, key), value);
}

size_t map_add_by_io(map *const as, universal_io *const io, const item_t value, char32_t *const last)
{
	return map_add_by_hash(as, map_get_hash_by_io(as, io, last), value);
}


size_t map_set(map *const as, const char *const key, const item_t value)
{
	return map_set_by_hash(as, map_get_hash(as, key), value);
}

size_t map_set_by_utf8(map *const as, const char32_t *const key, const item_t value)
{
	return map_set_by_hash(as, map_get_hash_by_utf8(as, key), value);
}

size_t map_set_by_io(map *const as, universal_io *const io, const item_t value, char32_t *const last)
{
	return map_set_by_hash(as, map_get_hash_by_io(as, io, last), value);
}

int map_set_by_index(map *const as, const size_t index, const item_t value)
{
	if (!map_is_correct(as) || index >= as->values_size || as->values[index].ref == SIZE_MAX)
	{
		return -1;
	}

	as->values[index].value = value;
	return 0;
}


size_t map_get_index(map *const as, const char *const key)
{
	return map_get_index_by_hash(as, map_get_hash(as, key));
}

size_t map_get_index_by_utf8(map *const as, const char32_t *const key)
{
	return map_get_index_by_hash(as, map_get_hash_by_utf8(as, key));
}

size_t map_get_index_by_io(map *const as, universal_io *const io, char32_t *const last)
{
	return map_get_index_by_hash(as, map_get_hash_by_io(as, io, last));
}


item_t map_get(map *const as, const char *const key)
{
	return map_get_by_hash(as, map_get_hash(as, key));
}

item_t map_get_by_utf8(map *const as, const char32_t *const key)
{
	return map_get_by_hash(as, map_get_hash_by_utf8(as, key));
}

item_t map_get_by_io(map *const as, universal_io *const io, char32_t *const last)
{
	return map_get_by_hash(as, map_get_hash_by_io(as, io, last));
}

item_t map_get_by_index(const map *const as, const size_t index)
{
	return map_is_correct(as) && index < as->values_size && as->values[index].ref != SIZE_MAX
		? as->values[index].value
		: ITEM_MAX;
}


const char *map_to_string(const map *const as, const size_t index)
{
	return map_is_correct(as) && index < as->values_size && as->values[index].ref != SIZE_MAX
		? &as->keys[as->values[index].ref]
		: NULL;
}

const char *map_last_read(const map *const as)
{
	return map_is_correct(as) && as->keys_size < as->keys_next ? &as->keys[as->keys_size] : NULL;
}

bool map_is_correct(const map *const as)
{
	return as != NULL && as->values != NULL && as->keys != NULL;
}


int map_clear(map *const as)
{
	if (!map_is_correct(as))
	{
		return -1;
	}

	free(as->values);
	as->values = NULL;

	free(as->keys);
	as->keys = NULL;

	return 0;
}
