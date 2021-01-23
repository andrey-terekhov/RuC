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

#include "if.h"
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "file.h"
#include "preprocessor.h"
#include "error.h"
#include "utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int checkif = 0;


int if_check(int type_if, environment *const env)
{
	int flag = 0;

	if (type_if == SH_IF)
	{
		if(calculator(1, env))
		{
			return -1;
		}
		return env->cstring[0];
	}
	else
	{
		if (collect_mident(env))
		{
			flag = 1;
		}

		if(space_end_line(env))
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

	while (env->curchar != EOF)
	{
		if (env->curchar == '#')
		{
			fl_cur = macro_keywords(env);
			if (fl_cur == SH_ENDIF)
			{
				checkif--;
				if (checkif < 0)
				{
					size_t position = skip_str(env); 
					macro_error(before_endif, ws_get_file(env->lk.ws, env->lk.current),  env->error_string, env->line, position);
					return -1;
				}
				return 0;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				checkif++;
				if(if_end(env))
				{
					return -1;
				}
			}
		}
		else
		{
			m_nextch(env);
		}
	}

	size_t position = skip_str(env); 
	macro_error(must_be_endif, ws_get_file(env->lk.ws, env->lk.current),  env->error_string, env->line, position);
	return -1;
}

int if_false(environment *const env)
{
	int fl_cur = env->cur;

	while (env->curchar != EOF)
	{
		if (env->curchar == '#')
		{
			fl_cur = macro_keywords(env);
			m_nextch(env);

			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				if(if_end(env))
				{
					return 0;
				}
			}
		}
		else
		{
			m_nextch(env);
		}
	}

	size_t position = skip_str(env); 
	macro_error(must_be_endif, ws_get_file(env->lk.ws, env->lk.current),  env->error_string, env->line, position);
	return 0;
}

int if_true(int type_if, environment *const env)
{
	int error = 0;
	while (env->curchar != EOF)
	{
		error = preprocess_scan(env);
		if(error)
		{
			return error;
		}

		if (env->cur == SH_ELSE || env->cur == SH_ELIF)
		{
			break;
		}

		if (env->cur == SH_ENDIF)
		{
			checkif--;
			if (checkif < 0)
			{
				size_t position = skip_str(env); 
				macro_error(before_endif, ws_get_file(env->lk.ws, env->lk.current),  env->error_string, env->line, position);
				return -1;
			}

			return 0;
		}
	}

	if (type_if != SH_IF && env->cur == SH_ELIF)
	{
		size_t position = skip_str(env); 
		macro_error(dont_elif, ws_get_file(env->lk.ws, env->lk.current),  env->error_string, env->line, position);
		checkif--;
		return -1;
	}

	return if_end(env);
}

int if_relis(environment *const env)
{
	int type_if = env->cur;
	int flag = if_check(type_if, env); // начало (if)
	if(flag == -1)
	{
		checkif--;
		return -1;
	}

	checkif++;
	if (flag)
	{
		return if_true(type_if, env);
	}
	else
	{
		int res = if_false(env);
		if(!res)
		{
			checkif--;
			return -1;
		}
		env->cur = res;
	}

	
	while (env->cur == SH_ELIF)
	{
		flag = if_check(type_if, env);
		if(flag == -1 || space_end_line(env))
		{
			checkif--;
			return -1;
		}

		if (flag)
		{
			return if_true(type_if, env);
		}
		else
		{
			int res = if_false(env);
			if(!res)
			{
				checkif--;
				return -1;
			}
			env->cur = res;
		}
	}
	

	if (env->cur == SH_ELSE)
	{
		env->cur = 0;
		return if_true(type_if, env);;
	}

	if (env->cur == SH_ENDIF)
	{
		checkif--;
		if (checkif < 0)
		{
			size_t position = skip_str(env); 
			macro_error(before_endif, ws_get_file(env->lk.ws, env->lk.current),  env->error_string, env->line, position);
			return -1;
		}
	}
	return 0;
}
