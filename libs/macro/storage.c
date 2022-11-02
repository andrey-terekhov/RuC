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

#include "storage.h"
#include <assert.h>
#include "keywords.h"
#include "uniscanner.h"


static const size_t MAX_MACRO = 256;


extern const char *storage_get_by_utf8(storage *const stg, const char32_t *const id);
extern const char *storage_get(storage *const stg, const char *const id);
extern size_t storage_get_args_by_utf8(storage *const stg, const char32_t *const id);
extern size_t storage_get_args(storage *const stg, const char *const id);

extern size_t storage_set_by_utf8(storage *const stg, const char32_t *const id, const char *value);
extern size_t storage_set(storage *const stg, const char *const id, const char *value);
extern size_t storage_set_args_by_utf8(storage *const stg, const char32_t *const id, const size_t args);
extern size_t storage_set_args(storage *const stg, const char *const id, const size_t args);

extern int storage_remove(storage *const stg, const char *const id);
extern int storage_remove_by_utf8(storage *const stg, const char32_t *const id);


static inline size_t add(storage *const stg, const void *const id, size_t (*reserve)(map *const, const void *const))
{
	if (!storage_is_correct(stg) || id == NULL)
	{
		return SIZE_MAX;
	}

	const size_t key = reserve(&stg->as, id);
	const size_t index = hash_add(&stg->hs, (item_t)key, 2);

	hash_set_by_index(&stg->hs, index, 0, ITEM_MAX);
	hash_set_by_index(&stg->hs, index, 1, 0);
	return index;
}


static inline size_t get_index(storage *const stg, const void *const id, size_t (*func)(map *const, const void *const))
{
	if (!storage_is_correct(stg) || id == NULL)
	{
		return SIZE_MAX;
	}

	const size_t index = func(&stg->as, id);
	const item_t value = map_get_by_index(&stg->as, index);

	return kw_is_correct(value) ? (size_t)value : hash_get_index(&stg->hs, (item_t)index);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


storage storage_create()
{
	static_assert(END_KEYWORD < MAX_HASH, "Keyword constants should be less than MAX_HASH");

	storage stg;

	stg.as = map_create(MAX_MACRO);
	stg.hs = hash_create(MAX_MACRO);
	stg.vec = strings_create(MAX_MACRO);

	kw_add(&stg.as);
	return stg;
}


size_t storage_add(storage *const stg, const char *const id)
{
	return add(stg, id, (size_t (*)(map *const, const void *const))&map_reserve);
}

size_t storage_add_by_utf8(storage *const stg, const char32_t *const id)
{
	return add(stg, id, (size_t (*)(map *const, const void *const))&map_reserve_by_utf8);
}

size_t storage_add_by_io(storage *const stg, universal_io *const io)
{
	if (!storage_is_correct(stg) || io == NULL)
	{
		return SIZE_MAX;
	}

	char32_t last = (char32_t)EOF;
	const size_t key = map_reserve_by_io(&stg->as, io, &last);
	uni_unscan_char(io, last);

	const size_t index = hash_add(&stg->hs, (item_t)key, 2);
	hash_set_by_index(&stg->hs, index, 0, ITEM_MAX);
	hash_set_by_index(&stg->hs, index, 1, 0);
	return index;
}


size_t storage_get_index(storage *const stg, const char *const id)
{
	return get_index(stg, id, (size_t (*)(map *const, const void *const))&map_get_index);
}

size_t storage_get_index_by_utf8(storage *const stg, const char32_t *const id)
{
	return get_index(stg, id, (size_t (*)(map *const, const void *const))&map_get_index_by_utf8);
}

const char *storage_get_by_index(const storage *const stg, const size_t id)
{
	return storage_is_correct(stg)
		? strings_get(&stg->vec, (size_t)hash_get_by_index(&stg->hs, id, 0))
		: NULL;
}

size_t storage_get_args_by_index(const storage *const stg, const size_t id)
{
	if (!storage_is_correct(stg))
	{
		return 0;
	}

	const item_t count = hash_get_by_index(&stg->hs, id, 1);
	return count != ITEM_MAX ? (size_t)count : 0;
}


size_t storage_set_by_index(storage *const stg, const size_t id, const char *value)
{
	if (!storage_is_correct(stg))
	{
		return SIZE_MAX;
	}

	if (hash_get_by_index(&stg->hs, id, 0) == (item_t)strings_size(&stg->vec) - 1)
	{
		strings_remove(&stg->vec);
	}

	hash_set_by_index(&stg->hs, id, 0, (item_t)strings_add(&stg->vec, value));
	return id;
}

size_t storage_set_args_by_index(storage *const stg, const size_t id, const size_t args)
{
	if (!storage_is_correct(stg))
	{
		return SIZE_MAX;
	}

	hash_set_by_index(&stg->hs, id, 1, (item_t)args);
	return id;
}


int storage_remove_by_index(storage *const stg, const size_t id)
{
	if (!storage_is_correct(stg))
	{
		return 0;
	}

	if (hash_get_by_index(&stg->hs, id, 0) == (item_t)strings_size(&stg->vec) - 1)
	{
		strings_remove(&stg->vec);
	}

	return hash_remove_by_index(&stg->hs, id);
}


size_t storage_search(storage *const stg, universal_io *const io)
{
	if (!storage_is_correct(stg))
	{
		return SIZE_MAX;
	}

	char32_t last = (char32_t)EOF;
	const size_t index = map_get_index_by_io(&stg->as, io, &last);
	uni_unscan_char(io, last);

	const item_t value = map_get_by_index(&stg->as, index);
	return kw_is_correct(value) ? (size_t)value : hash_get_index(&stg->hs, (item_t)index);
}

const char *storage_last_read(const storage *const stg)
{
	return storage_is_correct(stg) ? map_last_read(&stg->as) : NULL;
}

bool storage_is_correct(const storage *const stg)
{
	return stg != NULL;
}


int storage_clear(storage *const stg)
{
	if (!storage_is_correct(stg))
	{
		return -1;
	}

	map_clear(&stg->as);
	hash_clear(&stg->hs);
	strings_clear(&stg->vec);

	return 0;
}
