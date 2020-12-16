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
		calculator(1, context);
		return context->cstring[0];
	}
	else
	{
		context->msp = 0;
		if (collect_mident(context))
		{
			flag = 1;
		}

		space_end_line(context);
		if(context->error_in_string)
		{
			return 0;
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

void if_end(preprocess_context *context)
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
					m_error(before_endif, context);
				}
				return;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				checkif++;
				if_end(context);
				if(context->error_in_string)
				{
					return;
				}
			}
		}
		else
		{
			m_nextch(context);
		}
	}

	m_error(must_be_endif, context);
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
				if_end(context);
				if(context->error_in_string)
				{
					return 1;
				}
			}
		}
		else
		{
			m_nextch(context);
		}
	}

	m_error(must_be_endif, context);
	return 1;
}

void if_true(int type_if, preprocess_context *context)
{
	while (context->curchar != EOF)
	{
		preprocess_scan(context);

		if (context->cur == SH_ELSE || context->cur == SH_ELIF)
		{
			break;
		}

		if (context->cur == SH_ENDIF)
		{
			checkif--;
			if (checkif < 0)
			{
				m_error(before_endif, context);
			}

			return;
		}
	}

	if (type_if != SH_IF && context->cur == SH_ELIF)
	{
		m_error(dont_elif, context);
		checkif--;
		return;
	}

	if_end(context);
}

void if_relis(preprocess_context *context)
{
	int type_if = context->cur;
	int flag = if_check(type_if, context); // начало (if)
	if(context->error_in_string)
	{
		checkif--;
		return;
	}

	checkif++;
	if (flag)
	{
		if_true(type_if, context);
		return;
	}
	else
	{
		context->cur = if_false(context);
		if(context->error_in_string)
		{
			checkif--;
			return;
		}
	}

	if (type_if == SH_IF)
	{
		while (context->cur == SH_ELIF)
		{
			flag = if_check(type_if, context);
			space_end_line(context);
			if(context->error_in_string)
			{
				checkif--;
				return;
			}

			if (flag)
			{
				if_true(type_if, context);
				return;
			}
			else
			{
				context->cur = if_false(context);
				if(context->error_in_string)
				{
					checkif--;
					return;
				}
			}
		}
	}
	else if (context->cur == SH_ELIF)
	{
		m_error(10, context);
		checkif--;
		return;
	}

	if (context->cur == SH_ELSE)
	{
		context->cur = 0;
		if_true(type_if, context);

		return;
	}

	if (context->cur == SH_ENDIF)
	{
		checkif--;
		if (checkif < 0)
		{
			m_error(before_endif, context);
		}
	}
}
