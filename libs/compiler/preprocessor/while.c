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

#include "while.h"
#include "calculator.h"
#include "constants.h"
#include "context.h"
#include "context_var.h"
#include "file.h"
#include "preprocessor.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
			context->cur = macro_keywords(context);

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
				int i = 0;

				for (i = 0; i < context->reprtab[context->rp]; i++)
				{
					context->wstring[context->wsp++] = context->reprtab[context->rp + 2 + i];
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
