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

#pragma once

#include "constants.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct macro_long_string
{
	int *str;
	int p;
	int size;
} macro_long_string;

typedef struct control_string
{
	int str_before[CONTROLSIZE];
	int str_after[CONTROLSIZE];
	int p;
} control_string;

typedef struct data_file
{
	char *name;
	int p;
	int pred;
	int line;
	int include_line;
	control_string cs;
	FILE *input;
	macro_long_string include_source;
	macro_long_string before_source;
} data_file;

typedef struct data_files
{
	data_file *files;
	int p;
	int cur;
	int size;
	int i;
} data_files;

void data_files_init(data_files *s, int num);
void long_string_pinter(macro_long_string *s, int a);
void data_files_pinter(data_files *s, const char *file_way, FILE *input);
void macro_long_string_free(macro_long_string *s);
void data_files_clear(data_files *fs);
void include_source_set(data_files *fs, int temp_p, int p);
#ifdef __cplusplus
} /* extern "C" */
#endif
