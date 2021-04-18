/*
 *	Copyright 2018 Andrey Terekhov, Egor Anikin
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

#include "utils.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utf8.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int equal_reprtab(int i, int j, environment *const env)
{
	i += 2;
	j += 2;

	while (env->reprtab[i] == env->reprtab[j])
	{
		i++;
		j++;

		if (env->reprtab[i] == 0 && env->reprtab[j] == 0)
		{
			return 1;
		}
	}

	return 0;
}

void output_keywords(environment *const env)
{
	for (size_t j = 0; j < (size_t)env->reprtab[env->rp]; j++)
	{
		m_fprintf(env, env->reprtab[env->rp + 2 + j]);
	}
}

int macro_keywords(environment *const env)
{
	int oldrepr = env->rp;
	int r = 0;
	int n = 0;

	env->rp += 2;
	int hash = 0;
	do
	{
		hash += (int)env_get_curchar(env);
		env->reprtab[env->rp++] = (int)env_get_curchar(env);
		n++;
		env_scan_next_char(env);
	} while (utf8_is_letter(env_get_curchar(env)) || utf8_is_digit(env_get_curchar(env)));

	/*if (env_get_curchar(env) != '\n' && env_get_curchar(env) != ' ' && env_get_curchar(env) != '\t' && env_get_curchar(env) != '(' &&
		env_get_curchar(env) != '\"')
	{
		
		env_error(env, after_ident_must_be_space);
	}*/

	hash &= 255;
	env->reprtab[env->rp++] = 0;
	r = env->hashtab[hash];
	if (r)
	{
		do
		{
			if (equal_reprtab(r, oldrepr, env))
			{
				env->rp = oldrepr;
				env->reprtab[env->rp] = n;
				return (env->reprtab[r + 1] < 0) ? env->reprtab[r + 1] : 0;
			}
			else
			{
				r = env->reprtab[r];
			}
		} while (r);
	}

	env->rp = oldrepr;
	env->reprtab[env->rp] = n;
	return 0;
}

int mf_equal(environment *const env, int i, char32_t *buffer)
{
	size_t j = 0;
	i += 2;

	while (env->reprtab[i] == (int)buffer[j])
	{
		i++;
		j++;
		if (env->reprtab[i] == 0 && (int)buffer[j] == '\0')
		{
			return 1;
		}
	}

	return 0;
}

int collect_mident(environment *const env, char32_t *buffer)
{
	int r;
	int hash = 0;
	size_t buffer_size = 0;

	while (utf8_is_letter(env_get_curchar(env)) || utf8_is_digit(env_get_curchar(env)))
	{
		buffer[buffer_size++] = env_get_curchar(env);
		hash += env_get_curchar(env);
		env_scan_next_char(env);
	}

	buffer[buffer_size] = '\0';
	hash &= 255;
	r = env->hashtab[hash];

	while (r)
	{
		if (mf_equal(env, r, buffer))
		{
			return (env_io_get_char(env, macro_text_type, (size_t)env->reprtab[r + 1]) != MACRO_UNDEF 
				&& env->reprtab[r + 1] != SH_MAIN) ? r : 0;
		}

		r = env->reprtab[r];
	}

	return 0;
}

int skip_line(environment *const env)
{
	while (env_get_curchar(env) != '\n')
	{
		if (env_get_curchar(env) == ' ' || env_get_curchar(env) == '\t' || env_get_curchar(env) == '\r')
		{
			env_scan_next_char(env);
		}
		else
		{
			env_error(env, after_preproces_words_must_be_space);
			return -1;
		}
	}

	return 0;
}

void skip_separators(environment *const env)
{
	while (env_get_curchar(env) == ' ' || env_get_curchar(env) == '\t')
	{
		env_scan_next_char(env);
	}
}

int skip_string(environment *const env)
{
	char32_t c = env_get_curchar(env);
	m_fprintf(env, env_get_curchar(env));
	env_scan_next_char(env);

	while (env_get_curchar(env) != c && env_get_curchar(env) != (char32_t)EOF && env_get_curchar(env) != '\n')
	{
		if (env_get_curchar(env) == '\\')
		{
			m_fprintf(env, env_get_curchar(env));
			env_scan_next_char(env);
		}

		m_fprintf(env, env_get_curchar(env));
		env_scan_next_char(env);
	}

	if (env_get_curchar(env) == (char32_t)EOF || env_get_curchar(env) == '\n')
	{
		env_error(env, no_string_ending);
		return -1;
	}

	m_fprintf(env, env_get_curchar(env));
	env_scan_next_char(env);
	return 0;
}

void end_of_file(environment *const env)
{
	while (env_io_get_type(env) != file_type)
	{
		env_io_back_to_previous(env);
	}
}
