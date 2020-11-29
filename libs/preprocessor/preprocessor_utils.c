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
#include "context_var.h"
#include "file.h"
#include "preprocessor_error.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int strlen32(const char32_t* strarg)
{
   if(!strarg)
     return -1; //strarg is NULL pointer
   char32_t* str = strarg;
   for(;*str;++str)
     ; // empty body
   return str-strarg;
}

int is_letter(preprocess_context *context)
{
	if ((context->curchar >= 'A' && context->curchar <= 'Z') ||
		   (context->curchar >= 'a' && context->curchar <= 'z') || context->curchar == '_' ||
		   (context->curchar >= 0x410 /*'А'*/ && context->curchar <= 0x44F /*'я'*/))
	{
		return 1;
	}
	return 0;
}

int is_digit(int a)
{
	if(a >= '0' && a <= '9')
	{
		return 1;
	}
	return 0;
}

void collect_mident(preprocess_context *context, char32_t *str)
{
	int i = 1;
	int hash = 0;
	do
	{
		str[i++] = context->curchar;
		hash += context->curchar;
		m_nextch(context);
	} while (is_letter(context) != 0 || is_digit(context->curchar) != 0);
	str[i++] = 0;
	str[0] = hash & 255;
	return;
}

void space_end_line(preprocess_context *context)
{
	while (context->curchar != '\n')
	{
		if (context->curchar == ' ' || context->curchar == '\t')
		{
			m_nextch(context);
		}
		else
		{
			m_error(after_preproces_words_must_be_space, context);
		}
	}
	m_nextch(context);
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
