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
#include "parser.h"
#include "constants.h"
#include "context_var.h"
#include "file.h"
#include "logger.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


void gen_way(char *full, const char *path, const char *file, int is_slash)
{
	int size;

	if (is_slash != 0)
	{
		char *slash = strrchr(path, '/');
		if (slash != NULL)
		{
			size = slash - path;
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
		size = strlen(path);
		memcpy(full, path, size * sizeof(char));
		full[size++] = '/';
	}

	int file_size = strlen(file);
	memcpy(&full[size], file, file_size * sizeof(char));
	full[size + file_size] = '\0';
	// printf("4full = %s, path = %s file = %s \n", full, path, file);
}

int open_include_faile(preprocess_context *context, char *temp_way, file *fl, int flag)
{
	char file_way[STRIGSIZE + 1024];

	gen_way(file_way, fl->name, temp_way, 1);

	if (con_repr_add(&context->repr, file_way, SH_FILE) == 0)
	{
		return -2;
	}

	FILE *f = fopen(file_way, "r");

	if (f == NULL)
	{
		for (int i = 0; i < context->iwp; i++)
		{
			gen_way(file_way, context->include_ways[i], temp_way, 0);

			f = fopen(file_way, "r");

			if (f != NULL)
			{
				break;
			}
		}
	}

	if (f == NULL)
	{
		log_system_error(temp_way, "файл не найден");
		m_error(1, context);
	}

	if (flag == 0 || context->include_type > 0)
	{
		context->current_file = f;
	}
	else
	{
		fclose(f);
	}
	
	con_files_add_include(&context->fs, file_way);
	return 0;
}

void file_read(preprocess_context *context)
{
	int old_line = context->line;
	context->line = 2;

	get_next_char(context);

	if(context->nextchar != '#')
	{
		con_file_print_coment(&context->fs, context);
	}
	context->line = 1;

	if (context->nextchar == EOF)
	{
		context->curchar = EOF;
	}
	else
	{
		m_nextch(context);
	}

	while (context->curchar != EOF)
	{
		preprocess_scan(context);
	}
	m_fprintf('\n', context);
	con_file_close_cur(context);
	context->line = old_line;
}

void open_file(preprocess_context *context)
{
	int i = 0;
	char temp_way[STRIGSIZE];

	while (context->curchar != '\"')
	{
		if (context->curchar == EOF)
		{
			m_error(23, context);
		}
		temp_way[i++] = context->curchar;
		m_nextch(context);
	}
	temp_way[i] = '\0';
	int h = 0;

	if (temp_way[i - 1] == 'h' && temp_way[i - 2] == '.')
	{
		h++;
	}

	int old_cur = context->fs.cur;
	FILE* file_old = context->current_file;
	if ((h && context->include_type != 2) || (!h && context->include_type != 0))
	{
		int k = open_include_faile(context, temp_way, &context->fs.files[context->fs.cur], h);
		if (k == -2)
		{
			return;
		}
	}
	if (context->include_type == 1 || context->include_type == 2 && !h)
	{
		
		if (!h && context->nextch_type != FILETYPE)
		{
			m_change_nextch_type(FILETYPE, 0, context);
		}
		file_read(context);

		if (!h && context->dipp != 0)
		{
			m_old_nextch_type(context);
		}
	}

	context->fs.cur = old_cur;
	context->current_file = file_old;
}


void include_relis(preprocess_context *context)
{
	space_skip(context);

	if (context->curchar != '\"')
	{
		return;
	}
	m_nextch(context);
	open_file(context);
	m_nextch(context);


	space_end_line(context);
}
