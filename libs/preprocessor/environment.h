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
#include "linker.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct environment
{
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

	int nested_if;
	int flagint;

	const char **curent_path;

	size_t line;

	linker *lk;

	universal_io *output;
	universal_io *input;
} environment;

void env_init(environment *const env, linker *const lk, universal_io *const output);
void env_clear_error_string(environment *const env);
const char *env_get_current_file(environment *const env);

/**
 *	Add a comment to indicate line changes in the output
 *
 *	@param	env	Preprocessor environment
 */
void env_add_comment(environment *const env);

size_t env_skip_str(environment *const env);

void m_change_nextch_type(environment *const env, int type, int p);
void m_old_nextch_type(environment *const env);

int get_dipp(environment *const env);
int get_next_char(environment *const env);

void m_fprintf(environment *const env, int a);
void m_nextch(environment *const env);

#ifdef __cplusplus
} /* extern "C" */
#endif
