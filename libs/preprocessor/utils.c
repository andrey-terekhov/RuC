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

#include "utils.h"
#include "constants.h"
#include "context_var.h"
#include "file.h"
#include "error.h"
#include "utf8.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int equal_reprtab(int i, int j, preprocess_context *context)
{
	i += 2;
	j += 2;

	while (context->reprtab[i] == context->reprtab[j])
	{
		i++;
		j++;

		if (context->reprtab[i] == 0 && context->reprtab[j] == 0)
		{
			return 1;
		}
	}

	return 0;
}

void output_keywods(preprocess_context *context)
{
	for (int j = 0; j < context->reprtab[context->rp]; j++)
	{
		m_fprintf(context->reprtab[context->rp + 2 + j], context);
	}
}

int is_letter(preprocess_context *context)
{
	return (context->curchar >= 'A' && context->curchar <= 'Z') ||
		   (context->curchar >= 'a' && context->curchar <= 'z') || context->curchar == '_' ||
		   utf8_is_russian(context->curchar);
}

int is_digit(int a)
{
	return a >= '0' && a <= '9';
}

int macro_keywords(preprocess_context *context)
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
		m_nextch(context);
	} while (is_letter(context) || is_digit(context->curchar));

	/*if (context->curchar != '\n' && context->curchar != ' ' && context->curchar != '\t' && context->curchar != '(' &&
		context->curchar != '\"')
	{
		size_t position = skip_str(context); 
		macro_error(after_ident_must_be_space, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
	}*/

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

	while (context->reprtab[i] == context->mstring[j])
	{
		i++;
		j++;
		if (context->reprtab[i] == 0 && context->mstring[j] == MACROEND)
		{
			return 1;
		}
	}

	return 0;
}

int collect_mident(preprocess_context *context)
{
	int r;
	int hash = 0;
	context->msp = 0;

	while (is_letter(context) || is_digit(context->curchar))
	{
		context->mstring[context->msp++] = context->curchar;
		hash += context->curchar;
		m_nextch(context);
	}

	context->mstring[context->msp] = MACROEND;
	hash &= 255;
	r = context->hashtab[hash];

	while (r)
	{
		if (r >= context->mfirstrp && mf_equal(r, context))
		{
			return (context->macrotext[context->reprtab[r + 1]] != MACROUNDEF) ? r : 0;
		}

		r = context->reprtab[r];
	}

	return 0;
}

int find_file(preprocess_context *context, const char *s)
{
	int oldrp = context->rp;
	context->rp += 2;
	int r;
	int hash = 0;
	int i = 0;

	while (s[i] != '\0')
	{
		context->reprtab[context->rp++] = s[i];
		hash += s[i];
		i++;
	}

	hash &= 255;
	r = context->hashtab[hash];

	while (r)
	{
		if (context->reprtab[r + 1] == SH_FILE && equal_reprtab(r, oldrp, context))
		{
			context->rp = oldrp;
			return 0;
		}

		r = context->reprtab[r];
	}

	context->reprtab[oldrp] = context->hashtab[hash];
	context->reprtab[oldrp + 1] = SH_FILE;
	context->hashtab[hash] = oldrp;
	context->reprtab[context->rp++] = 0;
	return 1;
}

int space_end_line(preprocess_context *context)
{
	while (context->curchar != '\n')
	{
		if (context->curchar == ' ' || context->curchar == '\t')
		{
			m_nextch(context);
		}
		else
		{
			size_t position = skip_str(context); 
			macro_error(after_preproces_words_must_be_space, ws_get_file(context->fs.ws, context->fs.cur), context->line, context->error_string, position);
			return -1;
		}
	}

	return 0;
}

void space_skip(preprocess_context *context)
{
	while (context->curchar == ' ' || context->curchar == '\t')
	{
		m_nextch(context);
	}
}

void space_skip_str(preprocess_context *context)
{
	int c = context->curchar;
	m_fprintf(context->curchar, context);
	m_nextch(context);

	while (context->curchar != c && context->curchar != EOF)
	{
		if (context->curchar == '\\')
		{
			m_fprintf(context->curchar, context);
			m_nextch(context);
		}

		m_fprintf(context->curchar, context);
		m_nextch(context);
	}

	if (context->curchar != EOF)
	{
		m_fprintf(context->curchar, context);
		m_nextch(context);
	}
}

size_t skip_str(preprocess_context *context)
{
	char *line = context->error_string;
	size_t position = strlen(line);
	while (context->curchar != '\n' && context->curchar != EOF)
	{
		m_nextch(context);
	}
	return position;
}

void skip_file(preprocess_context *context)
{
	while (context->curchar != EOF)
	{
		m_nextch(context);
	}
}