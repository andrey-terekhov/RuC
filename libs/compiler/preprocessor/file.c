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


void m_nextch(preprocess_context *context);


int get_next_char(preprocess_context *context)
{
	unsigned char firstchar;
	unsigned char secondchar;
	if (fscanf(context->current_file, "%c", &firstchar) == EOF)
	{
		return context->nextchar = EOF;
	}
	else
	{
		if ((firstchar & /*0b11100000*/ 0xE0) == /*0b11000000*/ 0xC0)
		{
			fscanf(context->current_file, "%c", &secondchar);
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

void m_change_nextch_type(int type, int p, preprocess_context *context)
{
	context->oldcurchar[context->dipp] = context->curchar;
	context->oldnextchar[context->dipp] = context->nextchar;
	context->oldnextch_type[context->dipp] = context->nextch_type;
	context->oldnextp[context->dipp] = context->nextp;
	context->nextp = p;
	context->dipp++;

	// printf("nextch_type\n");
	context->nextch_type = type;
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

void control_string_pinter(preprocess_context *context, int before, int after)
{
	control_string *cs;

	if (context->h_flag == 0)
	{
		cs = &context->sources->files[context->sources->cur].cs;
	}
	else
	{
		cs = &context->headers->files[context->headers->cur].cs;
	}

	cs->str_before[cs->p] = before;
	cs->str_after[cs->p] = after;
	cs->p++;
}

void end_line(preprocess_context *context, macro_long_string *s)
{
	if (context->include_type > 0)
	{
		context->control_aflag++;
	}
	if (context->FILE_flag)
	{
		context->line++; //!!

#if MACRODEBUG
		if (context->line == 2)
		{
			printf("\nИсходный текст:\n\n");
		}

		printf("Line %i) ", context->line - 1);

		for (int j = context->temp_output; j < s->p; j++)
		{
			if (s->str[j] != EOF)
			{
				printf_character(s->str[j]);
			}
		}
#endif

		context->temp_output = s->p;
	}
}

void m_onemore(preprocess_context *context)
{
	context->curchar = context->nextchar;
	if (context->FILE_flag)
	{
		get_next_char(context);
		long_string_pinter(context->before_temp, context->curchar);

		if (context->curchar == EOF)
		{
			end_line(context, context->before_temp);
		}
	}
	else if (context->current_string != NULL)
	{
		context->nextchar = context->current_string[context->current_p++];
	}
	else
	{
		context->nextchar = EOF;
	}
}


void m_fprintf(int a, preprocess_context *context)
{
	if (a == '\n')
	{
		context->control_bflag++;
		if (context->control_aflag != 1)
		{
			control_string_pinter(context, context->control_bflag, context->control_aflag);
		}
		context->control_aflag = 0;
	}

	printer_printchar(&context->output_options, a);
	// printf_character(a);
	// printf(", %d; \n", a);
	// printf(" t = %d n = %d\n", nextch_type,context -> nextp);
}

void m_coment_skip(preprocess_context *context)
{
	if (context->curchar == '/' && context->nextchar == '/')
	{
		do
		{
			// m_fprintf_com();
			m_onemore(context);

			if (context->curchar == EOF)
			{
				return;
			}
		} while (context->curchar != '\n');
	}

	if (context->curchar == '/' && context->nextchar == '*')
	{
		// m_fprintf_com();
		m_onemore(context);
		// m_fprintf_com();

		do
		{
			m_onemore(context);
			// m_fprintf_com();
			if (context->curchar == '\n')
			{
				end_line(context, context->before_temp);
			}

			if (context->curchar == EOF)
			{
				end_line(context, context->before_temp);
				m_error(comm_not_ended, context);
			}
		} while (context->curchar != '*' || context->nextchar != '/');

		m_onemore(context);
		// m_fprintf_com();
		context->curchar = ' ';
	}
}

void m_nextch_cange(preprocess_context *context)
{
	m_nextch(context);
	// printf("2 lsp = %d context->curchar = %d l = %d\n",  lsp, context->curchar, context->curchar + lsp);
	m_change_nextch_type(FTYPE, context->localstack[context->curchar + context->lsp], context);
	m_nextch(context);
}

void m_nextch(preprocess_context *context)
{
	if (context->nextch_type != FILETYPE && context->nextch_type <= TEXTTYPE)
	{
		if (context->nextch_type == MTYPE && context->nextp < context->msp)
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
				m_nextch_cange(context);
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
				m_nextch(context);
			}
		}
		else
		{
			m_old_nextch_type(context);
		}
	}
	else
	{
		m_onemore(context);

		m_coment_skip(context);

		if (context->curchar == '\n')
		{
			end_line(context, context->before_temp);
		}
	}

	// printf(" t = %d curcar = %c curcar = %i n = %d \n", context->nextch_type,
	// context->curchar, context->curchar, context->nextp);
}
