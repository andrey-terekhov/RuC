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


void long_string_pinter(macro_long_string *s, int a)
{
	if (s->str != NULL)
	{
		if (s->p == s->size - 1)
		{
			s->size *= 2;
			int *reallocated = realloc(s->str, s->size * sizeof(int));
			memset(&reallocated[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
			s->str = reallocated;
		}
		s->str[s->p++] = a;
	}
}

void macro_long_string_init1(macro_long_string *s)
{
	s->size = 10000;
	s->p = 0;
	s->str = malloc(s->size * sizeof(int));
	memset(s->str, 0, s->size * sizeof(int));
}

char *get_faile_name(data_file *f)
{
	return f->name;
}

int *get_befor(data_file *f)
{
	return get_long_string(&f->befor_sorse);
}

void control_string_init(control_string *s)
{
	s->size = 100;
	s->p = 0;
	s->str_before = malloc(s->size * sizeof(int));
	s->str_after = malloc(s->size * sizeof(int));
	memset(s->str_before, 0, s->size * sizeof(int));
	memset(s->str_after, 0, s->size * sizeof(int));
}

void data_file_pinter(data_file *f, char *way, FILE *input)
{
	int i = 0;
	int z = strlen(way) + 1;
	f->way = malloc(z * sizeof(char));
	memset(f->way, 0, z * sizeof(char));
	f->way = way;

	while (way[i] != '\0')
	{
		f->way[i] = way[i];
		i++;
	}
	f->way[i] = '\0';

	control_string_init(&f->cs);

	macro_long_string_init1(&f->befor_sorse);

	f->include_sorse = malloc(300 * sizeof(int));
	memset(f->include_sorse, 0, 300 * sizeof(int));
	f->include_sorse[0] = 0;

	f->input = input;
	f->pred = -1;
}

void set_name(data_file *f, const char *name)
{
	int i = 0;
	int z = strlen(name) + 1;
	f->name = malloc(z * sizeof(char));
	memset(f->name, 0, z * sizeof(char));

	while (name[i] != '\0')
	{
		f->name[i] = name[i];
		i++;
	}
	f->name[i] = '\0';
}

int *get_control_befor(control_string *cs)
{
	return cs->str_before;
}

int *get_control_after(control_string *cs)
{
	return cs->str_after;
}

void data_files_pinter(data_files *s, char *file_way, FILE *input)
{
	if (s->p == s->size - 1)
	{
		s->size *= 2;
		data_file *reallocated = realloc(s->files, s->size * sizeof(data_file));
		memset(&reallocated[s->size * sizeof(data_file)], 0, (s->size / 2) * sizeof(data_file));
		s->files = reallocated;
	}
	set_name(&s->files[s->p], file_way);
	int k = strlen(file_way) - 1;
	while (file_way[k] != '/')
	{
		k--;
	}
	k++;
	file_way[k] = '\0';
	data_file_pinter(&s->files[s->p], file_way, input);
	s->p++;
}

void failes_cur_add(data_files *fs)
{
	fs->cur++;
}

int get_pred_faile(data_file *s)
{
	return s->pred;
}

int get_line(data_file *s)
{
	return s->line;
}

void set_line(data_file *s, int n)
{
	s->line = n;
}

int get_faile_start(data_file *f)
{
	return f->p;
}

int get_cur_faile_start(data_files *fs)
{
	return get_faile_start(&fs->files[fs->cur]);
}

int *get_long_string(macro_long_string *s)
{
	return s->str;
}

int get_long_string_p(macro_long_string *s)
{
	return s->p;
}

control_string get_control(data_file *f)
{
	return f->cs;
}

FILE *get_input(data_file *f)
{
	return f->input;
}


data_file get_cur_faile(data_files *fs)
{
	return fs->files[fs->cur];
}

int get_cur_failes(data_files *fs)
{
	return fs->cur;
}

void failes_fclose(data_file *f)
{
	fclose(f->input);
	f->input = NULL;
}

void failes_cur_fclose(data_files *fs)
{
	failes_fclose(&fs->files[fs->cur]);
}

void set_incude_s(data_file *f, int temp_p, int p)
{
	int *str_i = f->include_sorse;
	int *str_b = (&f->befor_sorse)->str;
	if (temp_p == -1)
	{
		str_b[0] = str_i[p - 1];
		str_b[1] = '\0';
		str_i[p - 1] = '\0';

		(&f->befor_sorse)->p = 1;
	}
	else
	{
		for (int i = 0; i < p - temp_p; i++)
		{
			str_b[i] = str_i[temp_p + i];
		}
		str_b[p - temp_p] = '\0';
		str_i[temp_p] = '\0';
		(&f->befor_sorse)->p = p - temp_p;
	}
}

void include_sorse_set(data_files *fs, int temp_p, int p)
{
	set_incude_s(&fs->files[fs->cur], temp_p, p);
}