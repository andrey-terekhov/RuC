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

#include "define.h"
#include "calculator.h"
#include "constants.h"
#include "file.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int define_get_from_macrotext(int r, environment *env);


int m_equal(environment *env)
{
	int i = 0;
	int n = 1;
	int j = 0;

	while (j < env->csp)
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

// define c параметрами (function)
int function_scob_collect(int t, int num, environment *env)
{
	int i;

	while (env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar))
		{
			int r = collect_mident(env);

			if (r)
			{
				int oldcp1 = env->cp;
				int oldlsp = env->lsp;
				int locfchange[STRING_SIZE];
				int lcp = 0;
				int ldip;

				env->lsp += num;
				if(define_get_from_macrotext(r, env))
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

				for (i = 0; i < lcp; i++)
				{
					env->fchange[env->cp++] = locfchange[i];
				}
			}
			else
			{
				for (i = 0; i < env->msp; i++)
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
				if(calculator(0, env))
				{
					return -1;
				}
				for (i = 0; i < env->csp; i++)
				{
					env->fchange[env->cp++] = env->cstring[i];
				}
			}
			else
			{
				for (i = 0; i < env->reprtab[env->rp]; i++)
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
	size_t position = skip_str(env); 
	macro_error(scob_not_clous, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
	return -1;
}

int function_stack_create(int n, environment *env)
{
	int num = 0;

	m_nextch(env);
	env->localstack[num + env->lsp] = env->cp;

	if (env->curchar == ')')
	{
		size_t position = skip_str(env); 
		macro_error(stalpe, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
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
				size_t position = skip_str(env); 
				macro_error(not_enough_param, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
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
				size_t position = skip_str(env); 
				macro_error(not_enough_param2, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
				return -1;
			}
			m_nextch(env);

			env->cp = env->localstack[env->lsp];
			return 0;
		}
	}

	size_t position = skip_str(env); 
	macro_error(scob_not_clous, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
	return -1;
}

int funktionleter(int flag_macro, environment *env)
{
	int n = 0;
	int i = 0;

	env->msp = 0;

	int r = collect_mident(env);

	if ((n = m_equal(env)) != 0)
	{
		env->macrotext[env->mp++] = MACROCANGE;
		env->macrotext[env->mp++] = n - 1;
	}
	else if (!flag_macro && r)
	{
		if(define_get_from_macrotext(r, env))
		{
			return -1;
		}
	}
	else
	{
		for (i = 0; i < env->msp; i++)
		{
			env->macrotext[env->mp++] = env->mstring[i];
		}
	}
	return 0;
}

int to_functionident(environment *env)
{
	int num = 0;
	env->csp = 0;

	while (env->curchar != ')')
	{
		env->msp = 0;

		if (utf8_is_letter(env->curchar))
		{
			while (utf8_is_letter(env->curchar) || utf8_is_digit(env->curchar))
			{
				env->cstring[env->csp++] = env->curchar;
				m_nextch(env);
			}
			env->cstring[env->csp++] = 0;
		}
		else
		{
			size_t position = skip_str(env); 
			macro_error(functionid_begins_with_letters, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
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
			size_t position = skip_str(env); 
			macro_error(after_functionid_must_be_comma, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
			return -1;
		}
	}
	m_nextch(env);
	return num;
}

int macrotext_add_function(environment *env)
{
	int j;
	int flag_macro = 0;
	int empty = 0;

	if (env->cur == SH_MACRO)
	{
		flag_macro = 1;
	}

	env->macrotext[env->mp++] = MACROFUNCTION;

	if (env->curchar == ')')
	{
		env->macrotext[env->mp++] = -1;
		empty = 1;
		m_nextch(env);
	}
	else
	{	
		int res = to_functionident(env);
		if(res == -1)
		{
			return -1;
		}
		env->macrotext[env->mp++] = res;
	}
	skip_space(env);

	while ((env->curchar != '\n' || flag_macro) && env->curchar != EOF)
	{
		if (utf8_is_letter(env->curchar) && !empty)
		{
			if(funktionleter(flag_macro, env))
			{
				return -1;
			}
		}
		else if (env->curchar == '#')
		{
			env->cur = macro_keywords(env);

			if (!flag_macro && env->cur == SH_EVAL && env->curchar == '(')
			{
				if(calculator(0, env))
				{
					return -1;
				}
				for (j = 0; j < env->csp; j++)
				{
					env->macrotext[env->mp++] = env->cstring[j];
				}
			}
			else if (flag_macro && env->cur == SH_ENDM)
			{
				m_nextch(env);
				env->macrotext[env->mp++] = MACROEND;
				return 0;
			}
			else
			{
				env->cur = 0;
				for (j = 0; j < env->reprtab[env->rp]; j++)
				{
					env->macrotext[env->mp++] = env->reprtab[env->rp + 2 + j];
				}
			}
		}
		else
		{
			env->macrotext[env->mp++] = env->curchar;
			m_nextch(env);
		}

		if (env->curchar == EOF)
		{
			size_t position = skip_str(env); 
			macro_error(not_end_fail_define, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
			return -1;
		}

		if (env->curchar == '\\')
		{
			m_nextch(env);
			space_end_line(env);
			if(space_end_line(env))
			{
				return -1;
			}
			//env->macrotext[env->mp++] = '\n';
			m_nextch(env);
		}
	}

	env->macrotext[env->mp++] = MACROEND;
	return 0;
}
//

// define
int define_get_from_macrotext(int r, environment *env)
{
	int t = env->reprtab[r + 1];

	if (r)
	{
		env->msp = 0;
		if (env->macrotext[t] == MACROFUNCTION)
		{
			if (env->macrotext[++t] > -1)
			{
				
				if(function_stack_create(env->macrotext[t], env))
				{
					return -1;
				}
			}
		}

		m_change_nextch_type(TEXTTYPE, t + 1, env);
		m_nextch(env);
	}
	else
	{
		size_t position = skip_str(env); 
		macro_error(ident_not_exist, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
		return -1;
	}

	return 0;
}

int define_add_to_reprtab(environment *env)
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
				size_t position = skip_str(env); 
				macro_error(repeat_ident, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
				return -1;
			}
		}
		r = env->reprtab[r];
	}

	env->reprtab[oldrepr] = env->hashtab[hash];
	env->reprtab[oldrepr + 1] = env->mp;
	env->hashtab[hash] = oldrepr;
	return 0;
}

int macrotext_add_define(int r, environment *env)
{
	int j;
	int lmp = env->mp;

	env->macrotext[env->mp++] = MACRODEF;
	if (env->curchar != '\n')
	{
		while (env->curchar != '\n')
		{
			if (env->curchar == EOF)
			{
				size_t position = skip_str(env); 
				macro_error(not_end_fail_define, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
				return -1;
			}
			else if (env->curchar == '#')
			{
				env->cur = macro_keywords(env);
				if (env->cur == SH_EVAL)
				{
					if (env->curchar != '(')
					{
						size_t position = skip_str(env); 
						macro_error(after_eval_must_be_ckob, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
						return -1;
					}

					if(calculator(0, env))
					{
						return -1;
					}

					for (j = 0; j < env->csp; j++)
					{
						env->macrotext[env->mp++] = env->cstring[j];
					}
				}
				else
				{
					for (j = 0; j < env->reprtab[env->rp]; j++)
					{
						env->macrotext[env->mp++] = env->reprtab[env->rp + 2 + j];
					}
				}
			}
			else if (env->curchar == '\\')
			{
				m_nextch(env);
				if(space_end_line(env))
				{
					return -1;
				}
				//env->macrotext[env->mp++] = '\n';
				m_nextch(env);
			}
			else if (utf8_is_letter(env->curchar))
			{
				int k = collect_mident(env);
				if (k)
				{
					if(define_get_from_macrotext(k, env))
					{
						return -1;
					}
				}
				else
				{
					for (j = 0; j < env->msp; j++)
					{
						env->macrotext[env->mp++] = env->mstring[j];
					}
				}
			}
			else
			{
				env->macrotext[env->mp++] = env->curchar;
				m_nextch(env);
			}
		}

		while (env->macrotext[env->mp - 1] == ' ' || env->macrotext[env->mp - 1] == '\t')
		{
			env->macrotext[env->mp - 1] = MACROEND;
			env->mp--;
		}
	}
	else
	{
		env->macrotext[env->mp++] = '0';
	}

	env->macrotext[env->mp++] = MACROEND;

	if (r)
	{
		env->reprtab[r + 1] = lmp;
	}
	return 0;
}

int define_realiz(environment *env)
{
	int r;

	if (!utf8_is_letter(env->curchar))
	{
		size_t position = skip_str(env); 
		macro_error(ident_begins_with_letters, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
		return -1;
	}

	r = define_add_to_reprtab(env);
	if(r == -1)
	{
		return -1;
	}

	env->msp = 0;

	if (env->curchar == '(' && !r)
	{
		m_nextch(env);
		if(macrotext_add_function(env))
		{
			return -1;
		}
	}
	else if (env->curchar != ' ' && env->curchar != '\n' && env->curchar != '\t')
	{
		size_t position = skip_str(env); 
		macro_error(after_ident_must_be_space, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
		return -1;
	}
	else
	{
		skip_space(env);
		return macrotext_add_define(r, env);
	}
	return 0;
}

int set_realiz(environment *env)
{
	int j;

	skip_space(env);

	if (!utf8_is_letter(env->curchar))
	{
		size_t position = skip_str(env); 
		macro_error(ident_begins_with_letters, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
		return -1;
	}

	j = collect_mident(env);

	if (env->macrotext[env->reprtab[j + 1]] == MACROFUNCTION)
	{
		size_t position = skip_str(env); 
		macro_error(functions_cannot_be_changed, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
		return -1;
	}
	else if (env->curchar != ' ' && env->curchar != '\t')
	{	
		size_t position = skip_str(env);
		macro_error(after_ident_must_be_space, lk_get_current(&env->lk)
			, env->error_string, env->line, position);
		return -1;
	}

	m_nextch(env);
	skip_space(env);

	return macrotext_add_define(j, env);
}
//
