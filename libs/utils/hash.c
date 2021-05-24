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

#include "stack.h"




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
	vector_resize(&hs, MAX_HASH);
	return hs;
}


size_t hash_add(hash *const hs, const item_t key, const size_t amount);

size_t hash_set(hash *const hs, const item_t key, const size_t num, const item_t value);

extern int hash_set_by_index(hash *const hs, const size_t index, const size_t num, const item_t value);


item_t hash_get(hash *const hs, const item_t key, const size_t num);

extern item_t hash_get_by_index(hash *const hs, const size_t index, const size_t num);

extern const item_t hash_get_key(const hash *const hs, const size_t index);


extern int hash_is_correct(const hash *const hs);

extern int hash_clear(hash *const hs);

