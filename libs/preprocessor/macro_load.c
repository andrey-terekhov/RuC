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
#include <string.h>
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utils.h"


int function_scope_collect(environment *const env, const size_t num, const size_t was_bracket)
{
	while (env_get_curchar(env) != (char32_t)EOF)
	{
		if (utf8_is_letter(env_get_curchar(env)))
		{	
			char32_t buffer[STRING_SIZE];
			const int macro_ptr = collect_mident(env, buffer);
			if (macro_ptr)
			{
				const size_t old_param_size = env_io_get_size(env, param_type);
				const size_t old_define_stack_size = env_io_get_size(env, define_stack_type);

				env_move_macro_stack_prt(env, num);
				if (macro_get(env, macro_ptr))
				{
					return -1;
				}

				char32_t loc_change[STRING_SIZE];
				size_t loc_change_size = 0;
				const size_t loc_depth = env_io_get_type(env) == param_type
				? env_io_get_depth(env) - 1
				: env_io_get_depth(env);

				while (env_io_get_depth(env) >= loc_depth) // 1 переход потому что есть префиксная замена
				{
					loc_change[loc_change_size++] = env_get_curchar(env);
					env_scan_next_char(env);
				}

				env_move_macro_stack_prt(env, (int)(-num));
				env_io_clear(env, param_type, old_param_size);
				env_io_clear(env, define_stack_type, old_define_stack_size);

				//env_clear_param(env, old_param_size);

				for (size_t i = 0; i < loc_change_size; i++)
				{
					env_io_add_char(env, param_type, loc_change[i]);
				}
			}
			else
			{
				size_t i = 0;
				while(buffer[i] != '\0')
				{
					env_io_add_char(env, param_type, buffer[i++]);
				}
			}
		}
		else if (env_get_curchar(env) == '(')
		{
			env_io_add_char(env, param_type, env_get_curchar(env));
			env_scan_next_char(env);

			if (function_scope_collect(env, num, 0))
			{
				return -1;
			}
		}
		else if (env_get_curchar(env) == ')' || (was_bracket == 1 && env_get_curchar(env) == ','))
		{
			if (was_bracket == 0)
			{
				env_io_add_char(env, param_type, env_get_curchar(env));
				env_scan_next_char(env);
			}
			return 0;
		}
		else if (env_get_curchar(env) == '#')
		{
			if (macro_keywords(env) == SH_EVAL && env_get_curchar(env) == '(')
			{
				char buffer[STRING_SIZE];
				if (calculate_arithmetic(env, buffer))
				{
					return -1;
				}

				size_t lenght = strlen(buffer);
				for (size_t i = 0; i < lenght; i++)
				{
					env_io_add_char(env, param_type, (char32_t)buffer[i]);
				}
			}
			else
			{
				for (size_t i = 0; i < (size_t)env->reprtab[env->rp]; i++)
				{
					env_io_add_char(env, param_type, (char32_t)env->reprtab[env->rp + 2 + i]);
				}
			}
		}
		else
		{
			env_io_add_char(env, param_type, env_get_curchar(env));
			env_scan_next_char(env);
		}
	}
	
	env_error(env, scope_not_close);
	return -1;
}

int function_stack_create(environment *const env, const size_t parameters)
{
	env_io_clear(env, define_stack_type, env_get_macro_stack_prt(env));
	env_io_clear(env, param_type, env_io_get_char(env, define_stack_type, env_get_macro_stack_prt(env)));
	env_scan_next_char(env);

	if (env_get_curchar(env) == ')')
	{
		env_error(env, stalpe);
		return -1;
	}

	size_t num = 0;
	env_io_add_char(env, define_stack_type, (char32_t)env_io_get_size(env, param_type));
	while (env_get_curchar(env) != ')')
	{
		if (function_scope_collect(env, num, 1))
		{
			return -1;
		}
		env_io_add_char(env, param_type, '\0');

		if (env_get_curchar(env) == ',')
		{
			num++;
			env_io_add_char(env, define_stack_type, (char32_t)env_io_get_size(env, param_type));
			if (num > parameters)
			{
				env_error(env, not_enough_param);
				return -1;
			}
			env_scan_next_char(env);

			if (env_get_curchar(env) == ' ')
			{
				env_scan_next_char(env);
			}
		}
		else if (env_get_curchar(env) == ')')
		{
			if (num != parameters)
			{
				env_error(env, not_enough_param2);
				return -1;
			}
			env_scan_next_char(env);
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

	size_t loc_macro_ptr = (size_t)env->reprtab[index + 1];
	if (env_io_get_char(env, macro_text_type, loc_macro_ptr++) == MACRO_FUNCTION)
	{
		if ((int)env_io_get_char(env, macro_text_type, loc_macro_ptr) > -1 
			&& function_stack_create(env, (size_t)env_io_get_char(env, macro_text_type, loc_macro_ptr)))
		{
			return -1;
		}
		loc_macro_ptr++;
	}
	env_io_switch_to_new(env, macro_text_type, loc_macro_ptr);

	return 0;
}
