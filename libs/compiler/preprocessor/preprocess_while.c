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

#include "preprocess_while.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocess_global.h"
#include "preprocess_defs.h"
#include "preprocess_nextch.h"
#include "preprocess.h"
#include "preprocess_utils.h"
#include "preprocess_error.h" 
#include "preprocess_calculator.h"


void while_collect(preprocess_context *context, compiler_context *c_context)
{
	int oldwsp = context->wsp;

	context->wstring[context->wsp++] = WHILEBEGIN;
	context->wstring[context->wsp++] = context->ifsp;
	context->wsp++;

	while (context->curchar != '\n')
	{
		context->ifstring[context->ifsp++] = context->curchar;
		m_nextch(context, c_context);
	}
	context->ifstring[context->ifsp++] = '\n';
	m_nextch(context, c_context);

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			context->cur = macro_keywords(context, c_context);

			if (context->cur == SH_WHILE)
			{
				while_collect(context, c_context);
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
		m_nextch(context, c_context);
	}
	a_erorr(8);
}

void while_relis(preprocess_context *context, compiler_context *c_context)
{
	int oldernextp =context-> nextp;
	int end = context->wstring[oldernextp + 2];

	context->cur = 0;
	while (context->wstring[oldernextp] == WHILEBEGIN)
	{
		m_nextch(context, c_context);
		m_change_nextch_type(IFTYPE, context->wstring[context->nextp],context, c_context);
		calculator(1,context, c_context);
		m_old_nextch_type(context);


		if (context->cstring[0] == 0)
		{
			context->nextp = end;
			m_nextch(context, c_context);
			return;
		}

		m_nextch(context, c_context);
		m_nextch(context, c_context);
		m_nextch(context, c_context);
		space_skip(context, c_context);

		while (context->nextp != end || context->nextch_type != WHILETYPE)
		{
			if (context->curchar == WHILEBEGIN)
			{
				context->nextp--;
				while_relis(context, c_context);
			}
			else if (context->curchar == EOF)
			{
				a_erorr(12);
			}
			else
			{
				preprocess_scan(context, c_context);
			}
		}
		context->nextp = oldernextp;
	}
}