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
	char32_t *local_io[all_types];
	size_t local_io_size[all_types];

	int curent_io_type;
	char32_t *curent_io;
	size_t curent_io_prt;

	char32_t macro_tab[MAXTAB];
	char32_t param[STRING_SIZE * 3];

	size_t define_stack[STRING_SIZE];
	size_t define_stack_prt;

	char32_t old_nextchar;
	char32_t old_curchar;
	int old_io_type[DEPTH];
	char32_t *old_io[DEPTH];
	size_t depth_io;

	char32_t curchar, nextchar;
	//end local io struct

	int hashtab[256];
	int reprtab[MAXTAB];
	int rp;

	char error_string[STRING_SIZE];
	size_t position;
	const char *curent_path;

	
	int cur;
	int nested_if;
	int prep_flag;
	size_t line;

	universal_io *input;
	universal_io *output;
	
} environment;

void env_init(environment *const env, universal_io *const output);

int env_set_file_input(environment *const env, universal_io *input);
universal_io *env_get_file_input(environment *const env);

int env_io_set(environment *const env, char32_t *io, int type);
int env_io_switch_to_new_type(environment *const env, int type, size_t prt);
void env_io_switch_to_old_type(environment *const env);
int env_io_clear(environment *const env, int type);//

int env_io_add_char(environment *const env, int type, char32_t simbol);
int env_io_set_char(environment *const env, int type, size_t prt, char32_t simbol);
char32_t env_io_get_char(environment *const env, int type, size_t prt);
char32_t env_get_curchar(environment *const env);
char32_t env_get_nextchar(environment *const env);
void m_nextch(environment *const env);

int env_io_get_type(environment *const env);
size_t env_io_get_size(environment *const env, int type);
int env_io_get_depth(environment *const env);
size_t env_io_get_prt(environment *const env);

int env_clear_param_define_stack(environment *const env);//
int env_clear_param(environment *const env, size_t size);//

int env_macro_ident_end(environment *const env);//


int env_set_define_stack_add(environment *const env, int num);//
size_t env_get_define_stack_prt(environment *const env);//
int env_set_define_stack_prt(environment *const env, int num);//

int env_curchar_set(environment *const env, char32_t c);//

void env_clear_error_string(environment *const env);//
void env_error(environment *const env, const int num);

char32_t get_next_char(environment *const env);//



void m_fprintf(environment *const env, int a);
/**
 *	Add a comment to indicate line changes in the output
 *
 *	@param	env	Preprocessor environment
 */
void env_add_comment(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
