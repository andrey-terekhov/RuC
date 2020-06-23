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
	int* str;
	int p;
	int size;
} macro_long_string;

typedef struct control_string
{
	int* str_before;
	int* str_after;
	int p;
	int size;
} control_string;

typedef struct data_file
{
	char* way;
	char* name;
	int p; 
	int pred;
	int line;
	control_string cs; 
	FILE* input;
	int* include_sorse;
	macro_long_string befor_sorse;
} data_file;

typedef struct data_files
{	data_file* files;
	int p;
	int cur;
	int size;
} data_files;

void long_string_pinter(macro_long_string *s, int a);
void data_files_pinter(data_files *s, char* file_way, FILE* input);
void failes_cur_add(data_files *fs);
int get_pred_faile(data_file *s);
int get_cur_faile_start(data_files *fs);
char* get_faile_name(data_file *f);
int* get_befor(data_file *f);
FILE* get_input(data_file *f);
data_file get_cur_faile(data_files *fs);
int get_cur_failes(data_files *fs);
control_string get_control(data_file *f);
void failes_cur_fclose(data_files *fs);
int* get_long_string(macro_long_string *s);
int get_long_string_p(macro_long_string *s);
int get_line(data_file *s);
void set_line(data_file *s, int n);
void include_sorse_set(data_files* fs, int temp_p, int p);
int* get_control_befor(control_string *cs);
int* get_control_after(control_string *cs);

#ifdef __cplusplus
} /* extern "C" */
#endif
