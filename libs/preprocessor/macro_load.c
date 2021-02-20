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

#include "macro_load.h"
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utils.h"


int function_scope_collect(environment *const env, const int nesting_flag, const int num)
{
	while (env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar))
		{
			const int macros_prt = collect_mident(env);
			if (macros_prt)
			{
				const int old_chg_prt = env->chg_prt;
				const int old_loc_stk_prt = env->loc_stk_prt;
				int loc_change[STRING_SIZE];
				int loc_chg_prt = 0;
				int loc_depth;

				env->loc_stk_prt += num;
				if (macros_get(env, macros_prt))
				{
					return -1;
				}
				loc_depth = get_depth(env);

				if (env->nextch_type == FTYPE)
				{
					loc_depth--;
				}

				while (get_depth(env) >= loc_depth) // 1 переход потому что есть префиксная замена
				{
					loc_change[loc_chg_prt++] = env->curchar;
					m_nextch(env);
				}

				env->loc_stk_prt = old_loc_stk_prt;
				env->chg_prt = old_chg_prt;

				for (int i = 0; i < loc_chg_prt; i++)
				{
					env->fchange[env->chg_prt++] = loc_change[i];
				}
			}
			else
			{
				for (int i = 0; i < env->msp; i++)
				{
					env->fchange[env->chg_prt++] = env->mstring[i];
				}
			}
		}
		else if (env->curchar == '(')
		{
			env->fchange[env->chg_prt++] = env->curchar;
			m_nextch(env);

			if (function_scope_collect(env, 0, num))
			{
				return -1;
			}
		}
		else if (env->curchar == ')' || (nesting_flag == 1 && env->curchar == ','))
		{
			if (nesting_flag == 0)
			{
				env->fchange[env->chg_prt++] = env->curchar;
				m_nextch(env);
			}

			return 0;
		}
		else if (env->curchar == '#')
		{
			if (macro_keywords(env) == SH_EVAL && env->curchar == '(')
			{
				if (calculate(env, 0))
				{
					return -1;
				}
				for (int i = 0; i < env->calc_prt; i++)
				{
					env->fchange[env->chg_prt++] = env->cstring[i];
				}
			}
			else
			{
				for (int i = 0; i < env->reprtab[env->rp]; i++)
				{
					env->fchange[env->chg_prt++] = env->reprtab[env->rp + 2 + i];
				}
			}
		}
		else
		{
			env->fchange[env->chg_prt++] = env->curchar;
			m_nextch(env);
		}
	}
	const size_t position = env_skip_str(env);
	macro_error(scope_not_clous, env_get_current_file(env), env->error_string, env->line, position);
	return -1;
}

int function_stack_create(environment *const env, const int n)
{
	int num = 0;

	m_nextch(env);
	env->localstack[num + env->loc_stk_prt] = env->chg_prt;

	if (env->curchar == ')')
	{
		const size_t position = env_skip_str(env);
		macro_error(stalpe, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	while (env->curchar != ')')
	{
		if (function_scope_collect(env, 1, num))
		{
			return -1;
		}
		env->fchange[env->chg_prt++] = CANGEEND;

		if (env->curchar == ',')
		{
			num++;
			env->localstack[num + env->loc_stk_prt] = env->chg_prt;

			if (num > n)
			{
				const size_t position = env_skip_str(env);
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
				const size_t position = env_skip_str(env);
				macro_error(not_enough_param2, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
			m_nextch(env);

			env->chg_prt = env->localstack[env->loc_stk_prt];
			return 0;
		}
	}

	const size_t position = env_skip_str(env);
	macro_error(scope_not_clous, env_get_current_file(env), env->error_string, env->line, position);
	return -1;
}

int macros_get(environment *const env, const int index)
{
	if (index == 0)
	{
		const size_t position = env_skip_str(env);
		macro_error(ident_not_exist, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	int loc_macro_prt = env->reprtab[index + 1];

	env->msp = 0;
	if (env->macrotext[loc_macro_prt] == MACROFUNCTION)
	{
		loc_macro_prt++;
		if (env->macrotext[loc_macro_prt] > -1 && function_stack_create(env, env->macrotext[loc_macro_prt]))
		{
			return -1;
		}
	}

	m_change_nextch_type(env, TEXTTYPE, loc_macro_prt + 1);
	m_nextch(env);

	return 0;
}

