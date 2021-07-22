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

#include "strings.h"
#include <stdlib.h>





/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


strings strings_create(const size_t alloc)
{
	strings vec;

	as.all_strings_size = MAP_HASH_MAX;
	as.all_strings_alloc = as.all_strings_size + alloc;

	as.all_strings = malloc(as.all_strings_alloc * sizeof(char));
	if (as.all_strings == NULL)
	{
		return strings_broken();
	}

	for (size_t i = 0; i < as.all_strings_size; i++)
	{
		as.all_strings[i].next = SIZE_MAX;
		as.all_strings[i].ref = SIZE_MAX;
	}

	as.keys_size = 0;
	as.keys_alloc = as.all_strings_alloc * MAP_KEY_SIZE;

	as.keys = malloc(as.keys_alloc * sizeof(char));
	if (as.keys == NULL)
	{
		free(as.all_strings);
		return map_broken();
	}

	vec.indexes_size = 0;
	vec.indexes_alloc = alloc != 0 ? alloc : 1;
	vec.indexes = malloc(vec.indexes_alloc * sizeof(size_t));

	return vec;
}


size_t strings_add(strings *const vec, const char *const value);

const char *strings_get(const strings *const vec, const size_t index);

const char *strings_remove(strings *const vec);


size_t strings_size(const strings *const vec)
{
	return strings_is_correct(vec) ? vec->indexes_size : SIZE_MAX;
}

bool strings_is_correct(const strings *const vec)
{
	return vec != NULL && vec->all_strings != NULL && vec->indexes != NULL;
}


int strings_clear(strings *const vec)
{
	if (!strings_is_correct(vec))
	{
		return -1;
	}

	free(vec->all_strings);
	vec->all_strings = NULL;

	free(vec->indexes);
	vec->indexes = NULL;

	return 0;
}
