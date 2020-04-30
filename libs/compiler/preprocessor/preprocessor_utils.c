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

#include "preprocessor_utils.h"
#include "constants.h"
#include "context.h"
#include "context_var.h"
#include "file.h"
#include "preprocessor_error.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int equal_reprtab(int i, int j, preprocess_context *context)
{
	++i;
	++j;

	while (context->reprtab[++i] == context->reprtab[++j])
	{
		if (context->reprtab[i] == 0 && context->reprtab[j] == 0)
		{
			return 1;
		}
	}

	return 0;
}

int is_letter(preprocess_context *context)
{
	return (context->curchar >= 'A' && context->curchar <= 'Z') ||
		   (context->curchar >= 'a' && context->curchar <= 'z') || context->curchar == '_' ||
		   (context->curchar >= 0x410 /*'А'*/ && context->curchar <= 0x44F /*'я'*/);
}

int is_digit(preprocess_context *context)
{
	return context->curchar >= '0' && context->curchar <= '9';
}

int macro_keywords(preprocess_context *context, compiler_context *c_context)
{
	int oldrepr = context->rp;
	int r = 0;
	int n = 0;

	context->rp += 2;
	int hash = 0;
	do
	{
		hash += context->curchar;
		context->reprtab[context->rp++] = context->curchar;
		n++;
		m_nextch(context, c_context);
	} while (is_letter(context) || is_digit(context));

	if (context->curchar != '\n' && context->curchar != ' ' && context->curchar != '\t' && context->curchar != '(' &&
		context->curchar != '\"')
	{
		m_error(after_ident_must_be_space, c_context);
	}

	hash &= 255;
	context->reprtab[context->rp++] = 0;
	r = context->hashtab[hash];

	if (r)
	{
		do
		{
			if (equal_reprtab(r, oldrepr, context))
			{
				context->rp = oldrepr;
				context->reprtab[context->rp] = n;
				return (context->reprtab[r + 1] < 0) ? context->reprtab[r + 1] : 0;
			}
			else
			{
				r = context->reprtab[r];
			}
		} while (r);
	}

	context->rp = oldrepr;
	context->reprtab[context->rp] = n;
	return 0;
}

int mf_equal(int i, preprocess_context *context)
{
	int j = 0;
	i += 2;

	while (context->reprtab[i++] == context->mstring[j++])
	{
		if (context->reprtab[i] == 0 && context->mstring[j] == MACROEND)
		{
			return 1;
		}
	}

	return 0;
}

int collect_mident(preprocess_context *context, compiler_context *c_context)
{
	int r;
	int hash = 0;
	context->msp = 0;

	while (is_letter(context) || is_digit(context))
	{
		context->mstring[context->msp++] = context->curchar;
		hash += context->curchar;
		m_nextch(context, c_context);
	}

	context->mstring[context->msp] = MACROEND;
	hash &= 255;
	r = context->hashtab[hash];

	while (r)
	{
		if (r >= context->mfirstrp && r <= context->mlastrp && mf_equal(r, context))
		{
			return (context->macrotext[context->reprtab[r + 1]] != MACROUNDEF) ? r : 0;
		}

		r = context->reprtab[r];
	}

	return 0;
}

void space_end_line(preprocess_context *context, compiler_context *c_context)
{
	while (context->curchar != '\n')
	{
		if (context->curchar == ' ' || context->curchar == '\t')
		{
			m_nextch(context, c_context);
		}
		else
		{
			m_error(after_preproces_words_must_be_space, c_context);
		}
	}
	m_nextch(context, c_context);
}

void space_skip(preprocess_context *context, compiler_context *c_context)
{
	while (context->curchar == ' ' || context->curchar == '\t')
	{
		m_nextch(context, c_context);
	}
}

void space_skip_str(preprocess_context *context, compiler_context *c_context)
{
	int c = context->curchar;
	m_fprintf(context->curchar, context, c_context);
	m_nextch(context, c_context);

	while (context->curchar != c && context->curchar != EOF)
	{
		if (context->curchar == '\\')
		{
			m_fprintf(context->curchar, context, c_context);
			m_nextch(context, c_context);
		}

		m_fprintf(context->curchar, context, c_context);
		m_nextch(context, c_context);
	}

	m_fprintf(context->curchar, context, c_context);
	m_nextch(context, c_context);
}
