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


#define MAX_CMT_SIZE 256


void env_init(environment *const env, universal_io *const output)
{
	env->output = output;

	env->curent_io_type = file_type;
	env->curent_io = NULL;
	env->depth_io = 0;
	env->curent_io_prt = 0;
	env->local_io[macro_text_type] = env->macro_tab;
	env->local_io[param_type] = env->change;
	env->local_io[if_type] = NULL;
	env->local_io[while_type] = NULL;
	env->define_stack_prt = 0;
	env->depth_io = 0;

	env->curchar = 0;
	env->nextchar = 0;
	env->cur = 0;
	
	env->line = 1;
	env->position = 0;
	env->nested_if = 0;
	env->error_string[0] = '\0';
	env->prep_flag = 0;
	env->rp = 1;

	for (size_t i = 0; i < all_types; i++)
	{
		env->local_io_size[i] = 0;
	}
	env->local_io_size[macro_text_type] = 1;


	for (size_t i = 0; i < HASH; i++)
	{
		env->hashtab[i] = 0;
	}

	for (size_t i = 0; i < MAXTAB; i++)
	{
		env->reprtab[i] = 0;
	}
}

void env_clear_error_string(environment *const env)
{
	env->position = 0;
}

void env_add_comment(environment *const env)
{
	comment cmt = cmt_create(env->curent_path, env->line);

	char buffer[MAX_CMT_SIZE];
	cmt_to_string(&cmt, buffer);

	uni_printf(env->output, "%s", buffer);
}

size_t env_skip_str(environment *const env)
{
	const size_t position = strlen(env->error_string);
	while (env->curchar != '\n' && env->curchar != EOF)
	{
		m_nextch(env);
	}
	return position;
}

int get_next_char(environment *const env)
{
	env->nextchar = uni_scan_char(env->input);
	return env->nextchar == U'\r' ? get_next_char(env) : env->nextchar;
}

int env_get_depth_io(environment *const env)
{
	return env->depth_io;
}

int env_get_io_type(environment *const env)
{
	return env->curent_io_type;
}

size_t env_get_io_size(environment *const env, int type)
{
	if(env == NULL || type >= all_types || type <= file_type)
	{
		return -1;
	}
	return env->local_io_size[type];
}

int env_get_define_stack_prt(environment *const env)
{
	if(env == NULL)
	{
		return -1;
	}
	return env->define_stack_prt;
}

int env_clear_param(environment *const env, size_t size)
{
	env->local_io_size[param_type] = size;
}

int env_clear_param_define_stack(environment *const env)
{
	env->local_io_size[param_type] = env->define_stack[env->define_stack_prt];
}

int env_set_define_stack_prt(environment *const env, int num)
{
	env->define_stack_prt += num;
}

int env_set_define_stack_add(environment *const env, int num)
{
	env->define_stack[env->define_stack_prt + num] = env->local_io_size[param_type];
}

int env_io_add_char(environment *const env, int type, int simbol)
{
	env->local_io[type][env->local_io_size[type]++] = simbol;
}

int env_macro_ident_end(environment *const env)
{
	int *macro_io = env->local_io[macro_text_type];
	while (macro_io[env->local_io_size[macro_text_type] - 1] == ' ' 
			|| macro_io[env->local_io_size[macro_text_type] - 1] == '\t')
	{
		env->local_io_size[macro_text_type]--;
	}
	macro_io[env->local_io_size[macro_text_type]++] = MACRO_END;
}

int env_io_switch_to_new_type(environment *const env, int type, int prt)//new
{
	if(env->curent_io_type != file_type && prt < env->local_io_size[type])
	{
		return -1;
	}

	if(env->curent_io_type == file_type)
	{
		env->old_nextchar = env->nextchar;
	}

	env->old_io_type[env->depth_io] = env->curent_io_type;
	env->old_io[env->depth_io] = env->curent_io;

	env->depth_io++;
	env->curent_io_type = type;
	env->curent_io_prt = 0;

	if(type != file_type)
	{
		env->curent_io = &env->local_io[type][prt];
		env->nextchar = env->curent_io[env->curent_io_prt++];
	}
	else
	{
		env->curent_io = NULL;
	}

	return 0;
}

void env_io_switch_to_old_type(environment *const env)
{
	env->depth_io--;
	env->curent_io_type = env->old_io_type[env->depth_io];
	env->curent_io = env->old_io[env->depth_io];
	env->curent_io_prt = 0;

	if(env->curent_io_type != file_type)
	{
		env->nextchar = env->curent_io[1];
	}
	else
	{
		env->nextchar = env->old_nextchar;
	}
}

void m_onemore(environment *const env)
{
	env->curchar = env->nextchar;

	get_next_char(env);
	

	if (env->curchar == EOF)
	{
		env->nextchar = EOF;
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
				env->line++;
			}

			if (env->curchar == EOF)
			{
				env_error(env, comm_not_ended);
				env->line++;
				return;
			}
		} while (env->curchar != '*' || env->nextchar != '/');

		m_onemore(env);
		// m_fprintf_com();
		env->curchar = ' ';
	}
}

void m_nextch(environment *const env)
{
	if (env->curent_io_type != file_type)
	{
		env->curchar = env->nextchar;
		env->nextchar = env->curent_io[env->curent_io_prt++];

		if(env->curent_io_type == macro_text_type)
		{
			if (env->curchar == '\n')
			{
				env_add_comment(env);
			}
			else if (env->curchar == MACRO_CANGE)
			{
				m_nextch(env);
				env_io_switch_to_new_type(env, param_type, env->define_stack[env->curchar + env->define_stack_prt]);
				m_nextch(env);
			}
			else if (env->curchar == MACRO_END)
			{
				env_io_switch_to_old_type(env);
			}
		}
		else if (env->curent_io_type == param_type && env->curchar == END_PARAMETER)
		{
			env_io_switch_to_old_type(env);
			m_nextch(env);
		}
		else if(env->nextchar == '\0')
		{
			env_io_switch_to_old_type(env);
		}
	}
	else
	{
		m_onemore(env);
		m_coment_skip(env);

		if (env->curchar != '\n' && env->curchar != EOF)
		{
			env->position += utf8_to_string(&env->error_string[env->position], env->curchar);
		}
		else
		{
			env->line++;
			env_clear_error_string(env);
		}
	}
	
	// printf("t = %d curchar = %c, %i nextchar = %c, %i\n", env->nextch_type,
	// env->curchar, env->curchar, env->nextchar, env->nextchar);
}

void env_error(environment *const env, const int num)
{
	const size_t position = env_skip_str(env);
	macro_error(num, env->curent_path, env->error_string, env->line, position);
}
