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

size_t storage_add(storage *const stg, const char32_t *const id, const char32_t *const value);
size_t storage_add_with_args(storage *const stg, const char32_t *const id, const char32_t *const value, const size_t args);
size_t storage_add_arg(storage *const stg, const char32_t *const id, const size_t index, const char32_t *const arg);
int storage_add_arg_by_index(storage *const stg, const size_t id, const size_t index, const char32_t *const arg);

size_t storage_set(storage *const stg, const char32_t *const id, const char32_t *value);
int storage_set_by_index(storage *const stg, const size_t id, const char32_t *value);

size_t storage_get_index(const storage *const stg, const char32_t *const id);
const char *storage_get(const storage *const stg, const char32_t *const id);
const char *storage_get_by_index(const storage *const stg, const size_t id);
size_t storage_get_amount(const storage *const stg, const char32_t *const id);
size_t storage_get_amount_by_index(const storage *const stg, const size_t id);
const char *storage_get_arg(const storage *const stg, const char32_t *const id, const size_t index);
const char *storage_get_arg_by_index(const storage *const stg, const size_t id, const size_t index);

int storage_remove(storage *const stg, const char32_t *const id);
int storage_remove_by_index(storage *const stg, const size_t id);

bool storage_is_correct(const storage *const stg);

int storage_clear(storage *const stg);
