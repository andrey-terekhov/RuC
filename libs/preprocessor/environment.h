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
#include "uniio.h"

#ifdef __cplusplus
extern "C" {
#endif

enum IO_TYPE
{
	file_type = -1,
	macro_text_type,
	param_type,
	if_type,
	while_type,
	all_types,
};

typedef struct environment
{
	
	//local io struct
	int *local_io[all_types];
	size_t local_io_size[all_types];

	int curent_io_type;
	int *curent_io;
	int curent_io_prt;

	int macro_tab[MAXTAB];
	int change[STRING_SIZE * 3];

	int define_stack[STRING_SIZE];
	size_t define_stack_prt;

	int old_nextchar;
	int old_io_type[DEPTH];
	int *old_io[DEPTH];
	size_t depth_io;
	//end local io struct

	int hashtab[256];
	int reprtab[MAXTAB];
	int rp;

	char error_string[STRING_SIZE];
	size_t position;

	int curchar, nextchar;
	int cur;

	int nested_if;
	int prep_flag;

	const char *curent_path;

	size_t line;

	universal_io *output;
	universal_io *input;
} environment;

void env_init(environment *const env, universal_io *const output);
void env_clear_error_string(environment *const env);

/**
 *	Add a comment to indicate line changes in the output
 *
 *	@param	env	Preprocessor environment
 */
void env_add_comment(environment *const env);

int env_get_io_type(environment *const env);

size_t env_get_io_size(environment *const env, int type);

int env_set_define_stack_add(environment *const env, int num);

int env_clear_param_define_stack(environment *const env);

int env_get_define_stack_prt(environment *const env);

int env_clear_param(environment *const env, size_t size);

int env_set_define_stack_prt(environment *const env, int num);

int env_io_add_char(environment *const env, int type, int simbol);

int env_io_switch_to_new_type(environment *const env, int type, int prt);
void env_io_switch_to_old_type(environment *const env);

int env_get_depth_io(environment *const env);
int get_next_char(environment *const env);

void m_fprintf(environment *const env, int a);
void m_nextch(environment *const env);

void env_error(environment *const env, const int num);

#ifdef __cplusplus
} /* extern "C" */
#endif
