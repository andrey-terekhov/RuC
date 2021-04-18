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
#include <stdint.h>
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
	env->input = NULL;

	env->curent_io_type = file_type;
	env->curent_io = NULL;
	env->depth_io = 0;
	env->curent_io_prt = 0;
	env->macro_stack_prt = 0;

	env->macro_tab[0] = '\0';
	env->args[0] = '\0';
	env->macro_stack[0] = '\0';

	env->local_io[macro_text_type] = env->macro_tab;
	env->local_io[param_type] = env->args;
	env->local_io[define_stack_type] = env->macro_stack;
	env->local_io[if_type] = NULL;
	env->local_io[while_type] = NULL;

	env->curchar = '\0';
	env->nextchar = '\0';
	env->cur = 0;
	
	env->error_string[0] = '\0';
	env->error_string_size = 0;

	env->curent_path  = NULL;
	env->old_nextchar = '\0';
	env->old_curchar = '\0';

	env->line = 1;
	env->nested_if = 0;
	env->rp = 1;

	for (size_t i = 0; i < all_types; i++)
	{
		env->local_io_size[i] = 0;
	}
	env->local_io_size[macro_text_type] = 1;
	env->local_io_size[if_type] = 1;

	for (size_t i = 0; i < DEPTH_IO_SISE; i++)
	{
		env->old_io_type[i] = all_types;
		env->old_io[i] = NULL;
	}


	for (size_t i = 0; i < HASH; i++)
	{
		env->hashtab[i] = 0;
	}

	for (size_t i = 0; i < MAXTAB; i++)
	{
		env->reprtab[i] = 0;
	}
}

int env_io_set_char(environment *const env, int type, size_t prt, char32_t simbol)
{
	if (env == NULL || type < macro_text_type || type >= all_types || env->local_io[type] == NULL)
	{
		return -1;
	}
	env->local_io[type][prt] = simbol;
	return 0;
}

char32_t env_io_get_char(environment *const env, int type, size_t prt)
{
	if (env == NULL || type < macro_text_type || type >= all_types
		|| env->local_io[type] == NULL || prt > env->local_io_size[type])
	{
		return EOF;
	}
	return env->local_io[type][prt];
}

char32_t env_get_curchar(environment *const env)
{
	if(env != NULL)
	{
		return env->curchar;
	}
	else
	{
		return EOF;
	}
}

char32_t env_get_nextchar(environment *const env)
{
	if(env != NULL)
	{
		return env->nextchar;
	}
	else
	{
		return EOF;
	}
}

int env_add_comment(environment *const env)
{
	if (env == NULL || env->curent_path == NULL)
	{
		return -1;
	}
	comment cmt = cmt_create(env->curent_path, env->line);

	char buffer[MAX_CMT_SIZE];
	cmt_to_string(&cmt, buffer);

	uni_printf(env->output, "%s", buffer);
	return 0;
}

void env_clear_error_string(environment *const env)
{
	env->error_string_size = 0;
}

size_t env_skip_str(environment *const env)
{
	const size_t position = strlen(env->error_string);
	while (env->curchar != '\n' && env->curchar != (char32_t)EOF)
	{
		env_scan_next_char(env);
	}
	return position;
}

char32_t get_next_char(environment *const env)
{
	env->nextchar = uni_scan_char(env->input);
	return env->nextchar == U'\r' ? get_next_char(env) : env->nextchar;
}

size_t env_io_get_depth(environment *const env)//
{
	if (env == NULL)
	{
		return SIZE_MAX;
	}
	return env->depth_io;
}

int env_io_get_type(environment *const env)
{
	if (env == NULL)
	{
		return all_types;
	}
	return env->curent_io_type;
}

size_t env_io_get_size(environment *const env, int type)
{
	if (env == NULL || type >= all_types || type <= file_type)
	{
		return -1;
	}
	return env->local_io_size[type];
}

size_t env_io_get_prt(environment *const env)
{
	if (env == NULL)
	{
		return SIZE_MAX;
	}
	return env->curent_io_prt;
}

size_t env_get_macro_stack_prt(environment *const env)
{
	if (env == NULL)
	{
		return SIZE_MAX;
	}
	return env->macro_stack_prt;
}

int env_move_macro_stack_prt(environment *const env, int num)
{
	if (env == NULL || (int)env->macro_stack_prt + num < 0)
	{
		return -1;
	}
	env->macro_stack_prt += num;
	return 0;
}

int env_clear_param(environment *const env, size_t size)
{
	if (env == NULL)
	{
		return -1;
	}
	env->local_io_size[param_type] = size;
	return 0;
}

int env_io_add_char(environment *const env, int type, char32_t simbol)
{
	if (env == NULL || type >= all_types || type <= file_type)
	{
		return -1;
	}
	env->local_io[type][env->local_io_size[type]++] = simbol;
	return 0;
}

int env_set_file_input(environment *const env, universal_io *input, const char* path)
{
	if (env == NULL || input == NULL)
	{
		return -1;
	}
	env->input = input;
	env->curchar = '\0';
	env->nextchar = '\0';
	env->curent_path = path;
	env_clear_error_string(env);
	return 0;
}

