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
#include "preprocessor.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include "macro_global_struct.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void print_marcer(int p, preprocess_context *context)
{
	m_fprintf('#',context);
	m_fprintf('1',context);
	int n = 1;
	while(n*10 < p)
	{
		n *=10;
	}
	while(n != 1)
	{
		int c = p % n; 
		c = (p - c)/n;
		p -= c*n;
		c += '0';
		m_fprintf(c,context);
		n /= 10;
	}
	m_fprintf(c,context);
	m_fprintf('\n',context);
}

void update_faile(data_file* faile)
{
	faile->start_control = control->p;
	faile->pred = old_cur;
	faile->start = context->output_options->p / sizeof(char);
}

void marcer_update(preprocess_context *context, control_string *control, data_files* failes)
{
	m_nextch();
	if(context->curchar == '1')
	{
		m_nextch();
		int c = context->curchar - '0'; 
		while(is_digit(context->nextchar))
		{	
			m_nextch();
			c = c*10 + context->curchar - '0';
		}
		m_nextch();

		int old_cur = failes->cur;
		failes->cur = c;
		update_faile(&failes->failes[failes->cur])
		if(curchar != '\n')
			a_erorr

	}
}

void m_fopen(preprocess_context *context,  char* file_way)
{
	context->input_stak[++context->inp_file] = fopen(file_way, "r");
	if (context->input_stak[context->inp_file] == NULL)
	{
		printf(" не найден файл %s\n", file_way);
		exit(1);
	}
	int k = strlen(file_way);
	int j = k - 1;
	while (file_way[j] != '/')
	{
		j--;
	}
	char name[50];
	for(int i = 0; i < k - j; i++)
	{
		name[i] = file_way[j + i];
	}
	j++;
	file_way[j] = '\0';
	int p = data_files_pinter(&context->files, file_way, name);
	print_marcer(p, context);
}

void include_fopen(preprocess_context *context, const char* file_way)
{
	context->input_stak[++context->inp_p] = fopen(file_way, "r");
	if (context->input_stak[context->inp_p] == NULL)
	{
		printf(" не найден файл %s\n", file_way);
		exit(1);
	}
}

void file_read(preprocess_context *context, compiler_context *c_context)
{
	get_next_char(context);
	m_nextch(context, c_context);
	// деректива лайн begin
	while (context->curchar != EOF)
	{
		preprocess_scan(context, c_context);
	}
	// деректива лайн end
	m_fprintf('\n', context, c_context);
	m_fprintf('\n', context, c_context);
	fclose(context->input_stak[context->inp_p]);
	context->inp_p--;
	m_nextch(context, c_context);
	m_nextch(context, c_context);
}

void open_file(preprocess_context *context, compiler_context *c_context)
{
	char file_way[100];
	int i = 0;

	while (context->way[i] != '\0')
	{
		file_way[i] = context->way[i];
		i++;
	}

	while (context->curchar != '\"')
	{
		if (context->curchar == EOF)
		{
			a_erorr(1);
		}

		if (context->curchar == '.' && context->nextchar == '.' && i != 0)
		{
			m_nextch(context, c_context);
			m_nextch(context, c_context);
			if (file_way[i] != '/')
			{
				i--;
			}
			while (file_way[i] != '/' && i > 0)
			{
				i--;
			}

			if (context->curchar == '/')
			{
				m_nextch(context, c_context);
			}
			else
			{
				a_erorr(1);
			}
		}
		else
		{
			file_way[i++] = context->curchar;
			m_nextch(context, c_context);
		}
	}
	file_way[i++] = '\0';

	include_fopen(context, file_way);

	if (file_way[i - 2] == 'h' && file_way[i - 3] == '.')
	{
		if(context->include_type = 1)
		{
			file_read(context, c_context);
		}
		else if (context->include_type = 2)
		{
			a_erorr(70);
		}
	}
	else 
	{
		file_read(context, c_context);
	}	
}



void include_relis(preprocess_context *context, compiler_context *c_context)
{
	int old_inp_p = context->inp_p;
	if (context->curchar != '\"')
	{
		a_erorr(1);
	}
	if (context->inp_p >= 20)
	{
		a_erorr(1); // переполнение или цыкл
	}
	m_nextch(context, c_context);

	open_file(context, c_context);
	//m_nextch(context, c_context);
	space_end_line(context, c_context);
}
