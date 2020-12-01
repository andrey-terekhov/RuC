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

#include "macros_get.h"
#include "calculator.h"
#include "constants.h"
#include "context_var.h"
#include "file.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void define_get_from_macrotext(int r, preprocess_context *context);


void function_scob_collect(int t, int num, preprocess_context *context)//
{
	int i;

	while (context->curchar != EOF)
	{
		if (is_letter(context) != 0)
		{
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			int r = con_repr_find(&context->repr, str);

			if (r != 0)
			{
				int oldcp1 = context->cp;
				int oldlsp = context->lsp;
				int locfchange[STRIGSIZE];
				int lcp = 0;
				int ldip;

				context->lsp += num;
				define_get_from_macrotext(r, context);
				ldip = get_dipp(context);

				if (context->nextch_type == FTYPE)
				{
					ldip--;
				}

				while (get_dipp(context) >= ldip) // 1 переход потому что есть префиксная замена
				{
					locfchange[lcp++] = context->curchar;
					m_nextch(context);
				}

				context->lsp = oldlsp;
				context->cp = oldcp1;

				for (i = 0; i < lcp; i++)
				{
					context->fchange[context->cp++] = locfchange[i];
				}
			}
			else
			{
				int i = 1;
				while (str[i] != '\0')
				{
					context->fchange[context->cp++] = str[i++];
				}
			}
		}
		else if (context->curchar == '(')
		{
			context->fchange[context->cp++] = context->curchar;
			m_nextch(context);
			function_scob_collect(0, num, context);
		}
		else if (context->curchar == ')' || (t == 1 && context->curchar == ','))
		{
			if (t == 0)
			{
				context->fchange[context->cp++] = context->curchar;
				m_nextch(context);
			}

			return;
		}
		else if (context->curchar == '#')
		{
			char32_t str[STRIGSIZE];
			collect_mident(context, str);
			if (con_repr_find(&context->repr, str) == SH_EVAL && context->curchar == '(')
			{
				calculator(0, context);
				for (i = 0; i < context->csp; i++)
				{
					context->fchange[context->cp++] = context->cstring[i];
				}
			}
			else
			{
				int i = 1; 
				while (str[i] != '\0')
				{
					context->fchange[context->cp++] = str[i++];
				}
			}
		}
		else
		{
			context->fchange[context->cp++] = context->curchar;
			m_nextch(context);
		}
	}
	m_error(scob_not_clous, context);
}

void function_stack_create(int n, preprocess_context *context)//
{
	int num = 0;

	m_nextch(context);
	// printf("function_stack_create n = %d\n", n);
	context->localstack[num + context->lsp] = context->cp;

	if (context->curchar == ')')
	{
		m_error(stalpe, context);
	}

	while (context->curchar != ')')
	{
		function_scob_collect(1, num, context);
		context->fchange[context->cp++] = CANGEEND;

		if (context->curchar == ',')
		{
			num++;
			context->localstack[num + context->lsp] = context->cp;

			if (num > n)
			{
				m_error(not_enough_param, context);
			}
			m_nextch(context);

			if (context->curchar == ' ')
			{
				m_nextch(context);
			}
		}
		else if (context->curchar == ')')
		{
			if (num != n)
			{
				m_error(not_enough_param2, context);
			}
			m_nextch(context);

			context->cp = context->localstack[context->lsp];
			return;
		}
	}

	m_error(scob_not_clous, context);
}

void define_get_from_macrotext(int r, preprocess_context *context)//
{
	int t = r;

	if (r != 0)
	{
		
		if (context->macrotext[t] == MACROFUNCTION)
		{
			if (context->macrotext[++t] > -1)
			{
				function_stack_create(context->macrotext[t], context);
			}
		}

		// printf("--from_macrotext r = %d\n", t + 1);
		m_change_nextch_type(TEXTTYPE, t + 1, context);
		m_nextch(context);
	}
	else
	{
		m_error(ident_not_exist, context);
	}
}
