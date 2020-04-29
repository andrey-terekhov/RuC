/*
 *	Copyright 2018 Andrey Terekhov, Egor Anikin
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

#include "preprocess_include.h"
#include <wchar.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocess.h"
#include "preprocess_global.h"
#include "preprocess_defs.h"
#include "preprocess_utils.h"  
#include "preprocess_error.h"
#include "preprocess_nextch.h"


void open_file_c(char file_way[], int i, preprocess_context *context)
{
	//file_way[i-1] == 'c';
	if(1)//!find_reportab())
	{
		return;
	}

	context->input_stak[context->inp_p++] = fopen(file_way, "r");
	if (context->input_stak[context->inp_p - 1] == NULL)
	{
		context->inp_p--;
	}
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

			if(context->curchar == '/')
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

	

	context->input_stak[context->inp_p++] = fopen(file_way, "r");
	if (context->input_stak[context->inp_p - 1] == NULL)
	{
		printf(" не найден файл %s\n", file_way);
		exit(1);
	}

	if(file_way[i-2] == 'h' && file_way[i-3] == '.')
	{
	open_file_c(file_way, i, context);
	}
}

void file_read(preprocess_context *context, compiler_context *c_context)
{
	while (context->curchar != EOF)
	{
		preprocess_scan(context, c_context);
	}
	m_fprintf('\n', context, c_context);
	m_fprintf('\n', context, c_context);
	fclose(context->input_stak[context->inp_file]);
	context->inp_p--; 
	context->inp_file--;
	m_nextch(context, c_context);
	m_nextch(context, c_context);
}

void include_relis(preprocess_context *context, compiler_context *c_context)
{
	int old_inp_p = context->inp_p;
	if (context->curchar != '\"')
	{
		a_erorr(1);
	}
	if(context->inp_p >= 20)
	{
		a_erorr(1);// переполнение или цыкл
	}
	m_nextch(context, c_context);

	open_file(context, c_context);
	m_nextch(context, c_context);

	space_end_line(context, c_context);

	context->inp_file = context->inp_p - 1;

	// деректива лайн begin

	if(old_inp_p + 2 == context->inp_p)
	{
		
		file_read(context, c_context);
		// деректива лайн end

	}

	if(old_inp_p + 1 == context->inp_p)
	{
		file_read(context, c_context);
	}

	if (context->inp_p != old_inp_p)
	{
		a_erorr(1);
	}
}