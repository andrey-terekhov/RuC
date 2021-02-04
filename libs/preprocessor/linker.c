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
#include "file.h"
#include "logger.h"
#include "preprocessor.h"
#include "error.h"
#include "utils.h"
#include "workspace.h"
#include "uniio.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

linker lk_create(workspace *const ws)
{
	linker lk;

	lk.ws = ws;
	lk.current = MAX_PATHS;
	lk.count = 0;

	for (size_t i = 0; i < MAX_PATHS; i++)
	{
		lk.included[i] = 0;
	}
	return lk;
}

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

size_t lk_open_include(environment *const env, const char* const header_path)
{
	char full_path[MAX_ARG_SIZE];

	lk_make_path(full_path, ws_get_file(env->lk.ws, env->lk.current), header_path, 1);

	in_set_file(env->input, full_path);
	if (!in_is_correct(env->input))
	{
		size_t i = 0;
		const char *dir;

		do  
		{
			dir = ws_get_dir(env->lk.ws, i++);
			lk_make_path(full_path, dir, header_path, 0);
		} while (dir != NULL && in_set_file(env->input, full_path));
		
	}

	if (!in_is_correct(env->input))
	{
		in_clear(env->input);
		log_system_error(full_path, "файл не найден");
		return SIZE_MAX - 1;
	}

	size_t const num = ws_get_files_num(env->lk.ws);	
	size_t index = ws_add_file(env->lk.ws, full_path);

	if (index < env->lk.count && !env->lk.included[index])
	{
		env->lk.included[index]++;
	}
	else if (num == ws_get_files_num(env->lk.ws))
	{
		in_clear(env->input);
		return SIZE_MAX;
	}
	
	return index;
}

int lk_open_source(environment *const env, size_t index)
{
	if (in_set_file(env->input, ws_get_file(env->lk.ws, index)))
	{
		log_system_error(ws_get_file(env->lk.ws, index), "файл не найден");
		return -1;
	}
	return 0;
}

int lk_preprocess_file(environment *const env, const size_t number)
{	
	size_t old_cur = env->lk.current;
	env->lk.current = number;
	size_t old_line = env->line;
	env->line = 1;
	get_next_char(env);
	m_nextch(env);
	if(env->nextchar != '#')
	{
		env_add_comment(env);
	}

	int was_error = 0; 
	while (env->curchar != EOF)
	{
		was_error = preprocess_scan(env) || was_error;
	}

	if (was_error)
	{
		was_error = -1;
	}

	m_fprintf('\n', env);

	env->line = old_line;
	env->lk.current = old_cur;

	in_clear(env->input);

	return was_error;
}

int lk_process_include_file(environment *const env)
{
	char header_path[MAX_ARG_SIZE];
	int i = 0;

	while (env->curchar != '\"')
	{
		if (env->curchar == EOF)
		{
			size_t position = skip_str(env); 
			macro_error(must_end_quote, ws_get_file(env->lk.ws, env->lk.current), env->error_string, env->line, position);
			return -1;
		}

		i += utf8_to_string(&header_path[i], (char32_t)env->curchar);
		m_nextch(env);
	}

	universal_io temp_io = io_create();
	universal_io *io_old = env->input;
	env->input = &temp_io;
		
	const size_t index = lk_open_include(env, header_path);
	if (index >= SIZE_MAX - 1)
	{
		env->input = io_old;
		return index == SIZE_MAX ? 0 : -1;
	}

	int flag_io_type = 0;	
	if (env->nextch_type != FILETYPE)
	{
		m_change_nextch_type(FILETYPE, 0, env);
		flag_io_type++;
	}

	env_clear_error_string(env);
	
	int res = 0;
	res = lk_preprocess_file(env, (size_t)index) * 2;
	env->input = io_old;
	

	if (flag_io_type)
	{
		m_old_nextch_type(env);
	}

	return res;
}

int lk_include(environment *const env)
{
	if (env == NULL)
	{
		return -1;
	}

	skip_space(env);

	if (env->curchar != '\"')
	{
		size_t position = skip_str(env); 
		macro_error(must_start_quote, ws_get_file(env->lk.ws, env->lk.current), env->error_string, env->line, position);
		return -1;	
	}

	m_nextch(env);

	int res = lk_process_include_file(env);
	if (res == -2)
	{
		skip_file(env);
		return res;
	}

	get_next_char(env);
	m_nextch(env);

	return res;
}

int lk_preprocess_all(environment *const env)
{
	if (env == NULL)
	{
		return -1;
	}

	env->lk.count = ws_get_files_num(env->lk.ws);

	for (size_t i = 0; i < env->lk.count; i++)
	{
		if (env->lk.included[i])
		{
			continue;
		}
	
		env->lk.included[i]++;
		
		universal_io in = io_create();
		env->input = &in;
		
		if (lk_open_source(env, i) || lk_preprocess_file(env, i))
		{
			return -1;
		}
	}

	return 0;
}
