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
#include "context.h"
#include "context_var.h"
#include "file.h"
#include "preprocess.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int checkif = 0;


int if_check(int type_if, preprocess_context *context, compiler_context *c_context)
{
	int flag = 0;

	if (type_if == SH_IF)
	{
		calculator(1, context, c_context);
		return context->cstring[0];
	}
	else
	{
		context->msp = 0;
		if (collect_mident(context, c_context))
		{
			flag = 1;
		}

		space_end_line(context, c_context);

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

void if_end(preprocess_context *context, compiler_context *c_context)
{
	int fl_cur;

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			fl_cur = macro_keywords(context, c_context);
			m_nextch(context, c_context);

			if (fl_cur == SH_ENDIF)
			{
				checkif--;
				if (checkif < 0)
				{
					m_error(befor_endif, c_context);
				}
				return;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				checkif++;
				if_end(context, c_context);
			}
		}
		else
		{
			m_nextch(context, c_context);
		}
	}

	m_error(must_be_endif, c_context);
}

int if_false(preprocess_context *context, compiler_context *c_context)
{
	int fl_cur = context->cur;

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			fl_cur = macro_keywords(context, c_context);
			m_nextch(context, c_context);

			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				if_end(context, c_context);
			}
		}
		else
		{
			m_nextch(context, c_context);
		}
	}

	m_error(must_be_endif, c_context);
	return 1;
}

void if_true(int type_if, preprocess_context *context, compiler_context *c_context)
{
	while (context->curchar != EOF)
	{
		preprocess_scan(context, c_context);

		if (context->cur == SH_ELSE || context->cur == SH_ELIF)
		{
			break;
		}

		if (context->cur == SH_ENDIF)
		{
			checkif--;
			if (checkif < 0)
			{
				m_error(befor_endif, c_context);
			}

			return;
		}
	}

	if (type_if != SH_IF && context->cur == SH_ELIF)
	{
		m_error(dont_elif, c_context);
	}

	if_end(context, c_context);
}

void if_relis(preprocess_context *context, compiler_context *c_context)
{
	int type_if = context->cur;
	int flag = if_check(type_if, context, c_context); // начало (if)

	checkif++;
	if (flag)
	{
		if_true(type_if, context, c_context);
		return;
	}
	else
	{
		context->cur = if_false(context, c_context);
	}

	if (type_if == SH_IF)
	{
		while (context->cur == SH_ELIF)
		{
			flag = if_check(type_if, context, c_context);
			space_end_line(context, c_context);

			if (flag)
			{
				if_true(type_if, context, c_context);
				return;
			}
			else
			{
				context->cur = if_false(context, c_context);
			}
		}
	}
	else if (context->cur == SH_ELIF)
	{
		a_erorr(7);
	}

	if (context->cur == SH_ELSE)
	{
		context->cur = 0;
		if_true(type_if, context, c_context);

		return;
	}

	if (context->cur == SH_ENDIF)
	{
		checkif--;
		if (checkif < 0)
		{
			m_error(befor_endif, c_context);
		}
	}
}
