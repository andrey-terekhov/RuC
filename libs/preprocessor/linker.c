/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
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
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "parser.h"
#include "utils.h"
#include "uniio.h"
#include "uniprinter.h"
#include <string.h>


void lk_make_path(char *const output, const char *const source, const char *const header, const int is_slash)
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

size_t lk_find_include(linker *const lk, const char* const path)
{
	char full_path[MAX_ARG_SIZE];
	lk_make_path(full_path, ws_get_file(lk->ws, lk->current), path, 1);

	universal_io temp_io = io_create();
	if (in_set_file(&temp_io, full_path))
	{
		size_t i = 0;
		const char *dir;
		do
		{
			dir = ws_get_dir(lk->ws, i++);
			lk_make_path(full_path, dir, path, 0);
		} while (dir != NULL && in_set_file(&temp_io, full_path));
	}

	if (!in_is_correct(&temp_io))
	{
		in_clear(&temp_io);
		macro_system_error(full_path, include_file_not_found);
		return SIZE_MAX - 1;
	}

	const size_t index = ws_add_file(lk->ws, full_path);
	in_clear(&temp_io);
	if (index == lk->count)
	{
		lk->included[lk->count++] = 0;
	}
	else if (lk->included[index])
	{
		return SIZE_MAX;
	}

	return index;
}

size_t lk_preprocess_include(linker *const lk)
{
	environment *env = lk->env;
	char header_path[MAX_ARG_SIZE];

	size_t i = 0;
	while (env_get_curchar(env) != '\"')
	{
		if (env_get_curchar(env) == (char32_t)EOF)
		{
			env_error(env, must_end_quote);
			return SIZE_MAX - 1;
		}

		i += utf8_to_string(&header_path[i], env_get_curchar(env));
		env_scan_next_char(env);
	}

	return lk_find_include(lk, header_path);
}

linker lk_create(workspace *const ws, environment *const env)
{
	linker lk;

	lk.env = env;
	lk.ws = ws;
	lk.current = MAX_PATHS;
	lk.count = ws_get_files_num(ws);

	for (size_t i = 0; i < lk.count; i++)
	{
		lk.included[i] = 0;
	}

	return lk;
}

int lk_open_file(linker *const lk, const size_t index)
{
	if (in_set_file(env_get_file_input(lk->env), ws_get_file(lk->ws, index)))
	{
		macro_system_error(ws_get_file(lk->ws, lk->current), source_file_not_found);
		return -1;
	}

	return 0;
}

size_t lk_include(linker *const lk)
{
	environment *env = lk->env;
	if (env == NULL)
	{
		return SIZE_MAX - 1;
	}

	skip_separators(env);

	if (env_get_curchar(env) != '\"')
	{
		env_error(env, must_start_quote);
		return SIZE_MAX - 1;
	}

	env_scan_next_char(env);

	return lk_preprocess_include(lk);
}

const char *lk_get_path(const linker *const lk, size_t num)
{
	return (lk == NULL && num <= ws_get_files_num(lk->ws)) ? NULL : ws_get_file(lk->ws, num);
}

size_t lk_get_count(const linker *const lk)
{
	return lk == NULL ? 0 : lk->count;
}

int lk_is_included(const linker *const lk, size_t index)
{
	return lk == NULL || index > MAX_PATHS ? 0 : lk->included[index];
}

size_t lk_get_current(const linker *const lk)
{
	return lk == NULL ? MAX_PATHS : lk->current;
}

int lk_set_current(linker *const lk, size_t index)
{
	if (lk == NULL || index > MAX_PATHS)
	{
		return -1;
	}

	lk->current = index;

	if (!lk->included[index])
	{
		lk->included[index]++;
	}
	return 0;
}
