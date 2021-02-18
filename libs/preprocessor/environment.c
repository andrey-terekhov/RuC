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

#include "environment.h"
#include "constants.h"
#include "commenter.h"
#include "error.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <string.h>

#define MAX_CMT_SIZE MAX_ARG_SIZE + 32


void env_init(environment *const env, linker *const lk, universal_io *const output)
{
	env->output = output;

	env->lk = lk;

	env->rp = 1;
	env->mp = 1;
	env->cp = 0;
	env->lsp = 0;
	env->csp = 0;
	env->ifsp = 0;
	env->wsp = 0;
	env->mfirstrp = -1;
	env->prep_flag = 0;
	env->nextch_type = FILETYPE;
	env->curchar = 0;
	env->nextchar = 0;
	env->cur = 0;
	env->nextp = 0;
	env->dipp = 0;
	env->line = 1;
	env->position = 0;
	env->nested_if = 0;
	env->flagint = 1;

	for (int i = 0; i < HASH; i++)
	{
		env->hashtab[i] = 0;
	}

	for (int i = 0; i < MAXTAB; i++)
	{
		env->reprtab[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE; i++)
	{
		env->mstring[i] = 0;
		env->error_string[i] = 0;
		env->localstack[i] = 0;
		env->cstring[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE * 3; i++)
	{
		env->fchange[i] = 0;
	}
	
	for (int i = 0; i < STRING_SIZE * 2; i++)
	{
		env->ifstring[i] = 0;
	}

	for (int i = 0; i < STRING_SIZE * 5; i++)
	{
		env->wstring[i] = 0;
	}

	for (int i = 0; i < DIP; i++)
	{
		env->oldcurchar[i] = 0;
		env->oldnextchar[i] = 0;
		env->oldnextch_type[i] = 0;
		env->oldnextp[i] = 0;
	}
}

void env_clear_error_string(environment *const env)
{
	env->position = 0;
}

const char *env_get_current_file(environment *const env)
{
	return ws_get_file(env->lk->ws, env->lk->current);
}

void env_add_comment(environment *const env)
{
	comment cmt = cmt_create(env_get_current_file(env), env->line);

	char buffer[MAX_CMT_SIZE];
	cmt_to_string(&cmt, buffer);

	uni_printf(env->output, "%s", buffer);
}

size_t env_skip_str(environment *const env)
{
	char *line = env->error_string;
	size_t position = strlen(line);
	while (env->curchar != '\n' && env->curchar != EOF)
	{
		m_nextch(env);
	}
	return position;
}

void m_nextch(environment *const env);

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

void m_change_nextch_type(environment *const env, int type, int p)
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

void m_fprintf(environment *const env, int a)
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
				const size_t position = env_skip_str(env);  
				macro_error(comm_not_ended, env_get_current_file(env), env->error_string, env->line, position);
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
	m_change_nextch_type(env, FTYPE, env->localstack[env->curchar + env->lsp]);
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

			if (env->curchar == '\n')
			{
				env_add_comment(env);
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
