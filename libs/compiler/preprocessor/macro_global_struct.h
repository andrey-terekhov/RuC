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
	int *str_before;
	int *str_after;
	int p;
	int size;
} control_string;

typedef struct data_file
{
	char* way;
	char* name;
	int start; 
	int pred;
	int start_control; 
} data_file;

typedef struct data_files
{	data_file* files;
	int p;
	int cur;
	int size;
} data_files;

void long_string_pinter(macro_long_string *s, int a);
void control_string_pinter(control_string *s, int before, int after);
int data_files_pinter(data_files *s, char* name, char* way);

#ifdef __cplusplus
} /* extern "C" */
#endif
