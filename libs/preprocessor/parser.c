/*
 *	Copyright 2018 Andrey Terekhov, Egor Anikin
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

#include "parser.h"
#include <string.h>
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "macro_load.h"
#include "macro_save.h"
#include "uniprinter.h"
#include "utils.h"


#define WHILE_BEGIN		'\r'
#define WHILE_END		(char32_t)EOF


int preprocess_token(linker *const lk);


int if_check(environment *const env, int type_if)
{
	int flag = 0;

	if (type_if == SH_IF)
	{
		return calculate_logic(env);
	}
	else
	{
		char32_t buffer[STRING_SIZE];
		if (collect_mident(env, buffer))
		{
			flag = 1;
		}

		if (skip_line(env))
		{
			return -1;
		}

		if (type_if == SH_IFDEF)
		{
			return flag;
		}
		else
		{
			return 1 - flag;
		}
	}
}

int if_end(environment *const env)
{
	int fl_cur;

	while (env_get_curchar(env) != (char32_t)EOF)
	{
		if (env_get_curchar(env) == '#')
		{
			fl_cur = macro_keywords(env);
			if (fl_cur == SH_ENDIF)
			{
				env->nested_if--;
				if (env->nested_if < 0)
				{
					env_error(env, before_endif);
					return -1;
				}
				return 0;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				env->nested_if++;
				if (if_end(env))
				{
					return -1;
				}
			}
		}
		else
		{
			env_scan_next_char(env);
		}
	}

	env_error(env, must_be_endif);
	return -1;
}

int if_false(environment *const env)
{
	int fl_cur = env->cur;

	while (env_get_curchar(env) != (char32_t)EOF)
	{
		if (env_get_curchar(env) == '#')
		{
			fl_cur = macro_keywords(env);
			env_scan_next_char(env);

			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}

			if ((fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF) && if_end(env))
			{
				return 0;
			}
		}
		else
		{
			env_scan_next_char(env);
		}
	}

	
	env_error(env, must_be_endif);
	return 0;
}

int if_true(linker *const lk, const int type_if)
{
	environment *env = lk->env;
	int error = 0;
	while (env_get_curchar(env) != (char32_t)EOF)
	{
		error = preprocess_token(lk);
		if (error)
		{
			return error;
		}

		if (env->cur == SH_ELSE || env->cur == SH_ELIF)
		{
			break;
		}

		if (env->cur == SH_ENDIF)
		{
			env->nested_if--;
			if (env->nested_if < 0)
			{
				env_error(env, before_endif);
				return -1;
			}

			return 0;
		}
	}

	if (type_if != SH_IF && env->cur == SH_ELIF)
	{
		env_error(env, dont_elif);
		env->nested_if--;
		return -1;
	}

	return if_end(env);
}

int if_implementation(linker *const lk)
{
	environment *env = lk->env;
	const int type_if = env->cur;
	const int truth_flag = if_check(env, type_if); // начало (if)
	if (truth_flag == -1)
	{
		env->nested_if--;
		return -1;
	}

	env->nested_if++;
	if (truth_flag)
	{
		return if_true(lk, type_if);
	}
	else
	{
		const int res = if_false(env);
		if (!res)
		{
			env->nested_if--;
			return -1;
		}
		env->cur = res;
	}


	while (env->cur == SH_ELIF)
	{
		const int el_truth_flag = if_check(env, type_if);
		if (el_truth_flag == -1 || skip_line(env))
		{
			env->nested_if--;
			return -1;
		}

		if (el_truth_flag)
		{
			return if_true(lk, type_if);
		}
		else
		{
			const int res = if_false(env);
			if (!res)
			{
				env->nested_if--;
				return -1;
			}
			env->cur = res;
		}
	}


	if (env->cur == SH_ELSE)
	{
		env->cur = 0;
		return if_true(lk, type_if);;
	}

	if (env->cur == SH_ENDIF)
	{
		env->nested_if--;
		if (env->nested_if < 0)
		{
			env_error(env, before_endif);
			return -1;
		}
	}
	return 0;
}


int while_collect(environment *const env)
{
	env_io_add_char(env, while_type, WHILE_BEGIN);
	size_t while_begin_prt = env_io_get_size(env, while_type);
	env_io_add_char(env, while_type, '0');
	env_io_add_char(env, while_type, (char32_t)env_io_get_size(env, if_type));
	

	while (env_get_curchar(env) != '\n')
	{
		env_io_add_char(env, if_type, env_get_curchar(env));
		env_scan_next_char(env);
	}
	env_io_add_char(env, if_type, '\n');
	env_scan_next_char(env);

	while (env_get_curchar(env) != (char32_t)EOF)
	{
		if (env_get_curchar(env) == '#')
		{
			int cur = macro_keywords(env);

			if (cur == SH_WHILE)
			{
				while_collect(env);
			}
			else if (cur == SH_ENDW)
			{
				env_io_set_char(env, while_type, while_begin_prt, (char32_t)env_io_get_size(env, while_type));
				env_io_add_char(env, while_type, WHILE_END);
				//env_io_add_char(env, while_type, WHILE_END);
				return 0;
			}
			else
			{
				for (size_t i = 0; i < (size_t)env->reprtab[env->rp]; i++)
				{
					env_io_add_char(env, while_type, (char32_t)env->reprtab[env->rp + 2 + i]);
				}
			}
		}

		env_io_add_char(env, while_type, env_get_curchar(env));
		env_scan_next_char(env);
	}

	env_error(env, must_end_endw);
	return -1;
}

int while_implementation(linker *const lk, size_t begin_pred)
{
	environment *env = lk->env;
	env_scan_next_char(env);
	size_t end = (size_t)env_get_curchar(env);
	env_scan_next_char(env);
	size_t if_prt = env_get_curchar(env);
	size_t begin  = env_io_get_prt(env) + begin_pred -1;
	while (1)
	{
		env_io_switch_to_new(env, if_type, if_prt);
		int res = calculate_logic(env);
		env_io_back_to_previous(env);

		if (res == -1)
		{
			return -1;
		}
		if (res == 0)
		{
			env_io_back_to_previous(env);
			env_io_switch_to_new(env, while_type, end);
			return 0;
		}

		env_io_back_to_previous(env);
		env_io_switch_to_new(env, while_type, begin);

		skip_separators(env);

		while (env_get_curchar(env) != WHILE_END)
		{
			if (env_get_curchar(env) == WHILE_BEGIN)
			{
				if (while_implementation(lk, begin))
				{
					return -1;
				}
				env_scan_next_char(env);
			}
			else if (env_get_curchar(env) == (char32_t)EOF)
			{
				env_error(env, must_end_endw);
				return -1;
			}
			else
			{
				int error = preprocess_token(lk);
				if (error)
				{
					return error;
				}
			}
		}
	}
	return 0;
}

int parser_include(linker *const lk)
{
	size_t index = lk_include(lk);
	environment *env = lk->env;

	if (index >= SIZE_MAX - 1)
	{
		env_scan_next_char(env);
		return index == SIZE_MAX ? 0 : -1;
	}

	int flag_io_type = 0;
	if (env_io_get_type(env) != file_type)
	{
		env_io_switch_to_new(env, file_type, 0);
		flag_io_type++;
	}

	

	const int res = preprocess_file(lk, index);

	if (res == -1)
	{
		end_of_file(env);
		return -1;
	}
	
	if (flag_io_type)
	{
		env_io_back_to_previous(env);
	}

	env_scan_next_char(env);
	env_scan_next_char(env);

	return 0;
}


int preprocess_words(linker *const lk)
{
	environment *env = lk->env;
	skip_separators(env);
	switch (env->cur)
	{
		case SH_INCLUDE:
		{
			return parser_include(lk);
		}
		case SH_DEFINE:
		case SH_MACRO:
		{
			return macro_add(env);
		}
		case SH_UNDEF:
		{
			char32_t buffer[STRING_SIZE];
			const int macro_ptr = collect_mident(env, buffer);
			if (macro_ptr)
			{
				env_io_set_char(env, macro_text_type, env->reprtab[macro_ptr + 1], MACRO_UNDEF);
				return skip_line(env);
			}
			else
			{
				env_error(env, macro_does_not_exist);
				return -1;
			}
		}
		case SH_IF:
		case SH_IFDEF:
		case SH_IFNDEF:
		{
			return if_implementation(lk);
		}
		case SH_SET:
		{
			return macro_set(env);
		}
		case SH_ELSE:
		case SH_ELIF:
		case SH_ENDIF:
			return 0;
		case SH_EVAL:
		{
			if (env_get_curchar(env) != '(')
			{
				env_error(env, after_eval_must_be_ckob);
				return -1;
			}
			char buffer[STRING_SIZE];
			if (calculate_arithmetic(env, buffer))
			{
				return -1;
			}

			uni_printf(env_get_file_output(env), "%s", buffer);
			return 0;
		}
		case SH_WHILE:
		{
			char32_t if_io[STRING_SIZE * 2];
			char32_t while_io[STRING_SIZE * 5];
			env_io_set(env, if_io, if_type);
			env_io_set(env, while_io, while_type);

			if (while_collect(env))
			{
				return -1;
			}

			env_io_switch_to_new(env, while_type, 0);

			int res = while_implementation(lk, 0);
			if (env_io_get_type(env) != file_type)
			{
				env_io_back_to_previous(env);
			}

			env_io_clear(env, if_type, 0);
			env_io_clear(env, while_type, 0);
			env_io_add_char(env, if_type, '\0'); 
			return res;
		}
		default:
		{
			//output_keywords(env);
			env_error(env, preproces_words_not_exist);
			return 0;
		}
	}
}

int preprocess_token(linker *const lk)
{
	environment *env = lk->env;
	switch (env_get_curchar(env))
	{
		case (char32_t)EOF:
			return 0;

		case '#':
		{
			env->cur = macro_keywords(env);

			if (env->cur != 0)
			{
				const int res = preprocess_words(lk);
				if (env_get_nextchar(env) != '#' && env_io_get_type(env) != while_type &&
					env_io_get_type(env) != macro_text_type)//curflag
				{
					env_add_comment(env);
				}
				if (env->cur != SH_INCLUDE && env->cur != SH_ELSE && env->cur != SH_ELIF && env->cur != SH_ENDIF)
				{
					env_scan_next_char(env);
				}
				return res;
			}
			else
			{
				// m_nextch(env);
				output_keywords(env);
			}

			return 0;
		}
		case '\'':
		case '\"':
		{
			return skip_string(env);
		}
		case '@':
		{
			env_scan_next_char(env);
			return 0;
		}
		default:
		{
			if (utf8_is_letter(env_get_curchar(env)))
			{
				char32_t buffer[STRING_SIZE];
				const int macro_ptr = collect_mident(env, buffer);
				if (macro_ptr)
				{
					return macro_get(env, macro_ptr);
				}
				else
				{
					size_t buffer_size = 0;
					while(buffer[buffer_size] != '\0')
					{
						m_fprintf(env, buffer[buffer_size]);
						buffer_size++;
					}
				}
			}
			else
			{
				m_fprintf(env, env_get_curchar(env));
				env_scan_next_char(env);
			}

			return 0;
		}
	}
}

int preprocess_file(linker *const lk, const size_t number)
{
	environment *env = lk->env;

	universal_io new_in = io_create();
	universal_io *old_in = env_get_file_input(env);
	env_set_file_input(env, &new_in, lk_get_path(lk, number));

	if (lk_open_file(lk, number))
	{
		return -1;
	}

	const size_t old_cur = lk_get_current(lk);
	lk_set_current(lk, number);

	const size_t old_line = env->line;
	env->line = 1;

	env_scan_next_char(env);
	env_scan_next_char(env);
	if (env_get_curchar(env) != '#')
	{
		env_add_comment(env);
	}

	int was_error = 0;
	while (env_get_curchar(env) != (char32_t)EOF)
	{
		was_error = preprocess_token(lk) || was_error;
	}

	m_fprintf(env, '\n');

	env->line = old_line;
	lk_set_current(lk, old_cur);

	in_clear(env_get_file_input(env));
	env_set_file_input(env, old_in, lk_get_path(lk, old_cur));
	return was_error ? -1 : 0;
}

