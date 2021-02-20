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
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "macro_load.h"
#include "linker.h"
#include "utils.h"


int m_equal(environment *const env)
{
	int i = 0;
	int n = 1;
	int j = 0;

	while (j < env->calc_prt)
	{
		while (env->mstring[i] == env->cstring[j])
		{
			i++;
			j++;
			if (env->mstring[i] == MACROEND && env->cstring[j] == 0)
			{
				return n;
			}
		}
		i++;
		j++;

		n++;
		i = 0;
		if (env->cstring[j++] != 0)
		{
			while (env->cstring[j++] != 0)
			{
				;
			}
		}
	}

	return 0;
}

int fn_check_macros(environment *const env, int flag_macro)
{
	int num = 0;
	env->msp = 0;

	const int macros_prt = collect_mident(env);

	if ((num = m_equal(env)) != 0)
	{
		env->macrotext[env->macro_prt++] = MACROCANGE;
		env->macrotext[env->macro_prt++] = num - 1;
	}
	else if (!flag_macro && macros_prt)
	{
		if(macros_get(env, macros_prt))
		{
			return -1;
		}
	}
	else
	{
		for (int i = 0; i < env->msp; i++)
		{
			env->macrotext[env->macro_prt++] = env->mstring[i];
		}
	}
	return 0;
}

int fn_add_ident(environment *const env)
{
	int num = 0;
	env->calc_prt = 0;

	while (env->curchar != ')')
	{
		env->msp = 0;

		if (utf8_is_letter(env->curchar))
		{
			while (utf8_is_letter(env->curchar) || utf8_is_digit(env->curchar))
			{
				env->cstring[env->calc_prt++] = env->curchar;
				m_nextch(env);
			}
			env->cstring[env->calc_prt++] = 0;
		}
		else
		{
			const size_t position = env_skip_str(env);
			macro_error(functionid_begins_with_letters, env_get_current_file(env), env->error_string, env->line, position);
			return -1;
		}

		env->msp = 0;
		if (env->curchar == ',')
		{
			m_nextch(env);
			skip_space(env);
			num++;
		}
		else if (env->curchar != ')')
		{
			const size_t position = env_skip_str(env);
			macro_error(after_functionid_must_be_comma, env_get_current_file(env), env->error_string, env->line, position);
			return -1;
		}
	}
	m_nextch(env);
	return num;
}

int macrotext_add_function(environment *const env)
{
	int flag_macro = 0;
	int empty = 0;

	if (env->cur == SH_MACRO)
	{
		flag_macro = 1;
	}

	env->macrotext[env->macro_prt++] = MACROFUNCTION;

	if (env->curchar == ')')
	{
		env->macrotext[env->macro_prt++] = -1;
		empty = 1;
		m_nextch(env);
	}
	else
	{
		int res = fn_add_ident(env);
		if (res == -1)
		{
			return -1;
		}
		env->macrotext[env->macro_prt++] = res;
	}
	skip_space(env);

	while ((env->curchar != '\n' || flag_macro) && env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar) && !empty)
		{
			if (fn_check_macros(env, flag_macro))
			{
				return -1;
			}
		}
		else if (env->curchar == '#')
		{
			env->cur = macro_keywords(env);

			if (!flag_macro && env->cur == SH_EVAL && env->curchar == '(')
			{
				if (calculate(env, 0))
				{
					return -1;
				}
				for (int i = 0; i < env->calc_prt; i++)
				{
					env->macrotext[env->macro_prt++] = env->cstring[i];
				}
			}
			else if (flag_macro && env->cur == SH_ENDM)
			{
				m_nextch(env);
				env->macrotext[env->macro_prt++] = MACROEND;
				return 0;
			}
			else
			{
				env->cur = 0;
				for (int i = 0; i < env->reprtab[env->rp]; i++)
				{
					env->macrotext[env->macro_prt++] = env->reprtab[env->rp + 2 + i];
				}
			}
		}
		else
		{
			env->macrotext[env->macro_prt++] = env->curchar;
			m_nextch(env);
		}

		if (env->curchar == EOF)
		{
			const size_t position = env_skip_str(env);
			macro_error(not_end_fail_define, env_get_current_file(env), env->error_string, env->line, position);
			return -1;
		}

		if (env->curchar == '\\')
		{
			m_nextch(env);
			skip_space_end_line(env);
			if (skip_space_end_line(env))
			{
				return -1;
			}
			//env->macrotext[env->macro_prt++] = '\n';
			m_nextch(env);
		}
	}

	env->macrotext[env->macro_prt++] = MACROEND;
	return 0;
}

