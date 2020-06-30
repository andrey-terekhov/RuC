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

#include "file.h"
#include "constants.h"
#include "context.h"
#include "context_var.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void m_nextch(preprocess_context *context, compiler_context *c_context);


int get_next_char(preprocess_context *context)
{
	unsigned char firstchar;
	unsigned char secondchar;
	if (context->nextch_type != PREPROCESS_STRING &&
		fscanf(context->input_stak[context->inp_p], "%c", &firstchar) == EOF)
	{
		return EOF;
	}
	else if (context->nextch_type == PREPROCESS_STRING &&
			 (firstchar = context->preprocess_string[context->strp++]) == '\0')
	{
		return EOF;
	}
	else
	{
		if ((firstchar & /*0b11100000*/ 0xE0) == /*0b11000000*/ 0xC0)
		{
			if (context->nextch_type != PREPROCESS_STRING)
			{
				fscanf(context->input_stak[context->inp_p], "%c", &secondchar);
			}
			else
			{
				secondchar = context->preprocess_string[context->strp++];
			}


			context->nextchar = ((int)(firstchar & /*0b11111*/ 0x1F)) << 6 | (secondchar & /*0b111111*/ 0x3F);
		}
		else
		{
			context->nextchar = firstchar;
		}

		if (context->nextchar == 13 /* cr */)
		{
			get_next_char(context);
		}
	}

	return context->nextchar;
}

int get_dipp(preprocess_context *context)
{
	return context->dipp;
}

void m_change_nextch_type(int type, int p, preprocess_context *context, compiler_context *c_context)
{
	if (type != SOURSTYPE)
	{
		context->oldcurchar[context->dipp] = context->curchar;
		context->oldnextchar[context->dipp] = context->nextchar;
		context->oldnextch_type[context->dipp] = context->nextch_type;
		context->oldnextp[context->dipp] = context->nextp;
		context->nextp = p;
		context->dipp++;
	}
	else
	{
		context->dipp = 0;
	}

	// printf("nextch_type\n");
	context->nextch_type = type;
	m_nextch(context, c_context);
}

void m_old_nextch_type(preprocess_context *context)
{
	context->dipp--;
	context->curchar = context->oldcurchar[context->dipp];
	context->nextchar = context->oldnextchar[context->dipp];
	context->nextch_type = context->oldnextch_type[context->dipp];
	context->nextp = context->oldnextp[context->dipp];
	// printf("oldnextch_type = %d\n", nextch_type);
}

void m_end_line(compiler_context *c_context)
{
	int j;

	c_context->mlines[++c_context->mline] = c_context->m_charnum;
	c_context->mlines[c_context->mline + 1] = c_context->m_charnum;

	printf("Line %i) ", c_context->mline - 1);

	for (j = c_context->mlines[c_context->mline - 1]; j < c_context->mlines[c_context->mline]; j++)
	{
		if (c_context->before_source[j] != EOF)
		{
			printf_character(c_context->before_source[j]);
		}
	}
}

void m_onemore(preprocess_context *context, compiler_context *c_context)
{
	context->curchar = context->nextchar;
	context->nextchar = get_next_char(context);
	c_context->before_source[c_context->m_charnum++] = context->curchar;

	if (context->curchar == EOF)
	{
		m_end_line(c_context);
		printf("\n");
	}
}

void m_fprintf(int a, preprocess_context *context, compiler_context *c_context)
{
	if (a == '\n')
	{
		c_context->m_conect_lines[context->mclp++] = c_context->mline - 1;
	}

	printer_printchar(&c_context->output_options, a);
	// printf_character(a);
	// printf(" %d ", a);
	// printf(" t = %d n = %d\n", nextch_type,context -> nextp);
}

void m_coment_skip(preprocess_context *context, compiler_context *c_context)
{
	if (context->curchar == '/' && context->nextchar == '/')
	{
		do
		{
			// m_fprintf_com();
			m_onemore(context, c_context);

			if (context->curchar == EOF)
			{
				return;
			}
		} while (context->curchar != '\n');
	}

	if (context->curchar == '/' && context->nextchar == '*')
	{
		// m_fprintf_com();
		m_onemore(context, c_context);
		// m_fprintf_com();

		do
		{
			m_onemore(context, c_context);
			// m_fprintf_com();

			if (context->curchar == EOF)
			{
				m_end_line(c_context);
				printf("\n");
				m_error(comm_not_ended, c_context);
			}
		} while (context->curchar != '*' || context->nextchar != '/');

		m_onemore(context, c_context);
		// m_fprintf_com();
		context->curchar = ' ';
	}
}

void m_nextch_cange(preprocess_context *context, compiler_context *c_context)
{
	m_nextch(context, c_context);
	// printf("2 lsp = %d context->curchar = %d l = %d\n",  lsp, context->curchar, context->curchar + lsp);
	m_change_nextch_type(FTYPE, context->localstack[context->curchar + context->lsp], context, c_context);
}

void m_nextch(preprocess_context *context, compiler_context *c_context)
{
	if (context->nextch_type != 0 && context->nextch_type <= TEXTTYPE)
	{
		if (context->nextch_type == SOURSTYPE)
		{
			context->curchar = c_context->before_source[context->nextp++];
			context->nextchar = c_context->before_source[context->nextp];
		}
		else if (context->nextch_type == MTYPE && context->nextp < context->msp)
		{
			context->curchar = context->mstring[context->nextp++];
			context->nextchar = context->mstring[context->nextp];
		}
		else if (context->nextch_type == CTYPE && context->nextp < context->csp)
		{
			context->curchar = context->cstring[context->nextp++];
			context->nextchar = context->cstring[context->nextp];
		}
		else if (context->nextch_type == IFTYPE && context->nextp < context->ifsp)
		{
			context->curchar = context->ifstring[context->nextp++];
			context->nextchar = context->ifstring[context->nextp];
		}
		else if (context->nextch_type == WHILETYPE && context->nextp < context->wsp)
		{
			context->curchar = context->wstring[context->nextp++];
			context->nextchar = context->wstring[context->nextp];
		}
		else if (context->nextch_type == TEXTTYPE && context->nextp < context->mp)
		{
			context->curchar = context->macrotext[context->nextp++];
			context->nextchar = context->macrotext[context->nextp];
			// printf(" i = %d curcar = %c curcar = %i n = %d\n", nextch_type, context->curchar,
			// context->curchar,context -> nextp);

			if (context->curchar == MACROCANGE)
			{
				m_nextch_cange(context, c_context);
			}
			else if (context->curchar == MACROEND)
			{
				m_old_nextch_type(context);
			}
		}
		else if (context->nextch_type == FTYPE)
		{
			context->curchar = context->fchange[context->nextp++];
			context->nextchar = context->fchange[context->nextp];

			if (context->curchar == CANGEEND)
			{
				m_old_nextch_type(context);
				m_nextch(context, c_context);
			}
		}
		else
		{
			m_old_nextch_type(context);
		}
	}
	else
	{
		m_onemore(context, c_context);
		m_coment_skip(context, c_context);

		if (context->curchar == '\n')
		{
			m_end_line(c_context);
		}
	}

	//	printf(" t = %d curcar = %c curcar = %i n = %d f = %d\n", context->nextch_type,
	//		context->curchar, context->curchar, context->nextp, context->inp_p);
}
