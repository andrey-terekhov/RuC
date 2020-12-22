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

#include "include.h"
#include "constants.h"
#include "context_var.h"
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


void gen_way(char *full, const char *path, const char *file, int is_slash)
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
	// printf("4full = %s, path = %s file = %s \n", full, path, file);
}

int open_include_faile(preprocess_context *context, char *temp_way, const char* f_name)
{
	char file_way[STRING_SIZE + 1024];

	gen_way(file_way, f_name, temp_way, 1);

	universal_io temp_io = io_create();
	int res = in_set_file(&temp_io, file_way);
	if (res == -1)
	{
		int i = 0;
		const char *temp_dir = ws_get_dir(context->fs.ws, i++);
		while (temp_dir != NULL)
		{
		
			gen_way(file_way, temp_dir, temp_way, 0);

			res = in_set_file(&temp_io, file_way);

			if (res == 0)
			{
				break;
			}
			temp_dir = ws_get_dir(context->fs.ws, i++);
		}
	}

	if (res == -1)
	{
		log_system_error(temp_way, "файл не найден");
		return -3;
	}
	else if (!find_file(context, file_way))
	{
		return -2;
	}

	if (context->include_type != 0)
	{
		in_set_file(context->io_input , file_way);
	}

	in_close_file(&temp_io);

	con_files_add_include(&context->fs, file_way, context->include_type);
	return 0;
}

int file_read(preprocess_context *context)
{
	int old_line = context->line;
	context->line = 1;

	get_next_char(context);

	if(context->nextchar != '#')
	{
		con_file_print_coment(&context->fs, context);
	}

	if (context->nextchar == EOF)
	{
		context->curchar = EOF;
	}
	else
	{
		m_nextch(context);
	}
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
	con_file_close_cur(context);
	context->line = old_line;

	
	context->position = 0;
	context->error_string[context->position] = '\0';
	return error;
}

int open_file(preprocess_context *context)
{
	int i = 0;
	char temp_way[STRING_SIZE];
	int rez = 0;

	while (context->curchar != '\"')
	{
		if (context->curchar == EOF)
		{
			size_t position = skip_str(context); 
			macro_error(must_end_quote, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
			return -1;
		}
		temp_way[i++] = (char)context->curchar;
		m_nextch(context);
	}
	temp_way[i] = '\0';
	int h = 0;

	if (temp_way[i - 1] == 'h' && temp_way[i - 2] == '.')
	{
		h++;
	}

	int old_cur = context->fs.cur;
	universal_io new_io = io_create();
	universal_io *io_old = context->io_input;
	context->io_input = &new_io;

	if ((h && context->include_type != 2) || (!h && context->include_type != 0))
	{
		int k = open_include_faile(context, temp_way, ws_get_file(context->fs.ws, context->fs.cur));
		if (k == -2)
		{
			context->fs.cur = old_cur;
			context->io_input = io_old;
			in_clear(&new_io);
			return 0;
		}
		else if(k == -3)
		{
			context->fs.cur = old_cur;
			context->io_input = io_old;
			in_clear(&new_io);
			return -1;
		}
		
	}

	if (context->include_type == 1 || (context->include_type == 2 && !h))
	{
		
		if (!h && context->nextch_type != FILETYPE)
		{
			m_change_nextch_type(FILETYPE, 0, context);
		}
		rez = -file_read(context);

		if (!h && context->dipp != 0)
		{
			m_old_nextch_type(context);
		}
	}

	context->fs.cur = old_cur;
	context->io_input = io_old;
	in_clear(&new_io);
	return rez;
}


int include_relis(preprocess_context *context)
{
	space_skip(context);

	if (context->curchar != '\"')
	{
		if(context->include_type > 0)
		{
			size_t position = skip_str(context); 
			macro_error(must_start_quote, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
			return -1;
		}
		else
		{
			return 0;
		}
		
	}
	m_nextch(context);
	int rez = open_file(context);
	if(rez == 1)
	{
		skip_file(context);
		return 1;
	}
	m_nextch(context);
	space_end_line(context);
	return rez;
}
