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

#include "get_macro.h"
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utils.h"


int function_scob_collect(const int t, const int num, environment *const env)
{
	while (env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar))
		{
			int r = collect_mident(env);

			if (r)
			{
				const int oldcp1 = env->cp;
				const int oldlsp = env->lsp;
				int locfchange[STRING_SIZE];
				int lcp = 0;
				int ldip;

				env->lsp += num;
				if(get_macro(r, env))
				{
					return -1;
				}
				ldip = get_dipp(env);

				if (env->nextch_type == FTYPE)
				{
					ldip--;
				}

				while (get_dipp(env) >= ldip) // 1 переход потому что есть префиксная замена
				{
					locfchange[lcp++] = env->curchar;
					m_nextch(env);
				}

				env->lsp = oldlsp;
				env->cp = oldcp1;

				for (int i = 0; i < lcp; i++)
				{
					env->fchange[env->cp++] = locfchange[i];
				}
			}
			else
			{
				for (int i = 0; i < env->msp; i++)
				{
					env->fchange[env->cp++] = env->mstring[i];
				}
			}
		}
		else if (env->curchar == '(')
		{
			env->fchange[env->cp++] = env->curchar;
			m_nextch(env);
			
			if(function_scob_collect(0, num, env))
			{
				return -1;
			}
		}
		else if (env->curchar == ')' || (t == 1 && env->curchar == ','))
		{
			if (t == 0)
			{
				env->fchange[env->cp++] = env->curchar;
				m_nextch(env);
			}

			return 0;
		}
		else if (env->curchar == '#')
		{
			if (macro_keywords(env) == SH_EVAL && env->curchar == '(')
			{	
				if(calculate(0, env))
				{
					return -1;
				}
				for (int i = 0; i < env->csp; i++)
				{
					env->fchange[env->cp++] = env->cstring[i];
				}
			}
			else
			{
				for (int i = 0; i < env->reprtab[env->rp]; i++)
				{
					env->fchange[env->cp++] = env->reprtab[env->rp + 2 + i];
				}
			}
		}
		else
		{
			env->fchange[env->cp++] = env->curchar;
			m_nextch(env);
		}
	}
	size_t position = env_skip_str(env); 
	macro_error(scob_not_clous, env_get_current_file(env), env->error_string, env->line, position);
	return -1;
}

int function_stack_create(const int n, environment *const env)
{
	int num = 0;

	m_nextch(env);
	env->localstack[num + env->lsp] = env->cp;

	if (env->curchar == ')')
	{
		size_t position = env_skip_str(env); 
		macro_error(stalpe, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	while (env->curchar != ')')
	{
		if(function_scob_collect(1, num, env))
		{
			return -1;
		}
		env->fchange[env->cp++] = CANGEEND;

		if (env->curchar == ',')
		{
			num++;
			env->localstack[num + env->lsp] = env->cp;

			if (num > n)
			{
				size_t position = env_skip_str(env); 
				macro_error(not_enough_param, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
			m_nextch(env);

			if (env->curchar == ' ')
			{
				m_nextch(env);
			}
		}
		else if (env->curchar == ')')
		{
			if (num != n)
			{
				size_t position = env_skip_str(env); 
				macro_error(not_enough_param2, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
			m_nextch(env);

			env->cp = env->localstack[env->lsp];
			return 0;
		}
	}

	size_t position = env_skip_str(env); 
	macro_error(scob_not_clous, env_get_current_file(env), env->error_string, env->line, position);
	return -1;
}

int get_macro(const int index, environment *const env)
{
	int t = env->reprtab[index + 1];

	if (index)
	{
		env->msp = 0;
		if (env->macrotext[t] == MACROFUNCTION && env->macrotext[++t] > -1 
			&& function_stack_create(env->macrotext[t], env))
		{
			return -1;
		}

		m_change_nextch_type(TEXTTYPE, t + 1, env);
		m_nextch(env);
	}
	else
	{
		size_t position = env_skip_str(env); 
		macro_error(ident_not_exist, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	return 0;
}

