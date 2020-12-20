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
#include "context_var.h"
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


int if_check(int type_if, preprocess_context *context)
{
	int flag = 0;

	if (type_if == SH_IF)
	{
		if(calculator(1, context))
		{
			return -1;
		}
		return context->cstring[0];
	}
	else
	{
		context->msp = 0;
		if (collect_mident(context))
		{
			flag = 1;
		}

		if(space_end_line(context))
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

int if_end(preprocess_context *context)
{
	int fl_cur;

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			fl_cur = macro_keywords(context);
			if (fl_cur == SH_ENDIF)
			{
				checkif--;
				if (checkif < 0)
				{
					size_t position = skip_str(context); 
					macro_error(before_endif, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
					return -1;
				}
				return 0;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				checkif++;
				if(if_end(context))
				{
					return -1;
				}
			}
		}
		else
		{
			m_nextch(context);
		}
	}

	size_t position = skip_str(context); 
	macro_error(must_be_endif, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
	return -1;
}

int if_false(preprocess_context *context)
{
	int fl_cur = context->cur;

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			fl_cur = macro_keywords(context);
			m_nextch(context);

			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				if(if_end(context))
				{
					return 0;
				}
			}
		}
		else
		{
			m_nextch(context);
		}
	}

	size_t position = skip_str(context); 
	macro_error(must_be_endif, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
	return 0;
}

int if_true(int type_if, preprocess_context *context)
{
	int error = 0;
	while (context->curchar != EOF)
	{
		error = preprocess_scan(context);
		if(error)
		{
			return error;
		}

		if (context->cur == SH_ELSE || context->cur == SH_ELIF)
		{
			break;
		}

		if (context->cur == SH_ENDIF)
		{
			checkif--;
			if (checkif < 0)
			{
				size_t position = skip_str(context); 
				macro_error(before_endif, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
				return -1;
			}

			return 0;
		}
	}

	if (type_if != SH_IF && context->cur == SH_ELIF)
	{
		size_t position = skip_str(context); 
		macro_error(dont_elif, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
		checkif--;
		return -1;
	}

	return if_end(context);
}

int if_relis(preprocess_context *context)
{
	int type_if = context->cur;
	int flag = if_check(type_if, context); // начало (if)
	if(flag == -1)
	{
		checkif--;
		return -1;
	}

	checkif++;
	if (flag)
	{
		return if_true(type_if, context);
	}
	else
	{
		int rez = if_false(context);
		if(!rez)
		{
			checkif--;
			return -1;
		}
		context->cur = rez;
	}

	
	while (context->cur == SH_ELIF)
	{
		flag = if_check(type_if, context);
		if(flag == -1 || space_end_line(context))
		{
			checkif--;
			return -1;
		}

		if (flag)
		{
			return if_true(type_if, context);
		}
		else
		{
			int rez = if_false(context);
			if(!rez)
			{
				checkif--;
				return -1;
			}
			context->cur = rez;
		}
	}
	

	if (context->cur == SH_ELSE)
	{
		context->cur = 0;
		return if_true(type_if, context);;
	}

	if (context->cur == SH_ENDIF)
	{
		checkif--;
		if (checkif < 0)
		{
			size_t position = skip_str(context); 
			macro_error(before_endif, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
			return -1;
		}
	}
	return 0;
}
