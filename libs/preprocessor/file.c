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
#include "context_var.h"
#include "error.h"
#include "utils.h"
#include "uniprinter.h"
#include "uniscanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void m_nextch(preprocess_context *context);


int strlen32(char32_t* strarg)
{
   if(!strarg)
   {
	   return -1;
   }
   char32_t* str = strarg;
   for(;*str; ++str);
   return str-strarg;
}

int get_next_char(preprocess_context *context)
{
	context->nextchar = uni_scan_char(context->io_input);
	if (context->nextchar == U'\r')
	{
		return get_next_char(context);
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
	context->nextch_type = type;
}

void m_old_nextch_type(preprocess_context *context)
{
	context->dipp--;
	context->curchar = context->oldcurchar[context->dipp];
	context->nextchar = context->oldnextchar[context->dipp];
	context->nextch_type = context->oldnextch_type[context->dipp];
	context->nextp = context->oldnextp[context->dipp];
}

void end_line(preprocess_context *context)
{
	context->line++;
}

void m_onemore(preprocess_context *context)
{
	context->curchar = context->nextchar;

	get_next_char(context);

	if (context->curchar == EOF)
	{
		context->nextchar = EOF;
		end_line(context);
	}
}


void m_fprintf(int a, preprocess_context *context)
{
	uni_print_char(context->io_output, a);
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
				end_line(context);
			}

			if (context->curchar == EOF)
			{
				m_error(comm_not_ended, context);
				end_line(context);
				return;
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

			if(context->curchar == '\n')
			{
				con_file_print_coment(&context->fs, context);
			}
			else if (context->curchar == MACROCANGE)
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
			end_line(context);
		}
	}

	if (context->curchar != '\n')
	{
		context->error_string[context->position++] = (char)context->curchar;
		context->error_string[context->position] = '\0';
	}
	else
	{
		context->position = 0;
	}
	// printf("t = %d curchar = %c, %i nextchar = %c, %i \n", context->nextch_type,
	// context->curchar, context->curchar, context->nextchar, context->nextchar);
}
