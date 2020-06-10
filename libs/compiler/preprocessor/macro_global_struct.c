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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void long_string_pinter(macro_long_string *s, int a)
{
	if(s->p == s->size - 1)
	{
		s->size *= 2;
		int *reallocated = realloc(s->str, s->size * sizeof(int));
		memset(&reallocated[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		s->str = reallocated;
	} 
	s->str[s->p++] = a;
}

void control_string_pinter(control_string *s, int before, int after)
{
	if(s->p == s->size - 1)
	{
		s->size *= 2;
		int *reallocated_b = realloc(s->str_before, s->size * sizeof(int));
		int *reallocated_a = realloc(s->str_after, s->size * sizeof(int));
		memset(&reallocated_b[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		memset(&reallocated_a[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		s->str_before = reallocated_b;
		s->str_after = reallocated_a;
	} 
	s->str_before[s->p] = before;
	s->str_after[s->p] = after;
	s->p++;
}
void data_file_pinter(data_file *s, char* name, char* way)
{
	s->name = name;
	s->way = way;
}

int data_files_pinter(data_files *s, char* name, char* way)
{
	if(s->p == s->size - 1)
	{
		s->size *= 2;
		int *reallocated = realloc(s->files, s->size * sizeof(int));
		memset(&reallocated[s->size * sizeof(int)], 0, (s->size / 2) * sizeof(int));
		s->files = reallocated;
	} 
	data_file_pinter(&s->files[s->p], name, way);
	s->p++;
	return s->p - 1;
}