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
#include "context.h"
#include "context_var.h"
#include "file.h"
#include "logger.h"
#include "macro_global_struct.h"
#include "preprocessor.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void update_faile(int old_cur, data_file *cur_f, data_file *old_f, preprocess_context *context)
{
	cur_f->pred = old_cur;
}

void cur_failes_next(data_files *fs, int old_cur, preprocess_context *context)
{
	fs->cur = fs->p - 1;
	update_faile(old_cur, &fs->files[fs->cur], &fs->files[old_cur], context);
}

void set_old_cur(data_files *fs, int old, preprocess_context *context)
{
	fs->cur = old;
}


/*char *gen_way(preprocess_context *context, char *cur_way, char *temp_way)
{
	char *file_way = malloc(STRIGSIZE * sizeof(char));
	//memset(file_way, 0, STRIGSIZE * sizeof(char));

	file_way = cur_way;
	int i = strlen(cur_way);
	int j = 0;

	while (temp_way[j] != '\0')
	{
		if (temp_way[j] == '.' && temp_way[j + 1] == '.' && i != 0)
		{
			j += 2;
			if (file_way[i] != '/')
			{
				i--;
			}
			while (file_way[i] != '/' && i > 0)
			{
				i--;
			}

			if (temp_way[j++] != '/')
			{
				m_error(24, context);
			}
		}
		else
		{
			file_way[i++] = temp_way[j++];
		}
	}

	file_way[i++] = '\0';
	return file_way;
}*/

void open_cur_faile(preprocess_context *context, const char *file_way)
{
	context->current_file = fopen(file_way, "r");
	if (context->current_file == NULL)
	{
		log_system_error(file_way, "файл не найден");
		m_error(just_kill_yourself, context);
	}
}

int open_p_faile(preprocess_context *context, const char *file_way)
{
	context->current_file = fopen(file_way, "r");
	if (context->current_file == NULL)
	{
		log_system_error(file_way, "файл не найден");
		m_error(just_kill_yourself, context);
	}
	data_files_pinter(context->sources, file_way, NULL);

	return context->sources->cur;
}

void gen_way(char *full, const char *path, const char *file, int is_slash)
{
	int size;

	if (is_slash)
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

int open_i_faile(preprocess_context *context, char *temp_way, data_file *fs, int flag)
{
	char file_way[STRIGSIZE + 1024];
	gen_way(file_way, fs->name, temp_way, 1);
	if (!find_file(context, file_way))
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
	if (flag == 0)
	{
		context->current_file = f;
		data_files_pinter(context->sources, file_way, NULL);
		return context->sources->cur;
	}
	else
	{
		data_files_pinter(context->headers, file_way, f);
		return context->headers->cur;
	}
}

void include_fclose(preprocess_context *context)
{
	fclose(context->current_file);
	context->current_file = NULL;
	context->line = 1;
}

void begin_faile(preprocess_context *context, data_files *fs)
{
	context->current_file = (fs->files[fs->cur]).input;
}

void file_read(preprocess_context *context)
{
	int cur;
	if (context->h_flag)
	{
		cur = context->headers->cur;
	}
	else
	{
		cur = context->sources->cur;
	}

	
	get_next_char(context);
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

	include_fclose(context);
}

void open_file(preprocess_context *context, data_file *f)
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
	data_files *fs;

	if (temp_way[i - 1] == 'h' && temp_way[i - 2] == '.')
	{
		h++;
		fs = context->headers;
	}
	else
	{
		fs = context->sources;
	}

	int old_cur;
	if (h || context->include_type != 0)
	{
		old_cur = open_i_faile(context, temp_way, f, h);
		if (old_cur == -2)
		{
			return;
		}
	}

	if (context->include_type == 1 || context->include_type == 2 && !h)
	{
		cur_failes_next(fs, old_cur, context);


		if (h)
		{
			begin_faile(context, context->headers);
		}

		if (!h && context->nextch_type != FILETYPE)
		{
			m_change_nextch_type(FILETYPE, 0, context);
		}

		file_read(context);

		if (!h && context->dipp != 0)
		{
			m_old_nextch_type(context);
		}

		set_old_cur(fs, old_cur, context);

		fs->i++;

	}
	else
	{
		if (h)
		{
			m_error(25, context);
		}
		else
		{
			context->h_flag = 1;
		}
	}
}


void include_relis(preprocess_context *context, data_files *fs)
{
	space_skip(context);

	if (context->curchar != '\"')
	{
		m_error(26, context);
	}
	m_nextch(context);
	open_file(context, &fs->files[fs->cur]);
	m_nextch(context);


	space_end_line(context);
}
