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

#ifndef _MSC_VER
	#include <unistd.h>
#else
	#define F_OK 0

	extern int access(const char *path, int mode);
#endif


static void lk_make_path(char *const output, const char *const source, const char *const header, const int is_slash)
{
	size_t index = 0;

	if (is_slash)
	{
		char *slash = strrchr(source, '/');
		if (slash != NULL)
		{
			index = slash - source + 1;
			strncpy(output, source, index);
		}
	}
	else
	{
		index = sprintf(output, "%s/", source);
	}

	strcpy(&output[index], header);
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
	linker lk;

	if (!ws_is_correct(ws))
	{
		lk.ws = NULL;
		return lk;
	}

	lk.ws = ws;
	lk.sources = ws_get_files_num(ws);

	lk.included = vector_create(MAX_PATHS);
	vector_increase(&lk.included, MAX_PATHS);

	lk.current = SIZE_MAX;

	return lk;
}


universal_io linker_add_source(linker *const lk, const size_t index)
{
	universal_io input = io_create();

	if (linker_is_correct(lk) && index < lk->sources && vector_get(&lk->included, index) == 0)
	{
		if (in_set_file(&input, ws_get_file(lk->ws, index)))
		{
			macro_system_error(ws_get_file(lk->ws, index), source_file_not_found);
		}
		else
		{
			vector_set(&lk->included, index, 1);
			lk->current = index;
		}
	}

	return input;
}

universal_io linker_add_header(linker *const lk, const size_t index)
{
	universal_io input = io_create();

	if (linker_is_correct(lk) && index >= lk->sources && index < MAX_PATHS && vector_get(&lk->included, index) == 0)
	{
		if (in_set_file(&input, ws_get_file(lk->ws, index)))
		{
			macro_system_error(ws_get_file(lk->ws, index), header_file_not_found);
		}
		else
		{
			vector_set(&lk->included, index, 1);
			lk->current = index;
		}
	}

	return input;
}


size_t linker_search_internal(linker *const lk, const char *const file)
{
	if (!linker_is_correct(lk) || file == NULL)
	{
		return SIZE_MAX;
	}

	char full_path[MAX_ARG_SIZE];

	lk_make_path(full_path, ws_get_file(lk->ws, lk->current), file, 1);

	if (access(full_path, F_OK) == -1)
	{
		size_t i = 0;
		const char *dir;
		do
		{
			dir = ws_get_dir(lk->ws, i++);
			lk_make_path(full_path, dir, file, 0);
		} while (dir != NULL && access(full_path, F_OK) == -1);
	}

	if (access(full_path, F_OK) == -1)
	{
		macro_system_error(full_path, header_file_not_found);
		return SIZE_MAX;
	}

	return ws_add_file(lk->ws, full_path);
}

size_t linker_search_external(linker *const lk, const char *const file)
{
	if (!linker_is_correct(lk) || file == NULL)
	{
		return SIZE_MAX;
	}

	char full_path[MAX_ARG_SIZE];

	size_t i = 0;
	const char *dir;
	do
	{
		dir = ws_get_dir(lk->ws, i++);
		lk_make_path(full_path, dir, file, 0);
	} while (dir != NULL && access(full_path, F_OK) == -1);

	if (access(full_path, F_OK) == -1)
	{
		lk_make_path(full_path, ws_get_file(lk->ws, lk->current), file, 1);
	}

	if (access(full_path, F_OK) == -1)
	{
		macro_system_error(full_path, header_file_not_found);
		return SIZE_MAX;
	}

	return ws_add_file(lk->ws, full_path);
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
	if (!linker_is_correct(lk))
	{
		return -1;
	}

	return vector_clear(&lk->included);
}
