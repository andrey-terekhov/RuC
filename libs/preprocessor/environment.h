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
#include "linker.h"
#include "uniio.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct environment
{
    int hashtab[256];
    int reprtab[MAXTAB];
    int rp;

    int macro_tab[MAXTAB];
    size_t macro_tab_size;

    char error_string[STRING_SIZE];
    size_t position;

    int mstring[STRING_SIZE];
    size_t msp;

    int change[STRING_SIZE * 3];
    size_t change_size;

    int localstack[STRING_SIZE];
    size_t local_stack_size;

    int calc_string[STRING_SIZE];
    size_t calc_string_size;

    int if_string[STRING_SIZE * 2];
    size_t if_string_size;

    int while_string[STRING_SIZE * 5];
    size_t while_string_size;

    int mfirstrp;

    int prep_flag;

    int curchar, nextchar;
    int nextch_type;
    int cur;

    size_t nextp;

    int oldcurchar[DEPTH];
    int oldnextchar[DEPTH];
    int oldnextch_type[DEPTH];
    int oldnextp[DEPTH];
    size_t depth;

    int nested_if;
    int flagint;

    size_t line;

    linker *lk;

    universal_io *output;
    universal_io *input;

    int disable_recovery;
    int was_error;
} environment;

void env_init(environment *const env, linker *const lk, universal_io *const output);
void env_clear_error_string(environment *const env);

/**
 *	Add a comment to indicate line changes in the output
 *
 *	@param	env	Preprocessor environment
 */
void env_add_comment(environment *const env);

void m_change_nextch_type(environment *const env, int type, int p);
void m_old_nextch_type(environment *const env);

int get_depth(environment *const env);
int get_next_char(environment *const env);

void m_fprintf(environment *const env, int a);
void m_nextch(environment *const env);

void env_error(environment *const env, const int num);

#ifdef __cplusplus
} /* extern "C" */
#endif
