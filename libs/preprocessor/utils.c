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
#include "file.h"
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

void output_keywods(environment *const env)
{
	for (int j = 0; j < env->reprtab[env->rp]; j++)
	{
		m_fprintf(env->reprtab[env->rp + 2 + j], env);
	}
}

int is_letter(environment *const env)
{
	return (env->curchar >= 'A' && env->curchar <= 'Z') ||
		   (env->curchar >= 'a' && env->curchar <= 'z') || env->curchar == '_' ||
		   utf8_is_russian(env->curchar);
}

int is_digit(int a)
{
	return a >= '0' && a <= '9';
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
		hash += env->curchar;
		env->reprtab[env->rp++] = env->curchar;
		n++;
		m_nextch(env);
	} while (is_letter(env) || is_digit(env->curchar));

	/*if (env->curchar != '\n' && env->curchar != ' ' && env->curchar != '\t' && env->curchar != '(' &&
		env->curchar != '\"')
	{
		size_t position = skip_str(env); 
		macro_error(after_ident_must_be_space
			, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
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

int mf_equal(int i, environment *const env)
{
	int j = 0;
	i += 2;

	while (env->reprtab[i] == env->mstring[j])
	{
		i++;
		j++;
		if (env->reprtab[i] == 0 && env->mstring[j] == MACROEND)
		{
			return 1;
		}
	}

	return 0;
}

int collect_mident(environment *const env)
{
	int r;
	int hash = 0;
	env->msp = 0;

	while (is_letter(env) || is_digit(env->curchar))
	{
		env->mstring[env->msp++] = env->curchar;
		hash += env->curchar;
		m_nextch(env);
	}

	env->mstring[env->msp] = MACROEND;
	hash &= 255;
	r = env->hashtab[hash];

	while (r)
	{
		if (r >= env->mfirstrp && mf_equal(r, env))
		{
			return (env->macrotext[env->reprtab[r + 1]] != MACROUNDEF) ? r : 0;
		}

		r = env->reprtab[r];
	}

	return 0;
}

int find_file(environment *const env, const char *s)
{
	int oldrp = env->rp;
	env->rp += 2;
	int r;
	int hash = 0;
	int i = 0;

	while (s[i] != '\0')
	{
		env->reprtab[env->rp++] = s[i];
		hash += s[i];
		i++;
	}

	hash &= 255;
	r = env->hashtab[hash];

	while (r)
	{
		if (env->reprtab[r + 1] == SH_FILE && equal_reprtab(r, oldrp, env))
		{
			env->rp = oldrp;
			return 0;
		}

		r = env->reprtab[r];
	}

	env->reprtab[oldrp] = env->hashtab[hash];
	env->reprtab[oldrp + 1] = SH_FILE;
	env->hashtab[hash] = oldrp;
	env->reprtab[env->rp++] = 0;
	return 1;
}

int space_end_line(environment *const env)
{
	while (env->curchar != '\n')
	{
		if (env->curchar == ' ' || env->curchar == '\t')
		{
			m_nextch(env);
		}
		else
		{
			size_t position = skip_str(env); 
			macro_error(after_preproces_words_must_be_space
			, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
			return -1;
		}
	}

	return 0;
}

void skip_space(environment *const env)
{
	while (env->curchar == ' ' || env->curchar == '\t')
	{
		m_nextch(env);
	}
}

void skip_space_str(environment *const env)
{
	int c = env->curchar;
	m_fprintf(env->curchar, env);
	m_nextch(env);

	while (env->curchar != c && env->curchar != EOF)
	{
		if (env->curchar == '\\')
		{
			m_fprintf(env->curchar, env);
			m_nextch(env);
		}

		m_fprintf(env->curchar, env);
		m_nextch(env);
	}

	if (env->curchar != EOF)
	{
		m_fprintf(env->curchar, env);
		m_nextch(env);
	}
}

size_t skip_str(environment *const env)
{
	char *line = env->error_string;
	size_t position = strlen(line);
	while (env->curchar != '\n' && env->curchar != EOF)
	{
		m_nextch(env);
	}
	return position;
}

void skip_file(environment *const env)
{
	while (env->curchar != EOF)
	{
		m_nextch(env);
	}
}