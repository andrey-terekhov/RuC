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

#include "multi_file.h"
#include "constants.h"
#include "context_var.h"
#include "file.h"
#include "logger.h"
#include "preprocessor.h"
#include "error.h"
#include "utils.h"
#include "workspace.h"
#include "uniio.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void mf_gen_way(char *full, const char *path, const char *file, int is_slash)
{
	int size;

	if (is_slash)
	{
		char *slash = strrchr(path, '/');
		if (slash != NULL)
		{
			size = (int)(slash - path);
			memcpy(full, path, size * sizeof(char));
			full[size++] = '/';
		}
		else
		{
			size = 0;
		}
	}
	else
	{
		size = (int)strlen(path);
		memcpy(full, path, size * sizeof(char));
		full[size++] = '/';
	}

	int file_size = (int)strlen(file);
	memcpy(&full[size], file, file_size * sizeof(char));
	full[size + file_size] = '\0';
}

int mf_open_include_faile(preprocess_context *context, const char* const path, const char* const temp_way, universal_io* temp_io)
{
	char full_path[MAX_ARG_SIZE];

	mf_gen_way(full_path, path, temp_way, 1);

	int res = in_set_file(temp_io, full_path);
	if (res == -1)
	{
		int i = 0;
		const char *temp_dir = ws_get_dir(context->fs.ws, i++);
		while (temp_dir != NULL)
		{
			mf_gen_way(full_path, temp_dir, temp_way, 0);
			res = in_set_file(temp_io, full_path);
			if (res == 0)
			{
				break;
			}
			
			temp_dir = ws_get_dir(context->fs.ws, i++);
		}
	}

	if (res == -1)
	{
		log_system_error(full_path, "файл не найден");

		return -1;
	}

	size_t num = ws_get_files_num(context->fs.ws);	
	size_t index = ws_add_file(context->fs.ws, full_path);

	if(index < context->fs.end_sorse && !context->fs.already_included[index])
	{
		context->fs.already_included[index]++;
	}
	else if(num == ws_get_files_num(context->fs.ws))
	{
		return -2;
	}
	
	return index;
}

int mf_preprocess_file(preprocess_context *context, const size_t new_cur_file)
{	
	size_t old_cur = context->fs.cur;
	context->fs.cur = new_cur_file;
	size_t old_line = context->line;
	context->line = 1;

	get_next_char(context);

	if(context->nextchar != '#')
	{
		con_file_print_coment(context);
	}

	if (context->nextchar == EOF)
	{
		context->curchar = EOF;
	}
	else
	{
		m_nextch(context);
	}

	context->position = 0;
	context->error_string[context->position] = '\0';

	int error = 0; 
	while (context->curchar != EOF)
	{
		error = preprocess_scan(context) || error;
	}

	if(error)
	{
		skip_file(context);
		error = -1;
	}

	m_fprintf('\n', context);

	context->line = old_line;
	context->fs.cur = old_cur;
	
	context->position = 0;
	context->error_string[context->position] = '\0';

	return error;
}

int mf_open_file(preprocess_context *context)
{
	int i = 0;
	char temp_way[MAX_ARG_SIZE];
	int res = 0;

	while (context->curchar != '\"')
	{
		if (context->curchar == EOF)
		{
			size_t position = skip_str(context); 
			macro_error(must_end_quote, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			return -1;
		}

		temp_way[i++] = (char)context->curchar;
		m_nextch(context);
	}

	temp_way[i] = '\0';

	universal_io temp_io = io_create();
		
	int index = mf_open_include_faile(context, ws_get_file(context->fs.ws, context->fs.cur), temp_way, &temp_io);
	if (index == -2)
	{
		in_clear(&temp_io);
		return 0;
	}
	else if(index == -1)
	{
		in_clear(&temp_io);
		return -1;
	}

	int  dipp = 0;	
	if (context->nextch_type != FILETYPE)
	{
		m_change_nextch_type(FILETYPE, 0, context);
		dipp++;
	}

	universal_io *io_old = context->io_input;
	context->io_input = &temp_io;
	
	res = -mf_preprocess_file(context, (size_t)index);

	context->io_input = io_old;
	in_clear(&temp_io);

	if (dipp)
	{
		m_old_nextch_type(context);
	}

	return res;
}

int mf_file_open(preprocess_context *context, size_t index)
{
	if (in_set_file(context->io_input, ws_get_file(context->fs.ws, index)))
	{
		log_system_error(ws_get_file(context->fs.ws, index), "файл не найден");
		return -1;
	}
	return 0;
}

int mf_preprocess_files(preprocess_context *context)
{
	size_t num = ws_get_files_num(context->fs.ws);
	context->fs.end_sorse = num;

	for (size_t i = 0; i < num; i++)
	{
		if(context->fs.already_included[i])
		{
			continue;
		}
		else
		{
			context->fs.already_included[i]++;
		}
		

		if(mf_file_open(context, i))
		{
			return -1;
		}
		else
		{
			if(mf_preprocess_file(context, i))
			{
				return -1;
			}
		}
	}

	return 0;
}

int mf_include_relis(preprocess_context *context)
{
	space_skip(context);

	if (context->curchar != '\"')
	{
		size_t position = skip_str(context); 
		macro_error(must_start_quote, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;	
	}

	m_nextch(context);

	int res = mf_open_file(context);

	if(res == 1)
	{
		skip_file(context);

		return 1;
	}

	get_next_char(context);
	m_nextch(context);

	return res;
}
