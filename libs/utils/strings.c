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


static const size_t AVERAGE_STRING_SIZE = 256;


strings strings_create(const size_t alloc)
{
	strings vec;

	vec.indexes_size = 0;
	vec.indexes_alloc = alloc != 0 ? alloc : 1;

	vec.indexes = malloc(vec.indexes_alloc * sizeof(size_t));	
	if (vec.indexes == NULL)
	{
		return vec;
	}

	vec.all_strings_size = 0;
	vec.all_strings_alloc = vec.indexes_alloc * AVERAGE_STRING_SIZE;

	vec.all_strings = malloc(vec.all_strings_alloc * sizeof(char));
	if (vec.all_strings == NULL)
	{
		free(vec.indexes);
		return vec;
	}

	return vec;
}


size_t strings_add(strings *const vec, const char *const value)
{
	if (!strings_is_correct(vec) || value == NULL)
	{
		return SIZE_MAX;
	}

	if (vec->indexes_size == vec->indexes_alloc)
	{
		size_t *indexes_new = realloc(vec->indexes, 2 * vec->indexes_alloc * sizeof(size_t));
		if (indexes_new == NULL)
		{
			return SIZE_MAX;
		}

		vec->indexes_alloc *= 2;
		vec->indexes = indexes_new;
	}

	vec->indexes[vec->indexes_size] = vec->all_strings_size;
	
	size_t i = 0;
	do
	{
		if (vec->all_strings_size == vec->all_strings_alloc)
		{
			char *all_strings_new = realloc(vec->all_strings, 2 * vec->all_strings_alloc * sizeof(char));
			if (all_strings_new == NULL)
			{
				return SIZE_MAX;
			}

			vec->all_strings_alloc *= 2;
			vec->all_strings = all_strings_new;
		}

		vec->all_strings[vec->all_strings_size++] = value[i];
	} while (value[i++] != '\0');

	return vec->indexes_size++;
}

const char *strings_get(const strings *const vec, const size_t index)
{
	if (!strings_is_correct(vec) || index >= vec->indexes_size)
	{
		return NULL;
	}

	return &vec->all_strings[vec->indexes[index]];
}

const char *strings_remove(strings *const vec)
{
	if (!strings_is_correct(vec) || vec->indexes_size == 0)
	{
		return NULL;
	}

	return &vec->all_strings[vec->indexes[--vec->indexes_size]];
}


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
