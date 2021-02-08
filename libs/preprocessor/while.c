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

#include "while.h"
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "file.h"
#include "preprocessor.h"
#include "error.h"
#include "linker.h"
#include "utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int while_collect(environment *const env)
{
	int oldwsp = env->wsp;

	env->wstring[env->wsp++] = WHILEBEGIN;
	env->wstring[env->wsp++] = env->ifsp;
	env->wsp++;

	while (env->curchar != '\n')
	{
		env->ifstring[env->ifsp++] = env->curchar;
		m_nextch(env);
	}
	env->ifstring[env->ifsp++] = '\n';
	m_nextch(env);

	while (env->curchar != EOF)
	{
		if (env->curchar == '#')
		{
			env->cur = macro_keywords(env);

			if (env->cur == SH_WHILE)
			{
				while_collect(env);
			}
			else if (env->cur == SH_ENDW)
			{	
				env->wstring[env->wsp++] = ' ';
				env->wstring[oldwsp + 2] = env->wsp;
				env->cur = 0;

				return 0;
			}
			else
			{
				int i = 0;

				for (i = 0; i < env->reprtab[env->rp]; i++)
				{
					env->wstring[env->wsp++] = env->reprtab[env->rp + 2 + i];
				}
			}
		}
		env->wstring[env->wsp++] = env->curchar;
		m_nextch(env);
	}

	size_t position = skip_str(env); 
	macro_error(must_end_endw
			, lk_get_current(env->lk)
			, env->error_string, env->line, position);
	return -1;
}

int while_realiz(environment *const env)
{
	int oldernextp = env->nextp;
	int end = env->wstring[oldernextp + 2];
	int error = 0;

	env->cur = 0;
	while (env->wstring[oldernextp] == WHILEBEGIN)
	{
		m_nextch(env);
		m_change_nextch_type(IFTYPE, env->wstring[env->nextp], env);
		m_nextch(env);
		if(calculator(1, env))
		{
			return -1;
		}
		m_old_nextch_type(env);


		if (env->cstring[0] == 0)
		{
			env->nextp = end;
			m_nextch(env);
			return 0;
		}

		m_nextch(env);
		m_nextch(env);
		m_nextch(env);
		skip_space(env);

		while (env->nextp != end || env->nextch_type != WHILETYPE)
		{
			if (env->curchar == WHILEBEGIN)
			{
				env->nextp--;
				if(while_realiz(env))
				{
					return -1;
				}
			}
			else if (env->curchar == EOF)
			{
				size_t position = skip_str(env); 
				macro_error(must_end_endw
			, lk_get_current(env->lk)
			, env->error_string, env->line, position);
				return -1;
			}
			else
			{
				error = preprocess_scan(env);
				if(error)
				{
					return error;
				}
			}
		}
		env->nextp = oldernextp;
	}
	return 0;
}
