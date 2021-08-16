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
#include "keywords.h"


static const size_t MAX_MACRO = 256;


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


storage storage_create()
{
	storage stg;

	stg.as = map_create(MAX_MACRO);
	stg.hs = hash_create(MAX_MACRO);
	stg.vec = strings_create(MAX_MACRO);

	kw_add(&stg.as);
	return stg;
}


size_t storage_add(storage *const stg, const char32_t *const id, const char32_t *const value)
{
	if (!storage_is_correct(stg) || id == NULL)
	{
		return SIZE_MAX;
	}

	const size_t key = map_reserve_by_utf8(&stg->as, id);
	const size_t index = hash_add(&stg->hs, key, 1);
	if (index == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	hash_set_by_index(&stg->hs, index, 0, value != NULL ? (item_t)strings_add_by_utf8(&stg->vec, value) : ITEM_MAX);
	return index;
}

size_t storage_add_with_args(storage *const stg, const char32_t *const id, const char32_t *const value, const size_t args)
{
	if (!storage_is_correct(stg) || id == NULL || value == NULL)
	{
		return SIZE_MAX;
	}

	const size_t key = map_reserve_by_utf8(&stg->as, id);
	const size_t index = hash_add(&stg->hs, key, 1 + args);
	if (index == SIZE_MAX)
	{
		return SIZE_MAX;
	}

	hash_set_by_index(&stg->hs, index, 0, (item_t)strings_add_by_utf8(&stg->vec, value));
	return index;
}

size_t storage_add_arg(storage *const stg, const char32_t *const id, const size_t index, const char32_t *const arg);
int storage_add_arg_by_index(storage *const stg, const size_t id, const size_t index, const char32_t *const arg);


size_t storage_set(storage *const stg, const char32_t *const id, const char32_t *value);
int storage_set_by_index(storage *const stg, const size_t id, const char32_t *value);


size_t storage_get_index(const storage *const stg, const char32_t *const id)
{
	if (!storage_is_correct(stg) || id == NULL)
	{
		return SIZE_MAX;
	}

}

const char *storage_get(const storage *const stg, const char32_t *const id);
const char *storage_get_by_index(const storage *const stg, const size_t id);
size_t storage_get_amount(const storage *const stg, const char32_t *const id);
size_t storage_get_amount_by_index(const storage *const stg, const size_t id);
const char *storage_get_arg(const storage *const stg, const char32_t *const id, const size_t index);
const char *storage_get_arg_by_index(const storage *const stg, const size_t id, const size_t index);


int storage_remove(storage *const stg, const char32_t *const id);
int storage_remove_by_index(storage *const stg, const size_t id);


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