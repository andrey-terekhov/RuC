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


int function_scope_collect(environment *const env, const size_t num, const size_t was_bracket)
{
	while (env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar))
		{
			const int macro_ptr = collect_mident(env);
			if (macro_ptr)
			{
				const size_t old_change_size = env->args_size;
				const size_t old_local_stack_size = env->local_stack_size;

				env->local_stack_size += num;
				if (macro_get(env, macro_ptr))
				{
					return -1;
				}

				int loc_change[STRING_SIZE];
				size_t loc_change_size = 0;
				const size_t loc_depth = env->nextch_type == PARAM_TYPE
				? get_depth(env) - 1
				: get_depth(env);

				while (get_depth(env) >= loc_depth) // 1 переход потому что есть префиксная замена
				{
					loc_change[loc_change_size++] = env->curchar;
					m_nextch(env);
				}

				env->local_stack_size = old_local_stack_size;
				env->args_size = old_change_size;

				for (size_t i = 0; i < loc_change_size; i++)
				{
					env->args[env->args_size++] = loc_change[i];
				}
			}
			else
			{
				for (size_t i = 0; i < env->msp; i++)
				{
					env->args[env->args_size++] = env->mstring[i];
				}
			}
		}
		else if (env->curchar == '(')
		{
			env->args[env->args_size++] = env->curchar;
			m_nextch(env);

			if (function_scope_collect(env, num, 0))
			{
				return -1;
			}
		}
		else if (env->curchar == ')' || (was_bracket == 1 && env->curchar == ','))
		{
			if (was_bracket == 0)
			{
				env->args[env->args_size++] = env->curchar;
				m_nextch(env);
			}

			return 0;
		}
		else if (env->curchar == '#')
		{
			if (macro_keywords(env) == SH_EVAL && env->curchar == '(')
			{
				
				int calculate_res;
				if (calculate_arithmetic(env, &calculate_res))
				{
					return -1;
				}

				char buffer[STRING_SIZE];
				sprintf(buffer, "%d", calculate_res);
				size_t i = 0;
				while (buffer[i] != '\0')
				{
					env->args[env->args_size++] = buffer[i];
					i++;
				}
			}
			else
			{
				for (size_t i = 0; i < (size_t)env->reprtab[env->rp]; i++)
				{
					env->args[env->args_size++] = env->reprtab[env->rp + 2 + i];
				}
			}
		}
		else
		{
			env->args[env->args_size++] = env->curchar;
			m_nextch(env);
		}
	}
	
	env_error(env, scope_not_close);
	return -1;
}

int function_stack_create(environment *const env, const size_t parameters)
{
	m_nextch(env);

	if (env->curchar == ')')
	{
		env_error(env, stalpe);
		return -1;
	}

	size_t num = 0;
	env->localstack[num + env->local_stack_size] = env->args_size;

	while (env->curchar != ')')
	{
		if (function_scope_collect(env, num, 1))
		{
			return -1;
		}
		env->args[env->args_size++] = END_PARAMETER;

		if (env->curchar == ',')
		{
			num++;
			env->localstack[num + env->local_stack_size] = env->args_size;

			if (num > parameters)
			{
				env_error(env, not_enough_param);
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
			if (num != parameters)
			{
				env_error(env, not_enough_param2);
				return -1;
			}
			m_nextch(env);

			env->args_size = env->localstack[env->local_stack_size];
			return 0;
		}
	}
	
	env_error(env, scope_not_close);
	return -1;
}

int macro_get(environment *const env, const size_t index)
{
	if (index == 0)
	{
		env_error(env, ident_not_exist);
		return -1;
	}

	env->msp = 0;

	size_t loc_macro_ptr = env->reprtab[index + 1];
	if (env->macro_tab[loc_macro_ptr++] == MACRO_FUNCTION)
	{
		if (env->macro_tab[loc_macro_ptr] > -1 && function_stack_create(env, env->macro_tab[loc_macro_ptr]))
		{
			return -1;
		}
		
		loc_macro_ptr++;
	}

	m_change_nextch_type(env, MACRO_TEXT_TYPE, loc_macro_ptr);
	m_nextch(env);

	return 0;
}
