/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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

#include "linker.h"
#include <string.h>
#include "item.h"
#include "error.h"

#ifndef _WIN32
	#include <unistd.h>
#else
	#define F_OK 0

	extern int access(const char *path, int mode);
#endif


static void linker_make_path(char *const buffer, const char *const name, const char *const path, const bool is_file)
{
	size_t index = 0;

	if (path != NULL)
	{
		if (is_file)
		{
			char *slash = strrchr(path, '/');
			if (slash != NULL)
			{
				index = slash - path + 1;
				strncpy(buffer, path, index);
			}
		}
		else
		{
			index = sprintf(buffer, "%s/", path);
		}
	}

	strcpy(&buffer[index], name);
}

static inline size_t linker_internal_path(linker *const lk, const char *const file)
{
	char path[MAX_ARG_SIZE];
	linker_make_path(path, file, ws_get_file(lk->ws, lk->current), true);

	return access(path, F_OK) != -1 && vector_add(&lk->included, 0) != SIZE_MAX
		? ws_add_file(lk->ws, path)
		: SIZE_MAX;
}

static inline size_t linker_external_path(linker *const lk, const char *const file)
{
	char path[MAX_ARG_SIZE];
	for (size_t i = 0; i < ws_get_dirs_num(lk->ws); i++)
	{
		linker_make_path(path, file, ws_get_dir(lk->ws, i), false);

		if (access(path, F_OK) != -1)
		{
			vector_add(&lk->included, 0);
			return ws_add_file(lk->ws, path);
		}
	}

	return SIZE_MAX;
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


linker linker_create(workspace *const ws)
{
	return (linker)
		{
			.ws = ws,
			.sources = ws_get_files_num(ws),
			.included = vector_create(MAX_PATHS),
			.current = SIZE_MAX,
		};
}


universal_io linker_add_source(linker *const lk, const size_t index)
{
	universal_io input = io_create();

	if (linker_is_correct(lk) && in_set_file(&input, ws_get_file(lk->ws, index)) == 0)
	{
		lk->current = index;
	}

	return input;
}

universal_io linker_add_header(linker *const lk, const size_t index)
{
	universal_io input = io_create();

	if (linker_is_correct(lk) && index >= lk->sources
		&& vector_get(&lk->included, index - lk->sources) == 0
		&& in_set_file(&input, ws_get_file(lk->ws, index)) == 0)
	{
		vector_set(&lk->included, index - lk->sources, 1);
		lk->current = index;
	}

	return input;
}


size_t linker_search_internal(linker *const lk, const char *const file)
{
	if (!linker_is_correct(lk) || file == NULL)
	{
		return SIZE_MAX;
	}

	const size_t index = linker_internal_path(lk, file);
	return index != SIZE_MAX ? index : linker_external_path(lk, file);
}

size_t linker_search_external(linker *const lk, const char *const file)
{
	if (!linker_is_correct(lk) || file == NULL)
	{
		return SIZE_MAX;
	}

	const size_t index = linker_external_path(lk, file);
	return index != SIZE_MAX ? index : linker_internal_path(lk, file);
}


const char* linker_current_path(const linker *const lk)
{
	return linker_is_correct(lk) ? ws_get_file(lk->ws, lk->current) : NULL;
}


size_t linker_size(const linker *const lk)
{
	return linker_is_correct(lk) ? ws_get_files_num(lk->ws) : SIZE_MAX;
}

bool linker_is_correct(const linker *const lk)
{
	return lk != NULL && ws_is_correct(lk->ws) && vector_is_correct(&lk->included);
}


int linker_clear(linker *const lk)
{
	if (lk != NULL)
	{
		return -1;
	}

	return vector_clear(&lk->included);
}
