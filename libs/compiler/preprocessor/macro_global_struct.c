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


void data_file_pinter(data_file *f, FILE *input)
{
	f->include_line = -1;
	f->input = input;
	f->pred = -1;
}

void data_file_free(data_file *f)
{
	if (f->input != NULL)
	{
		free(f->name);
	}
}

void data_files_clear(data_files *fs)
{
	for (int i = 0; i < fs->p; i++)
	{
		data_file_free(&fs->files[i]);
	}

	free(fs->files);
}

void data_files_init(data_files *s, int num)
{
	s->size = num;
	s->p = 0;
	s->i = 0;
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

	if (input == NULL)
	{
		s->files[s->p].name = file_way;
	}
	else
	{
		s->files[s->p].name = malloc((strlen(file_way) + 1) * sizeof(char));
		strcpy(s->files[s->p].name, file_way);
	}

	data_file_pinter(&s->files[s->p], input);
	s->p++;
}