int define_add_to_reprtab(environment *const env)
{
	int r;
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
	r = env->hashtab[hash];

	while (r)
	{
		if (equal_reprtab(r, oldrepr, env))
		{
			if (env->macrotext[env->reprtab[r + 1]] == MACROUNDEF)
			{
				env->rp = oldrepr;
				return r;
			}
			else
			{
				const size_t position = env_skip_str(env);
				macro_error(repeat_ident, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
		}
		r = env->reprtab[r];
	}

	env->reprtab[oldrepr] = env->hashtab[hash];
	env->reprtab[oldrepr + 1] = env->macro_prt;
	env->hashtab[hash] = oldrepr;
	return 0;
}

int macrotext_add_define(environment *const env, int rep_prt)
{
	int lmp = env->macro_prt;

	env->macrotext[env->macro_prt++] = MACRODEF;
	if (env->curchar != '\n')
	{
		while (env->curchar != '\n')
		{
			if (env->curchar == EOF)
			{
				const size_t position = env_skip_str(env);
				macro_error(not_end_fail_define, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
			else if (env->curchar == '#')
			{
				env->cur = macro_keywords(env);
				if (env->cur == SH_EVAL)
				{
					if (env->curchar != '(')
					{
						const size_t position = env_skip_str(env);
						macro_error(after_eval_must_be_ckob, env_get_current_file(env), env->error_string, env->line, position);
						return -1;
					}

					if (calculate(env, 0))
					{
						return -1;
					}

					for (int i = 0; i < env->calc_prt; i++)
					{
						env->macrotext[env->macro_prt++] = env->cstring[i];
					}
				}
				else
				{
					for (int i = 0; i < env->reprtab[env->rp]; i++)
					{
						env->macrotext[env->macro_prt++] = env->reprtab[env->rp + 2 + i];
					}
				}
			}
			else if (env->curchar == '\\')
			{
				m_nextch(env);
				if (skip_space_end_line(env))
				{
					return -1;
				}
				//env->macrotext[env->macro_prt++] = '\n';
				m_nextch(env);
			}
			else if (utf8_is_letter(env->curchar))
			{
				const int macros_prt = collect_mident(env);
				if (macros_prt && macros_get(env, macros_prt))
				{
					return -1;
				}
				else
				{
					for (int i = 0; i < env->msp; i++)
					{
						env->macrotext[env->macro_prt++] = env->mstring[i];
					}
				}
			}
			else
			{
				env->macrotext[env->macro_prt++] = env->curchar;
				m_nextch(env);
			}
		}

		while (env->macrotext[env->macro_prt - 1] == ' ' || env->macrotext[env->macro_prt - 1] == '\t')
		{
			env->macrotext[env->macro_prt - 1] = MACROEND;
			env->macro_prt--;
		}
	}
	else
	{
		env->macrotext[env->macro_prt++] = '0';
	}

	env->macrotext[env->macro_prt++] = MACROEND;

	if (rep_prt)
	{
		env->reprtab[rep_prt + 1] = lmp;
	}
	return 0;
}

int macros_add(environment *const env)
{
	int r;

	if (!utf8_is_letter(env->curchar))
	{
		const size_t position = env_skip_str(env);
		macro_error(ident_begins_with_letters, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	r = define_add_to_reprtab(env);
	if (r == -1)
	{
		return -1;
	}

	env->msp = 0;

	if (env->curchar == '(' && !r)
	{
		m_nextch(env);
		return macrotext_add_function(env);
	}
	else if (env->curchar != ' ' && env->curchar != '\n' && env->curchar != '\t')
	{
		const size_t position = env_skip_str(env);
		macro_error(after_ident_must_be_space, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}
	else
	{
		skip_space(env);
		return macrotext_add_define(env, r);
	}
}

int macros_set(environment *const env)
{
	skip_space(env);

	if (!utf8_is_letter(env->curchar))
	{
		const size_t position = env_skip_str(env);
		macro_error(ident_begins_with_letters, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	const int macros_prt = collect_mident(env);	

	if (env->macrotext[env->reprtab[macros_prt + 1]] == MACROFUNCTION)
	{
		const size_t position = env_skip_str(env);
		macro_error(functions_cannot_be_changed, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}
	else if (env->curchar != ' ' && env->curchar != '\t')
	{
		const size_t position = env_skip_str(env);
		macro_error(after_ident_must_be_space, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}

	m_nextch(env);
	skip_space(env);

	return macrotext_add_define(env, macros_prt);
}
