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

#include "macro_global_struct.h"
#include "context_var.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void  macro_long_string_init(macro_long_string* s)
{
	s->size = LONGSTR;
	s->p = 0;
	s->str = malloc(s->size * sizeof(int));
}

void long_string_pinter(macro_long_string *s, int a)
{
	if (s->str != NULL)
	{
		if (s->p == s->size)
		{
			s->size *= 2;
			int *reallocated = realloc(s->str, s->size * sizeof(int));
			s->str = reallocated;
		}
		s->str[s->p++] = a;
	}
}

void macro_long_string_free(macro_long_string *s)
{
	free(s->str);
}


void data_file_pinter(data_file *f, FILE *input)
{
	f->cs.p = 0;

	macro_long_string_init(&f->befor_sorse);
	macro_long_string_init(&f->include_sorse);
	
	f->include_sorse.str[0] = 0;
	f->input = input;
	f->pred = -1;
}

void data_file_free(data_file *f)
{
	macro_long_string_free(&f->befor_sorse);
	macro_long_string_free(&f->include_sorse);
}

void data_files_clear(data_files *fs)
{
	for(int i = 0; i < fs->p; i++)
	{
		data_file_free(&fs->files[i]);
	}

	free(fs->files);
}

void data_files_init(data_files *s, int num)
{
	s->size = num;
	s->p = 0;
	s->cur = -1;
	s->files = malloc(num * sizeof(data_file));
}

void data_files_pinter(data_files *s, const char *file_way, FILE *input)
{
	if (s->p == s->size)
	{
		s->size *= 2;
		data_file *reallocated = realloc(s->files, s->size * sizeof(data_file));
		s->files = reallocated;
	}
	
	s->files[s->p].name = file_way;

	data_file_pinter(&s->files[s->p], input);
	s->p++;
}

void include_sorse_set(data_files *fs, int temp_p, int p)
{
	data_file *f = &fs->files[fs->cur];
	int *str_i = f->include_sorse.str;
	int *str_b = f->befor_sorse.str;

	if (temp_p == -1)
	{
		str_b[0] = str_i[p - 1];
		str_b[1] = '\0';
		str_i[p - 1] = '\0';

		f->befor_sorse.p = 1;
	}
	else
	{
		for (int i = 0; i < p - temp_p + 1; i++)
		{
			str_b[i] = str_i[temp_p + i - 1];
		}
		str_b[p - temp_p + 1] = '\0';
		str_i[temp_p] = '\0';
		f->befor_sorse.p = p - temp_p + 1;
	}
}
