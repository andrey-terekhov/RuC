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
#include "calculator.h"
#include "constants.h"
#include "context_var.h"
#include "macros_get.h"
#include "macros_seve.h"
#include "file.h"
#include "include.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void while_collect(preprocess_context *context)
{
	int oldwsp = context->wsp;

	context->wstring[context->wsp++] = WHILEBEGIN;
	context->wstring[context->wsp++] = context->ifsp;
	context->wsp++;

	while (context->curchar != '\n')
	{
		context->ifstring[context->ifsp++] = context->curchar;
		m_nextch(context);
	}
	context->ifstring[context->ifsp++] = '\n';
	m_nextch(context);

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			context->cur = con_repr_find(&context->repr, str);

			if (context->cur == SH_WHILE)
			{
				while_collect(context);
			}
			else if (context->cur == SH_ENDW)
			{
				context->wstring[oldwsp + 2] = context->wsp;
				context->cur = 0;

				return;
			}
			else
			{
				int i = 1;
				while (str[i] != '\0')
				{
					context->wstring[context->wsp++] = str[i++];
				}
			}
		}
		context->wstring[context->wsp++] = context->curchar;
		m_nextch(context);
	}
	m_error(40, context);
}

void while_relis(preprocess_context *context)
{
	int oldernextp = context->nextp;
	int end = context->wstring[oldernextp + 2];

	context->cur = 0;
	while (context->wstring[oldernextp] == WHILEBEGIN)
	{
		m_nextch(context);
		m_change_nextch_type(IFTYPE, context->wstring[context->nextp], context);
		m_nextch(context);
		calculator(1, context);
		m_old_nextch_type(context);


		if (context->cstring[0] == 0)
		{
			context->nextp = end;
			m_nextch(context);
			return;
		}

		m_nextch(context);
		m_nextch(context);
		m_nextch(context);
		space_skip(context);

		while (context->nextp != end || context->nextch_type != WHILETYPE)
		{
			if (context->curchar == WHILEBEGIN)
			{
				context->nextp--;
				while_relis(context);
			}
			else if (context->curchar == EOF)
			{
				m_error(41, context);
			}
			else
			{
				preprocess_scan(context);
			}
		}
		context->nextp = oldernextp;
	}
}

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
		char32_t str[STRIGSIZE];
		collect_mident(context, str);
		
		if (con_repr_find(&context->repr, str) != 0)
		{
			flag = 1;
		}

		space_end_line(context);

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
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			fl_cur = con_repr_find(&context->repr, str);
			m_nextch(context);

			if (fl_cur == SH_ENDIF)
			{
				context->checkif--;
				if (context->checkif < 0)
				{
					m_error(before_endif, context);
				}
				return;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				context->checkif++;
				if_end(context);
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
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			fl_cur = con_repr_find(&context->repr, str);
			m_nextch(context);

			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}

			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				if_end(context);
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
			context->checkif--;
			if (context->checkif < 0)
			{
				m_error(before_endif, context);
			}

			return;
		}
	}

	if (type_if != SH_IF && context->cur == SH_ELIF)
	{
		m_error(dont_elif, context);
	}

	if_end(context);
}

void if_relis(preprocess_context *context)
{
	int type_if = context->cur;
	int flag = if_check(type_if, context); // начало (if)

	context->checkif++;
	if (flag != 0)
	{
		if_true(type_if, context);
		return;
	}
	else
	{
		context->cur = if_false(context);
	}

	if (type_if == SH_IF)
	{
		while (context->cur == SH_ELIF)
		{
			flag = if_check(type_if, context);
			space_end_line(context);

			if (flag != 0)
			{
				if_true(type_if, context);
				return;
			}
			else
			{
				context->cur = if_false(context);
			}
		}
	}
	else if (context->cur == SH_ELIF)
	{
		m_error(10, context);
	}

	if (context->cur == SH_ELSE)
	{
		context->cur = 0;
		if_true(type_if, context);

		return;
	}

	if (context->cur == SH_ENDIF)
	{
		context->checkif--;
		if (context->checkif < 0)
		{
			m_error(before_endif, context);
		}
	}
}



void preprocess_words(preprocess_context *context)
{
	/*if (context->curchar != '(')
	{
		m_nextch(context);
	}*/
	space_skip(context);
	switch (context->cur)
	{
		case SH_INCLUDE:
		{
			include_relis(context);
			return;
		}
		case SH_DEFINE:
		case SH_MACRO:
		{
			define_relis(context);
			return;
		}
		case SH_UNDEF:
		{
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			int k = con_repr_find(&context->repr, str);
			if(k)
			{
				context->macrotext[k]= MACROUNDEF;
			}
			space_end_line(context);
			return;
		}
		case SH_IF:
		case SH_IFDEF:
		case SH_IFNDEF:
		{
			if_relis(context);
			return;
		}
		case SH_SET:
		{
			set_relis(context);
			return;
		}
		case SH_ELSE:
		case SH_ELIF:
		case SH_ENDIF:
			return;
		case SH_EVAL:
		{
			if (context->curchar == '(')
			{
				calculator(0, context);
			}
			else
			{
				m_error(after_eval_must_be_ckob, context);
			}

			m_change_nextch_type(CTYPE, 0, context);
			return;
		}
		case SH_WHILE:
		{
			context->wsp = 0;
			context->ifsp = 0;
			while_collect(context);
			m_change_nextch_type(WHILETYPE, 0, context);
			m_nextch(context);
			m_nextch(context);

			context->nextp = 0;
			while_relis(context);

			return;
		}
		default:
		{
			return;//error
		}
	}
}

void preprocess_scan(preprocess_context *context)
{
	int i;

	switch (context->curchar)
	{
		case EOF:
			return;

		case '#':
		{
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			context->cur = con_repr_find(&context->repr, str);

			if (context->cur != 0)
			{
				context->prep_flag = 1;
				preprocess_words(context);
				if(context->curchar != '#')
				{
					con_file_print_coment(&context->fs, context);
				}
			}
			else
			{
				// m_nextch(context);
				int i = 1; 
				while (str[i] != '\0')
				{
					m_fprintf(str[i++], context);
				}
			}

			return;
		}
		case '\'':
		case '\"':
		{
			space_skip_str(context);
			return;
		}
		case '@':
		{
			m_nextch(context);
			return;
		}
		default:
		{
			if (is_letter(context) != 0 && context->prep_flag == 1)
			{
				char32_t str[STRIGSIZE];
				collect_mident(context, str);
				int r = con_repr_find(&context->repr, str);
				if (r != 0 && r <= MAXTAB)
				{
					
					define_get_from_macrotext(r, context);
				}
				else
				{
					int i = 1; 
					while (str[i] != '\0')
					{
						m_fprintf(str[i++], context);
					}
				}
			}
			else
			{
				m_fprintf(context->curchar, context);
				m_nextch(context);
			}
		}
	}
}