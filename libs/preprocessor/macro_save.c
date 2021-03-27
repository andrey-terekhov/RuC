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

#include "macro_save.h"
#include <string.h>
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "macro_load.h"
#include "utils.h"


size_t m_equal(char32_t *buffer, int *temp_str)
{
	size_t n = 1;
	size_t i = 0;
	while (temp_str[i] != -1)
	{
		size_t j = 0;
		while (temp_str[i + j] == buffer[j])
		{
			j++;
			if (temp_str[i + j] == 0 && buffer[j] == MACRO_END)
			{
				return n;
			}
		}

		i += j;
		while (temp_str[i] != 0)
		{
			i++;
		}
		i++;
		n++;
	}
	return 0;
}

int func_check_macro(environment *const env, int flag_macro_directive, int *temp_str)
{
	char32_t buffer[STRING_SIZE];
	const int macro_ptr = collect_mident(env, buffer);
	const int num = m_equal(env, temp_str);
	if (num != 0)
	{
		env_io_add_char(env, macro_text_type, MACRO_CANGE);
		env_io_add_char(env, macro_text_type, num - 1);
	}
	else if (!flag_macro_directive && macro_ptr)
	{
		if (macro_get(env, macro_ptr))
		{
			return -1;
		}
	}
	else
	{
		size_t i = 0;
		while(buffer[i] != MACRO_END)
		{
			env_io_add_char(env, macro_text_type, buffer[i++]);
		}
	}

	return 0;
}

int func_add_ident(environment *const env, int *temp_str)
{
	int num = 0;
	int temp_str_size = 0;

	while (env->curchar != ')')
	{
		if (utf8_is_letter(env->curchar))
		{
			while (utf8_is_letter(env->curchar) || utf8_is_digit(env->curchar))
			{
				temp_str[temp_str_size++] = env->curchar;
				m_nextch(env);
			}
			temp_str[temp_str_size++] = 0;
		}
		else
		{
			env_error(env, functionid_begins_with_letters);
			return -1;
		}

		if (env->curchar == ',')
		{
			m_nextch(env);
			skip_separators(env);
			num++;
		}
		else if (env->curchar != ')')
		{
			env_error(env, after_functionid_must_be_comma);
			return -1;
		}
	}
	temp_str[temp_str_size++] = -1;
	
	m_nextch(env);
	return num;
}

int macro_tab_add_func(environment *const env)
{
	const int flag_macro_directive = env->cur == SH_MACRO;

	
	env_io_add_char(env, macro_text_type, MACRO_FUNCTION);
	
	int empty = 0;
	int temp_str[STRING_SIZE];
	if (env->curchar == ')')
	{
		env_io_add_char(env, macro_text_type, -1);
		empty = 1;
		m_nextch(env);
	}
	else
	{
		const int res = func_add_ident(env, temp_str);
		if (res == -1)
		{
			return -1;
		}
		env_io_add_char(env, macro_text_type, res);
	}
	skip_separators(env);

	while ((env->curchar != '\n' || flag_macro_directive) && env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar) && !empty)
		{
			if (func_check_macro(env, flag_macro_directive, temp_str))
			{
				return -1;
			}
		}
		else if (env->curchar == '#')
		{
			env->cur = macro_keywords(env);

			if (!flag_macro_directive && env->cur == SH_EVAL && env->curchar == '(')
			{
				char buffer[STRING_SIZE];
				if (calculate_arithmetic(env, buffer))
				{
					return -1;
				}

				size_t lenght = strlen(buffer);
				for (size_t i = 0; i < lenght; i++)
				{
					env_io_add_char(env, macro_text_type, buffer[i]);
				}
			}
			else if (flag_macro_directive && env->cur == SH_ENDM)
			{
				m_nextch(env);
				env_io_add_char(env, macro_text_type, MACRO_END);
				return 0;
			}
			else
			{
				env->cur = 0;
				for (size_t i = 0; i < (size_t)env->reprtab[env->rp]; i++)
				{
					env_io_add_char(env, macro_text_type, env->reprtab[env->rp + 2 + i]);
				}
			}
		}
		else
		{
			env_io_add_char(env, macro_text_type, env->curchar);
			m_nextch(env);
		}

		if (env->curchar == EOF)
		{
			env_error(env, not_end_fail_define);
			return -1;
		}

		if (env->curchar == '\\')
		{
			m_nextch(env);
			skip_line(env);
			if (skip_line(env))
			{
				return -1;
			}
			//env->macro_tab[env->macro_ptr++] = '\n';
			m_nextch(env);
		}
	}

	env_io_add_char(env, macro_text_type, MACRO_END);
	return 0;
}