universal_io *env_get_file_input(environment *const env)
{
	if (env == NULL)
	{
		return NULL;
	}
	return env->input;
}

universal_io *env_get_file_output(environment *const env)
{
	if (env == NULL)
	{
		return NULL;
	}
	return env->output;
}

void env_curchar_chec(environment *const env)
{
	if (env->curent_io_type == macro_text_type  && env->curchar == '\n')
	{
		env_add_comment(env);
	}
	else if (env->curent_io_type == macro_text_type  && env->curchar == MACRO_CANGE)
	{
		size_t num_param = (size_t) env->nextchar;
		env->curent_io_prt += 2;
		env_io_switch_to_new(env, param_type, env->macro_stack[num_param + env->macro_stack_prt]);
	}
	else if (env->curchar == '\0')
	{
		env_io_back_to_previous(env);
	}
}

int env_io_clear(environment *const env, int type, size_t size)
{
	if (env == NULL || type <= macro_text_type || type >= all_types)
	{
		return -1;
	}

	if (env->local_io_size[type] > size)
	{
		env->local_io_size[type] = size;
	}
	return 0;
}

int env_io_set(environment *const env, char32_t *buffer, int type)
{
	if (env == NULL || (type != if_type && type != while_type))
	{
		return -1;
	}

	env->local_io[type] = buffer;
	return 0;
}

int env_io_switch_to_new(environment *const env, int type, size_t prt)
{
	if (env == NULL || (type != file_type && (type >= all_types || type <= file_type ||
		env->local_io[type] == NULL || ( type != param_type && prt >= env->local_io_size[type]))))
	{
		return -1;
	}

	if (env->curent_io_type == file_type)
	{
		env->old_curchar = env->curchar;
		env->old_nextchar = env->nextchar;
	}

	env->old_io_type[env->depth_io] = env->curent_io_type;
	env->old_io[env->depth_io] = &env->curent_io[env->curent_io_prt-2];

	env->depth_io++;
	env->curent_io_type = type;
	env->curent_io_prt = 0;

	if (type != file_type)
	{
		env->curent_io = &env->local_io[type][prt];
		env->curchar = env->curent_io[env->curent_io_prt++];
		env->nextchar = env->curent_io[env->curent_io_prt++];
		env_curchar_chec(env);
	}
	else
	{
		env->curent_io = NULL;
	}
	//printf("new t = %d curchar = %c, %i nextchar = %c, %i; prt = %d\n", env->curent_io_type,
	//env->curchar, env->curchar, env->nextchar, env->nextchar, env->curent_io_prt);

	return 0;
}

int env_io_back_to_previous(environment *const env)
{
	if (env == NULL)
	{
		return -1;
	}
	env->depth_io--;
	env->curent_io_type = env->old_io_type[env->depth_io];
	env->curent_io = env->old_io[env->depth_io];
	env->curent_io_prt = 0;

	if (env->curent_io_type != file_type)
	{
		env->curchar = env->curent_io[env->curent_io_prt++];
		env->nextchar = env->curent_io[env->curent_io_prt++];
		env_curchar_chec(env);
	}
	else
	{
		env->curchar = env->old_curchar;
		env->nextchar = env->old_nextchar;
	}
	return 0;

	//printf("old t = %d curchar = %c, %i nextchar = %c, %i; prt = %d\n", env->curent_io_type,
	//env->curchar, env->curchar, env->nextchar, env->nextchar, env->curent_io_prt);
}

void m_onemore(environment *const env)
{
	env->curchar = env->nextchar;
	get_next_char(env);

	if (env->curchar == (char32_t)EOF)
	{
		env->nextchar = (char32_t)EOF;
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

			if (env->curchar == (char32_t)EOF)
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

			if (env->curchar == (char32_t)EOF)
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

void env_scan_next_char(environment *const env)
{
	env->curchar = env->nextchar;

	if (env->curent_io_type != file_type)
	{
		env->nextchar = env->curent_io[env->curent_io_prt++];
		env_curchar_chec(env);
		//printf("t = %d curchar = %c, %i nextchar = %c, %i; prt = %d\n", env->curent_io_type,
		//env->curchar, env->curchar, env->nextchar, env->nextchar, env->curent_io_prt);

		return;
	}

	get_next_char(env);

	m_coment_skip(env);
	if (env->curchar == (char32_t)EOF)
	{
		env->nextchar = (char32_t)EOF;
	}

	if (env->curchar != '\n' && env->curchar != (char32_t)EOF)
	{
		env->error_string_size += utf8_to_string(&env->error_string[env->error_string_size], env->curchar);
	}
	else
	{
		env->line++;
		env_clear_error_string(env);
	}
	
	//printf("t = %d curchar = %c, %i nextchar = %c, %i; prt = %ld\n", env->curent_io_type,
	//env->curchar, env->curchar, env->nextchar, env->nextchar, env->curent_io_prt);
}

int env_error(environment *const env, const int num)
{
	if (env == NULL)
	{
		return -1;
	}
	const size_t position = env_skip_str(env);
	macro_error(num, env->curent_path, env->error_string, env->line, position);
	return 0;
}
