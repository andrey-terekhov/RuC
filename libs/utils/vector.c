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

#include "vector.h"
#include <stdlib.h>
#include <string.h>


static int change_size(vector *const vec, const size_t size)
{
	if (size > vec->size_alloc)
	{
		const size_t alloc_new = size > 2 * vec->size_alloc ? size : 2 * vec->size_alloc;
		item_t *array_new = realloc(vec->array, alloc_new * sizeof(item_t));
		if (array_new == NULL)
		{
			return -1;
		}

		vec->size_alloc = alloc_new;
		vec->array = array_new;
	}

	if (size > vec->size)
	{
		memset(&vec->array[vec->size], 0, (size - vec->size) * sizeof(item_t));
	}

	vec->size = size;
	return 0;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


vector vector_create(const size_t alloc)
{
	vector vec;

	vec.size = 0;
	vec.size_alloc = alloc != 0 ? alloc : 1;
	vec.array = malloc(vec.size_alloc * sizeof(item_t));

	return vec;
}


size_t vector_add(vector *const vec, const item_t value)
{
	if (!vector_is_correct(vec) || change_size(vec, vec->size + 1))
	{
		return SIZE_MAX;
	}

	vec->array[vec->size - 1] = value;
	return vec->size - 1;
}

size_t vector_add_double(vector *const vec, const double value)
{
	if (!vector_is_correct(vec) || change_size(vec, vec->size + DOUBLE_SIZE))
	{
		return SIZE_MAX;
	}

	item_store_double(value, &vec->array[vec->size - DOUBLE_SIZE]);
	return vec->size - DOUBLE_SIZE;
}

size_t vector_add_int64(vector *const vec, const int64_t value)
{
	if (!vector_is_correct(vec) || change_size(vec, vec->size + INT64_SIZE))
	{
		return SIZE_MAX;
	}

	item_store_int64(value, &vec->array[vec->size - DOUBLE_SIZE]);
	return vec->size - INT64_SIZE;
}


int vector_set(vector *const vec, const size_t index, const item_t value)
{
	if (!vector_is_correct(vec) || index >= vec->size)
	{
		return -1;
	}

	vec->array[index] = value;
	return 0;
}

size_t vector_set_double(vector *const vec, const size_t index, const double value)
{
	if (!vector_is_correct(vec) || index + DOUBLE_SIZE > vec->size)
	{
		return SIZE_MAX;
	}

	return item_store_double(value, &vec->array[index]);
}

size_t vector_set_int64(vector *const vec, const size_t index, const int64_t value)
{
	if (!vector_is_correct(vec) || index + INT64_SIZE > vec->size)
	{
		return SIZE_MAX;
	}

	return item_store_int64(value, &vec->array[index]);
}


item_t vector_get(const vector *const vec, const size_t index)
{
	if (!vector_is_correct(vec) || index >= vec->size)
	{
		return ITEM_MAX;
	}

	return vec->array[index];
}

double vector_get_double(const vector *const vec, const size_t index)
{
	if (!vector_is_correct(vec) || index + DOUBLE_SIZE > vec->size)
	{
		return DBL_MAX;
	}

	return item_restore_double(&vec->array[index]);
}

int64_t vector_get_int64(const vector *const vec, const size_t index)
{
	if (!vector_is_correct(vec) || index + INT64_SIZE > vec->size)
	{
		return LLONG_MAX;
	}

	return item_restore_int64(&vec->array[index]);
}


item_t vector_remove(vector *const vec)
{
	if (!vector_is_correct(vec) || vec->size == 0)
	{
		return ITEM_MAX;
	}

	return vec->array[--vec->size];
}

double vector_remove_double(vector *const vec)
{
	if (!vector_is_correct(vec) || vec->size < DOUBLE_SIZE)
	{
		return DBL_MAX;
	}

	vec->size -= DOUBLE_SIZE;
	return item_restore_double(&vec->array[vec->size]);
}

int64_t vector_remove_int64(vector *const vec)
{
	if (!vector_is_correct(vec) || vec->size < INT64_SIZE)
	{
		return LLONG_MAX;
	}

	vec->size -= INT64_SIZE;
	return item_restore_int64(&vec->array[vec->size]);
}


int vector_increase(vector *const vec, const size_t size)
{
	return vector_is_correct(vec) ? change_size(vec, vec->size + size) : -1;
}

int vector_resize(vector *const vec, const size_t size)
{
	return vector_is_correct(vec) ? change_size(vec, size) : -1;
}

size_t vector_size(const vector *const vec)
{
	return vector_is_correct(vec) ? vec->size : SIZE_MAX;
}

bool vector_is_correct(const vector *const vec)
{
	return vec != NULL && vec->array != NULL;
}


int vector_clear(vector *const vec)
{
	if (!vector_is_correct(vec))
	{
		return -1;
	}

	free(vec->array);
	vec->array = NULL;

	return 0;
}
