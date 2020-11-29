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
#include <stdio.h>
#include "workspace.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct reprtab
{
	char32_t tab[MAXTAB];
	int hashtab[HASH];
	int p;
} reprtab;

typedef struct file
{
	const char* name;
	int const_name;
} file;

typedef struct files
{
	file files[MAX_ARG_SIZE*3];
	int main_faile;
	int p;
	int p_s;
	int cur;
	int begin_f;
	int end_h;
} files;

typedef struct preprocess_context
{
	int include_type;

	reprtab repr;

	int macrotext[MAXTAB];
	int mp;

	int strp;

	int fchange[STRIGSIZE * 3];
	int cp;

	int localstack[STRIGSIZE];
	int lsp;

	int cstring[STRIGSIZE];
	int csp;

	int ifstring[STRIGSIZE * 2];
	int ifsp;

	int wstring[STRIGSIZE * 5];
	int wsp;

	int mfirstrp;

	int mclp;

	int prep_flag;

	int curchar, nextchar;
	int nextch_type;
	int cur;

	int nextp;
	int main_file;

	int oldcurchar[DIP];
	int oldnextchar[DIP];
	int oldnextch_type[DIP];
	int oldnextp[DIP];
	int dipp;

	size_t line;

	int temp_output;
	files fs;
	int h_flag;

	FILE *current_file;
	int *current_string;
	int current_p;

	const char *include_ways[MAX_FLAGS];
	int iwp;

	universal_io io;
} preprocess_context;

void con_init(preprocess_context *context);


int con_repr_add(reprtab *repr, char* s, int cod);
void con_repr_add_ident(reprtab *repr, preprocess_context *context);
int con_repr_find(reprtab* repr, char32_t* s);
void con_repr_change(reprtab *repr, preprocess_context *context);

void con_files_add_parametrs(files* fs, const char *name);
void con_files_add_include(files* fs, const char *name);

int con_file_open_main(files* fs, preprocess_context *context);
int con_file_open_sorse(files* fs, preprocess_context *context);
int con_file_open_hedrs(files* fs, preprocess_context *context);
int con_file_open_next(files* fs, preprocess_context *context, int h_flag);
void con_file_close_cur(preprocess_context *context);

void con_file_it_is_main(files *fs);
void con_file_it_is_end_h(files *fs);

void con_file_print_coment(files *fs, preprocess_context *context);

#ifdef __cplusplus
} /* extern "C" */
#endif