int define_add_to_reprtab(environment *const env)
{
	if (!utf8_is_letter(env->curchar))
	{
		env_error(env, ident_begins_with_letters);
		return -1;
	}

	
	int oldrepr = env->rp;
	int hash = 0;
	env->rp += 2;

	do
	{
		hash += env->curchar;
		env->reprtab[env->rp++] = env->curchar;
		m_nextch(env);
	} while (utf8_is_letter(env->curchar) || utf8_is_digit(env->curchar));

	hash &= 255;
	env->reprtab[env->rp++] = 0;
	int r = env->hashtab[hash];

	while (r)
	{
		if (equal_reprtab(r, oldrepr, env))
		{
			if (env->macro_tab[env->reprtab[r + 1]] == MACRO_UNDEF)
			{
				env->rp = oldrepr;
				return r;
			}
			else
			{
				env_error(env, repeat_ident);
				return -1;
			}
		}
		r = env->reprtab[r];
	}

	env->reprtab[oldrepr] = env->hashtab[hash];
	env->reprtab[oldrepr + 1] = (int)env_get_io_size(env, macro_text_type);
	env->hashtab[hash] = oldrepr;
	return 0;
}

int macro_tab_add_define(environment *const env, const int rep_ptr)
{
	int old_macro_tab_size = (int)env_get_io_size(env, macro_text_type);

	env_io_add_char(env, macro_text_type, MACRO_DEF);
	if (env->curchar != '\n')
	{
		while (env->curchar != '\n')
		{
			if (env->curchar == EOF)
			{
				env_error(env, not_end_fail_define);
				return -1;
			}
			else if (env->curchar == '#')
			{
				env->cur = macro_keywords(env);
				if (env->cur == SH_EVAL)
				{
					if (env->curchar != '(')
					{
						env_error(env, after_eval_must_be_ckob);
						return -1;
					}

					char buffer[STRING_SIZE];
					if (calculate_arithmetic(env, buffer))
					{
						return -1;
					}

					size_t lenght = strlen(buffer);
					for (size_t i = 0; i < lenght; i++)
					{
						env_io_add_char(env, macro_text_type, buffer[i]);
					}
				}
				else
				{
					for (size_t i = 0; i < (size_t)env->reprtab[env->rp]; i++)
					{
						env_io_add_char(env, macro_text_type, env->reprtab[env->rp + 2 + i]);
					}
				}
			}
			else if (env->curchar == '\\')
			{
				m_nextch(env);
				if (skip_line(env))
				{
					return -1;
				}
				//env_io_add_char(env, macro_text_type, '\n';
				m_nextch(env);
			}
			else if (utf8_is_letter(env->curchar))
			{
				char32_t buffer[STRING_SIZE];
				const int macro_ptr = collect_mident(env, buffer);
				if (macro_ptr && macro_get(env, macro_ptr))
				{
					return -1;
				}
				else
				{
					size_t i = 0;
					while(buffer[i] != MACRO_END)
					{
						env_io_add_char(env, macro_text_type, buffer[i++]);
					}
				}
			}
			else
			{
				env_io_add_char(env, macro_text_type, env->curchar);
				m_nextch(env);
			}
		}

		
	}
	else
	{
		env_io_add_char(env, macro_text_type, '0');
	}

	env_io_add_char(env, macro_text_type, MACRO_END);

	if (rep_ptr)
	{
		env->reprtab[rep_ptr + 1] = old_macro_tab_size;
	}
	return 0;
}

int macro_add(environment *const env)
{
	const int r = define_add_to_reprtab(env);
	if (r == -1)
	{
		return -1;
	}

	if (env->curchar == '(' && !r)
	{
		m_nextch(env);
		return macro_tab_add_func(env);
	}
	else if (env->curchar != ' ' && env->curchar != '\n' && env->curchar != '\t')
	{
		env_error(env, after_ident_must_be_space);
		return -1;
	}
	else
	{
		skip_separators(env);
		return macro_tab_add_define(env, r);
	}
}

int macro_set(environment *const env)
{
	skip_separators(env);

	if (!utf8_is_letter(env->curchar))
	{
		env_error(env, ident_begins_with_letters);
		return -1;
	}

	char32_t buffer[STRING_SIZE];
	const int macro_ptr = collect_mident(env, buffer);	
	if (env->macro_tab[env->reprtab[macro_ptr + 1]] == MACRO_FUNCTION)//==0 ==UNDEF
	{
		env_error(env, functions_cannot_be_changed);
		return -1;
	}
	else if (env->curchar != ' ' && env->curchar != '\t')
	{
		env_error(env, after_ident_must_be_space);
		return -1;
	}

	m_nextch(env);
	skip_separators(env);

	return macro_tab_add_define(env, macro_ptr);
}
