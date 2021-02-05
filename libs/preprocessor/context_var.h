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
#include "workspace.h"
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct files
{
	workspace *ws;
	int p;
	int cur;
	int begin_f;
	int end_h;
} files;

typedef struct preprocess_context
{
	int include_type;

	int hashtab[256];
	int reprtab[MAXTAB];
	int rp;

	int macrotext[MAXTAB];
	int mp;

	char error_string[STRING_SIZE];
	size_t position;

	int mstring[STRING_SIZE];
	int msp;

	int fchange[STRING_SIZE * 3];
	int cp;

	int localstack[STRING_SIZE];
	int lsp;

	int cstring[STRING_SIZE];
	int csp;

	int ifstring[STRING_SIZE * 2];
	int ifsp;

	int wstring[STRING_SIZE * 5];
	int wsp;

	int mfirstrp;

	int prep_flag;

	int curchar, nextchar;
	int nextch_type;
	int cur;

	int nextp;

	int oldcurchar[DIP];
	int oldnextchar[DIP];
	int oldnextch_type[DIP];
	int oldnextp[DIP];
	int dipp;

	size_t line;

	files fs;
	int h_flag;

	universal_io *io_output;
	universal_io *io_input;
} preprocess_context;

void preprocess_context_init(preprocess_context *context, workspace *const ws, universal_io *const io, universal_io *const io_input);

void con_files_add_include(files* fs, char *name, int h_flag);
int con_file_open_sorse(files* fs, preprocess_context *context);
int con_file_open_hedrs(files* fs, preprocess_context *context);
int con_file_open_next(files* fs, preprocess_context *context, int h_flag);
void con_file_close_cur(preprocess_context *context);

void con_file_it_is_end_h(files *fs, int i);

void con_file_print_coment(files *fs, preprocess_context *context);

#ifdef __cplusplus
} /* extern "C" */
#endif
