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
#include "utf8.h"


const size_t MAP_HASH_MAX = 256;
const size_t MAP_KEY_SIZE = 8;


struct map_hash
{
	size_t next;
	size_t ref;
	int value;
};


size_t map_get_hash(map *const as, const char *const key)
{
	char32_t ch = utf8_convert(key);
	if (!utf8_is_letter(ch))
	{
		return SIZE_MAX;
	}

	as->keys_next = as->keys_size;
	if (map_add_key_symbol(as, ch))
	{
		return SIZE_MAX;
	}

	size_t hash = ch;
	while (key[as->keys_next - as->keys_size] != '\0')
	{
		ch = utf8_convert(&key[as->keys_next - as->keys_size]);
		if (!utf8_is_letter(ch) && !utf8_is_digit(ch))
		{
			return SIZE_MAX;
		}

		if (map_add_key_symbol(as, ch))
		{
			return SIZE_MAX;
		}

		hash += ch;
	}

	return hash % MAP_HASH_MAX;
}

size_t map_get_hash_by_io(map *const as, universal_io *const io, char32_t *const last);

size_t map_add_by_hash(map *const as, const size_t hash, const int value);


map map_broken()
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


map map_create(const size_t values)
{
	map as;

	as.values_size = MAP_HASH_MAX;
	as.values_alloc = as.values_size + values;

	as.values = malloc(as.values_alloc * sizeof(map_hash));
	if (as.values == NULL)
	{
		return map_broken();
	}

	for (size_t i = 0; i < as.values_size; i++)
	{
		as.values[i].next = SIZE_MAX;
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


size_t map_add(map *const as, const char *const key, const int value)
{
	if (!map_is_correct(as) || key == NULL)
	{
		return SIZE_MAX;
	}

	return map_add_by_hash(as, map_get_hash(as, key), value);
}

size_t map_add_by_io(map *const as, universal_io *const io, const int value, char32_t *const last)
{
	if (!map_is_correct(as) || !in_is_correct(io) || last == NULL)
	{
		return SIZE_MAX;
	}

	return map_add_by_hash(as, map_get_hash_by_io(as, io, last), value);
}


size_t map_set(map *const as, const char *const key, const int value);

size_t map_set_by_io(map *const as, universal_io *const io, const int value, char32_t *const last);

int map_set_at(map *const as, const size_t index, const int value);



int map_get(const map *const as, const char *const key);

int map_get_by_io(const map *const as, universal_io *const io, char32_t *const last);

int map_get_at(const map *const as, const size_t index);


int map_is_correct(const map *const as)
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
