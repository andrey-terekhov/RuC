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
	old_f->line = context->control_bflag;
	context->control_bflag = 0;
}

void returned_faile(data_file *cur_f, preprocess_context *context)
{
	context->control_bflag = cur_f->line;
}

void cur_failes_next(data_files *fs, int old_cur, preprocess_context *context)
{
	fs->cur = fs->p - 1;
	update_faile(old_cur, &fs->files[fs->cur], &fs->files[old_cur], context);
}

void set_old_cur(data_files *fs, int old, preprocess_context *context)
{
	fs->cur = old;
	returned_faile(&fs->files[fs->cur], context);
}

void print_marcer(int p, preprocess_context *context)
{
	m_fprintf('#', context);
	m_fprintf('1', context);
	int n = 1;
	int c = 0;
	while (n * 10 < p)
	{
		n *= 10;
	}
	while (n != 1)
	{
		c = p % n;
		c = (p - c) / n;
		p -= c * n;
		c += '0';
		m_fprintf(c, context);
		n /= 10;
	}
	p += '0';
	m_fprintf(p, context);
	m_fprintf('\n', context);
}

void print_end_marcer(preprocess_context *context)
{
	m_fprintf('\n', context);
	m_fprintf('#', context);
	m_fprintf('2', context);
	m_fprintf('\n', context);
}

char *gen_way(preprocess_context *context, char *cur_way, char *temp_way)
{
	char *file_way = malloc(STRIGSIZE * sizeof(char));
	memset(file_way, 0, STRIGSIZE * sizeof(char));

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
}

int open_p_faile(preprocess_context *context, char *file_way)
{
	context->curent_file = fopen(file_way, "r");
	if (context->curent_file == NULL)
	{
		printf("1 не найден файл %s\n", file_way);
		exit(1);
	}
	data_files_pinter(&context->c_files, file_way, NULL);

	return (&context->c_files)->cur;
}

int open_i_faile(preprocess_context *context, char *temp_way, char *cur_way, int flag)
{
	char *file_way = gen_way(context, cur_way, temp_way);
	FILE *f = fopen(file_way, "r");

	if (f == NULL)
	{
		for (int i = 0; i < context->iwp; i++)
		{
			file_way = gen_way(context, context->include_ways[i], temp_way);
			f = fopen(file_way, "r");
			if (f != NULL)
			{
				break;
			}
		}
	}
	if (f == NULL)
	{
		printf("2 не найден файл %s\n", temp_way);
		exit(1);
	}
	if (flag == 0)
	{
		context->curent_file = f;
		data_files_pinter(&context->c_files, file_way, NULL);
		return (&context->c_files)->cur;
	}
	else
	{
		data_files_pinter(&context->h_files, file_way, f);
		return (&context->h_files)->cur;
	}
}

void include_fclose(preprocess_context *context)
{
	fclose(context->curent_file);
	context->curent_file = NULL;
	context->line = 1;
}

void begin_faile(preprocess_context *context, data_files *fs)
{
	context->curent_file = get_input(&fs->files[fs->cur]);
}

void file_read(preprocess_context *context)
{
	int cur;
	if (context->h_flag)
	{
		cur = get_cur_failes(&context->h_files);
	}
	else
	{
		cur = get_cur_failes(&context->c_files);
	}

	print_marcer(cur, context);
	if (context->FILE_flag)
	{
		get_next_char(context);
		m_nextch(context);
	}
	else
	{
		context->curent_p = 0;
		m_nextch(context);
		m_nextch(context);
	}


	while (context->curchar != EOF)
	{
		preprocess_scan(context);
	}

	control_string_pinter(context, context->control_bflag + 1, 1);
	context->control_bflag = 0;

	print_end_marcer(context);

	if (context->FILE_flag)
	{
		include_fclose(context);
	}
}

void open_file(preprocess_context *context, data_file *f)
{
	int i = 0;
	char *temp_way = malloc(STRIGSIZE * sizeof(char));
	memset(temp_way, 0, STRIGSIZE * sizeof(char));

	while (context->curchar != '\"')
	{
		if (context->curchar == EOF)
		{
			m_error(23, context);
		}
		temp_way[i++] = context->curchar;
		m_nextch(context);
	}
	temp_way[i++] = '\0';

	if (temp_way[i - 2] == 'h' && temp_way[i - 3] == '.')
	{
		if (context->include_type == 0)
		{
			open_i_faile(context, temp_way, f->way, 1);
			context->befor_temp_p = -1;
		}
		else if (context->include_type == 1)
		{
			int old_cur = open_i_faile(context, temp_way, f->way, 1);

			(&(&(&context->h_files)->files[(&context->h_files)->cur])->befor_sorse)->p = (&context->befor_temp)->p;
			(&(&(&context->h_files)->files[(&context->h_files)->cur])->befor_sorse)->size =
				(&context->befor_temp)->size;

			cur_failes_next(&context->h_files, old_cur, context);
			(&context->befor_temp)->str = (&(&(&context->h_files)->files[(&context->h_files)->cur])->befor_sorse)->str;
			(&context->befor_temp)->p = 0;

			begin_faile(context, &context->h_files);
			file_read(context);
			set_old_cur(&context->h_files, old_cur, context);

			(&context->befor_temp)->str = (&(&(&context->h_files)->files[(&context->h_files)->cur])->befor_sorse)->str;
			(&context->befor_temp)->p = (&(&(&context->h_files)->files[(&context->h_files)->cur])->befor_sorse)->p;
			(&context->befor_temp)->size =
				(&(&(&context->h_files)->files[(&context->h_files)->cur])->befor_sorse)->size;
		}
		else
		{
			m_error(25, context);
		}
	}
	else
	{
		if (context->include_type != 0)
		{
			context->FILE_flag = 1;
			int old_cur = open_i_faile(context, temp_way, f->way, 0);

			(&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->p = (&context->befor_temp)->p;
			(&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->size =
				(&context->befor_temp)->size;

			cur_failes_next(&context->c_files, old_cur, context);
			(&context->befor_temp)->str = (&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->str;
			(&context->befor_temp)->p = 0;

			file_read(context);

			set_old_cur(&context->c_files, old_cur, context);

			(&context->befor_temp)->str = (&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->str;
			(&context->befor_temp)->p = (&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->p;
			(&context->befor_temp)->size =
				(&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->size;
			context->FILE_flag = 0;
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
	if (context->inp_p >= 20)
	{
		m_error(27, context);
		 // переполнение или цыкл
	}
	m_nextch(context);

	open_file(context, &fs->files[fs->cur]);
	m_nextch(context);


	space_end_line(context);
}
