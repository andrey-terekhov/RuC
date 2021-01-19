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


const size_t MAP_HASH_MAX = 256;


struct map_hash
{
	size_t next;
	size_t ref;
	int value;
};


size_t map_get_hash(const char *const key);


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
	

	return as;
}


size_t map_add(map *const as, const char *const key, const int value)
{
	if (as == NULL || key == NULL)
	{
		return SIZE_MAX;
	}

	return 0;
}

size_t map_add_by_io(map *const as, universal_io *const io, const int value);


size_t map_set(map *const as, const char *const key, const int value)
{
	if (as == NULL || key == NULL)
	{
		return SIZE_MAX;
	}

	return 0;
}

size_t map_set_by_io(map *const as, universal_io *const io, const int value);

int map_set_at(map *const as, const size_t index, const int value)
{
	if (as == NULL)
	{
		return -1;
	}

	return 0;
}



int map_get(const map *const as, const char *const key)
{
	if (as == NULL || key == NULL)
	{
		return -1;
	}

	return 0;
}

int map_get_by_io(const map *const as, universal_io *const io);

int map_get_at(const map *const as, const size_t index)
{
	if (as == NULL)
	{
		return -1;
	}

	return 0;
}


int map_clear(map *const as)
{
	if (as == NULL || as->table == NULL || as->table->key == NULL)
	{
		return -1;
	}

	free(as->table->key);
	return 0;
}
