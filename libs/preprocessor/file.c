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

#include "file.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utils.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void m_nextch(environment *const env);


size_t strlen32(const char32_t *const str)
{
	if (str == NULL)
	{
		return SIZE_MAX;
	}

	size_t i = 0;
	while (str[i] != '\0')
	{
		i++;
	}
	return i;
}

int get_next_char(environment *const env)
{
	env->nextchar = uni_scan_char(env->input);
	if (env->nextchar == U'\r')
	{
		return get_next_char(env);
	}
	return env->nextchar;
}

int get_dipp(environment *const env)
{
	return env->dipp;
}

void m_change_nextch_type(int type, int p, environment *const env)
{
	env->oldcurchar[env->dipp] = env->curchar;
	env->oldnextchar[env->dipp] = env->nextchar;
	env->oldnextch_type[env->dipp] = env->nextch_type;
	env->oldnextp[env->dipp] = env->nextp;
	env->nextp = p;
	env->dipp++;
	env->nextch_type = type;
}

void m_old_nextch_type(environment *const env)
{
	env->dipp--;
	env->curchar = env->oldcurchar[env->dipp];
	env->nextchar = env->oldnextchar[env->dipp];
	env->nextch_type = env->oldnextch_type[env->dipp];
	env->nextp = env->oldnextp[env->dipp];
}

void end_line(environment *const env)
{
	env->line++;
}

void m_onemore(environment *const env)
{
	env->curchar = env->nextchar;

	get_next_char(env);

	if (env->curchar == EOF)
	{
		env->nextchar = EOF;
		end_line(env);
	}
}


void m_fprintf(int a, environment *const env)
{
	uni_print_char(env->output, a);
}

void m_coment_skip(environment *const env)
{
	if (env->curchar == '/' && env->nextchar == '/')
	{
		do
		{
			// m_fprintf_com();
			m_onemore(env);

			if (env->curchar == EOF)
			{
				return;
			}
		} while (env->curchar != '\n');
	}

	if (env->curchar == '/' && env->nextchar == '*')
	{
		// m_fprintf_com();
		m_onemore(env);
		// m_fprintf_com();

		do
		{
			m_onemore(env);
			// m_fprintf_com();
			if (env->curchar == '\n')
			{
				end_line(env);
			}

			if (env->curchar == EOF)
			{
				size_t position = skip_str(env); 
				macro_error(comm_not_ended
			, lk_get_current(env->lk)
			, env->error_string, env->line, position);
				end_line(env);
				return;
			}
		} while (env->curchar != '*' || env->nextchar != '/');

		m_onemore(env);
		// m_fprintf_com();
		env->curchar = ' ';
	}
}

void m_nextch_cange(environment *const env)
{
	m_nextch(env);
	m_change_nextch_type(FTYPE, env->localstack[env->curchar + env->lsp], env);
	m_nextch(env);
}

void m_nextch(environment *const env)
{
	if (env->nextch_type != FILETYPE && env->nextch_type <= TEXTTYPE)
	{
		if (env->nextch_type == MTYPE && env->nextp < env->msp)
		{
			env->curchar = env->mstring[env->nextp++];
			env->nextchar = env->mstring[env->nextp];
		}
		else if (env->nextch_type == CTYPE && env->nextp < env->csp)
		{
			env->curchar = env->cstring[env->nextp++];
			env->nextchar = env->cstring[env->nextp];
		}
		else if (env->nextch_type == IFTYPE && env->nextp < env->ifsp)
		{
			env->curchar = env->ifstring[env->nextp++];
			env->nextchar = env->ifstring[env->nextp];
		}
		else if (env->nextch_type == WHILETYPE && env->nextp < env->wsp)
		{
			env->curchar = env->wstring[env->nextp++];
			env->nextchar = env->wstring[env->nextp];
		}
		else if (env->nextch_type == TEXTTYPE && env->nextp < env->mp)
		{
			env->curchar = env->macrotext[env->nextp++];
			env->nextchar = env->macrotext[env->nextp];

			if(env->curchar == '\n')
			{
				lk_add_comment(env);
			}
			else if (env->curchar == MACROCANGE)
			{
				m_nextch_cange(env);
			}
			else if (env->curchar == MACROEND)
			{
				m_old_nextch_type(env);
			}
		}
		else if (env->nextch_type == FTYPE)
		{
			env->curchar = env->fchange[env->nextp++];
			env->nextchar = env->fchange[env->nextp];

			if (env->curchar == CANGEEND)
			{
				m_old_nextch_type(env);
				m_nextch(env);
			}
		}
		else
		{
			m_old_nextch_type(env);
		}
	}
	else
	{
		m_onemore(env);

		m_coment_skip(env);

		if (env->curchar == '\n')
		{
			end_line(env);
		}
	}

	if (env->curchar != '\n' && env->curchar != EOF)
	{
		env->error_string[env->position++] = (char)env->curchar;
		env->error_string[env->position] = '\0';
	}
	else
	{
		env_clear_error_string(env);
	}
	// printf("t = %d curchar = %c, %i nextchar = %c, %i \n", env->nextch_type,
	// env->curchar, env->curchar, env->nextchar, env->nextchar);
}
