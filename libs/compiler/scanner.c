/*
 *	Copyright 2014 Andrey Terekhov
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

#include "errors.h"
#include "global.h"
#include "uniscanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int getnext(compiler_context *context)
{
	int ret = scanner_getnext(&context->input_options);
	context->nextchar = ret;
	return ret;
}

void onemore(compiler_context *context)
{
	context->curchar = context->nextchar;
	context->nextchar = getnext(context);
	//	if (kw)
	//		printf("context->curchar =%c %i context->nextchar=%c %i\n",
	//		context->curchar, context->curchar, context->nextchar,
	//		context->nextchar);
}

void endofline(compiler_context *context)
{
	if (context->prep_flag == 1)
	{
		int j;
		printer_printf(&context->output_options, "line %i) ", context->line - 1);
		for (j = context->lines[context->line - 1]; j < context->lines[context->line]; j++)
		{
			if (context->source[j] != EOF)
			{
				printer_printchar(&context->output_options, context->source[j]);
			}
		}
		fflush(stdout);
	}
}

void endnl(compiler_context *context)
{
	context->lines[++context->line] = context->charnum;
	context->lines[context->line + 1] = context->charnum;
	if (context->kw)
	{
		endofline(context);
	}
}

void nextch(compiler_context *context)
{
	onemore(context);
	if (context->curchar == EOF)
	{
		onemore(context);
		context->lines[++context->line] = context->charnum;
		context->lines[context->line + 1] = context->charnum;
		if (context->kw)
		{
			endofline(context);
			printer_printf(&context->output_options, "\n");
		}
		return;
	}

	context->source[context->charnum++] = context->curchar;
	if (context->instring)
	{
		return;
	}

	if (context->curchar == '/' && context->nextchar == '/')
	{
		do
		{
			onemore(context);
			context->source[context->charnum++] = context->curchar;
			if (context->curchar == EOF)
			{
				endnl(context);
				printer_printf(&context->output_options, "\n");
				return;
			}
		} while (context->curchar != '\n');

		endnl(context);
		return;
	}

	if (context->curchar == '/' && context->nextchar == '*')
	{
		onemore(context);
		context->source[context->charnum++] = context->curchar; // надо сразу выесть /*, чтобы не попасть на /*/
		do
		{
			onemore(context);
			context->source[context->charnum++] = context->curchar;
			if (context->curchar == EOF)
			{
				endnl(context);
				printer_printf(&context->output_options, "\n");
				error(context, comm_not_ended);
			}
			if (context->curchar == '\n')
			{
				endnl(context);
			}
		} while (context->curchar != '*' || context->nextchar != '/');

		onemore(context);
		context->source[context->charnum++] = context->curchar;
		context->curchar = ' ';
		return;
	}
	if (context->curchar == '\n')
	{
		endnl(context);
	}
	return;
}

void next_string_elem(compiler_context *context)
{
	context->num = context->curchar;
	if (context->curchar == '\\')
	{
		nextch(context);
		if (context->curchar == 'n' || context->curchar == 1085 /* 'н' */)
		{
			context->num = 10;
		}
		else if (context->curchar == 't' || context->curchar == 1090 /* 'т' */)
		{
			context->num = 9;
		}
		else if (context->curchar == '0')
		{
			context->num = 0;
		}
		else if (context->curchar != '\'' && context->curchar != '\\' && context->curchar != '\"')
		{
			error(context, bad_escape_sym);
		}
		else
		{
			context->num = context->curchar;
		}
	}
	nextch(context);
}

int letter(compiler_context *context)
{
	return (context->curchar >= 'A' && context->curchar <= 'Z') ||
		   (context->curchar >= 'a' && context->curchar <= 'z') || context->curchar == '_' ||
		   (context->curchar >= 0x410 /*А */ && context->curchar <= 0x44F /*'я'*/);
}

int digit(compiler_context *context)
{
	return context->curchar >= '0' && context->curchar <= '9';
}

int ispower(compiler_context *context)
{
	return context->curchar == 'e' || context->curchar == 'E'; // || context->curchar == 'е' || context->curchar == 'Е')
															   // // это русские е и Е
}

int equal(compiler_context *context, int i, int j)
{
	++i;
	++j;
	/* Assuming allocated */
	while (REPRTAB[++i] == REPRTAB[++j])
	{
		if (REPRTAB[i] == 0 && REPRTAB[j] == 0)
		{
			return 1;
		}
	}
	return 0;
}

int scan(compiler_context *context)
{
	int cr;
	while (context->curchar == ' ' || context->curchar == '\t' || context->curchar == '\n')
	{
		nextch(context);
	}
   // printf("scan context->curchar=%c %i\n", context->curchar, context->curchar);
	switch (context->curchar)
	{
		case EOF:
		{
			return LEOF;
		}
		case ',':
		{
			nextch(context);
			return COMMA;
		}

		case '=':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = EQEQ;
			}
			else
			{
				cr = ASS;
			}
			return cr;

		case '*':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = MULTASS;
			}
			else
			{
				cr = LMULT;
			}
			return cr;

		case '/':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = DIVASS;
			}
			else
			{
				cr = LDIV;
			}
			return cr;

		case '%':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = REMASS;
			}
			else
			{
				cr = LREM;
			}
			return cr;

		case '+':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = PLUSASS;
			}
			else if (context->curchar == '+')
			{
				nextch(context);
				cr = INC;
			}
			else
			{
				cr = LPLUS;
			}
			return cr;

		case '-':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = MINUSASS;
			}
			else if (context->curchar == '-')
			{
				nextch(context);
				cr = DEC;
			}
			else if (context->curchar == '>')
			{
				nextch(context);
				cr = ARROW;
			}
			else
            {
				cr = LMINUS;
			}
                return cr;

		case '<':
			nextch(context);
			if (context->curchar == '<')
			{
				nextch(context);
				if (context->curchar == '=')
				{
					nextch(context);
					cr = SHLASS;
				}
				else
				{
					cr = LSHL;
				}
			}
			else if (context->curchar == '=')
			{
				nextch(context);
				cr = LLE;
			}
			else
			{
				cr = LLT;
			}

			return cr;

		case '>':
			nextch(context);
			if (context->curchar == '>')
			{
				nextch(context);
				if (context->curchar == '=')
				{
					nextch(context);
					cr = SHRASS;
				}
				else
				{
					cr = LSHR;
				}
			}
			else if (context->curchar == '=')
			{
				nextch(context);
				cr = LGE;
			}
			else
			{
				cr = LGT;
			}
			return cr;

		case '&':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = ANDASS;
			}
			else if (context->curchar == '&')
			{
				nextch(context);
				cr = LOGAND;
			}
			else
			{
				cr = LAND;
			}
			return cr;

		case '^':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = EXORASS;
			}
			else
			{
				cr = LEXOR;
			}
			return cr;

		case '|':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = ORASS;
			}
			else if (context->curchar == '|')
			{
				nextch(context);
				cr = LOGOR;
			}
			else
			{
				cr = LOR;
			}
			return cr;

		case '!':
			nextch(context);
			if (context->curchar == '=')
			{
				nextch(context);
				cr = NOTEQ;
			}
			else
			{
				cr = LOGNOT;
			}
			return cr;
		case '\'':
		{
			context->instring = 1;
			nextch(context);
			next_string_elem(context);
			if (context->curchar != '\'')
			{
				error(context, no_right_apost);
			}
			nextch(context);
			context->instring = 0;
			context->ansttype = LCHAR;
			return NUMBER;
		}
		case '\"':
		{
			int n = 0;
			int flag = 1;
			context->instring = 1;
			nextch(context);
			while (flag)
			{
				while (context->curchar != '\"' && n < MAXSTRINGL)
				{
					next_string_elem(context);
					context->lexstr[n++] = context->num;
					// printf("n= %i %c %i\n", n-1,
					// context->num, context->num);
				}
				if (n == MAXSTRINGL)
				{
					error(context, too_long_string);
				}
				nextch(context);
				while (context->curchar == ' ' || context->curchar == '\t' || context->curchar == '\n')
				{
					nextch(context);
				}
				if (context->curchar == '\"')
				{
					nextch(context);
				}
				else
				{
					flag = 0;
				}
			}

			context->num = n;
			context->instring = 0;
			return STRING;
		}
		case '(':
		{
			nextch(context);
			return LEFTBR;
		}

		case ')':
		{
			nextch(context);
			return RIGHTBR;
		}

		case '[':
		{
			nextch(context);
			return LEFTSQBR;
		}

		case ']':
		{
			nextch(context);
			return RIGHTSQBR;
		}

		case '~':
		{
			nextch(context);
			return LNOT; // поразрядное отрицание
		}
		case '{':
		{
			nextch(context);
			return BEGIN;
		}
		case '}':
		{
			nextch(context);
			return END;
		}
		case ';':
		{
			nextch(context);
			return SEMICOLON;
		}
		case '?':
		{
			nextch(context);
			return QUEST;
		}
		case ':':
		{
			nextch(context);
			return COLON;
		}
		case '.':
			if (context->nextchar < '0' || context->nextchar > '9')
			{
				nextch(context);
				return DOT;
			}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			int flagint = 1;
			int flagtoolong = 0;
			double k;
			context->num = 0;
			context->numdouble = 0.0;
			while (digit(context))
			{
				context->numdouble = context->numdouble * 10 + (context->curchar - '0');
				if (context->numdouble > (double)INT_MAX)
				{
					flagtoolong = 1;
					flagint = 0;
				}
				context->num = context->num * 10 + (context->curchar - '0');
				nextch(context);
			}

			if (context->curchar == '.')
			{
				flagint = 0;
				nextch(context);
				k = 0.1;
				while (digit(context))
				{
					context->numdouble += (context->curchar - '0') * k;
					k *= 0.1;
					nextch(context);
				}
			}

			if (ispower(context))
			{
				int d = 0;
				int k = 1;
				int i;
				nextch(context);
				if (context->curchar == '-')
				{
					flagint = 0;
					nextch(context);
					k = -1;
				}
				else if (context->curchar == '+')
				{
					nextch(context);
				}
				if (!digit(context))
				{
					error(context, must_be_digit_after_exp);
				}
				while (digit(context))
				{
					d = d * 10 + context->curchar - '0';
					nextch(context);
				}
				if (flagint)
				{
					for (i = 1; i <= d; i++)
					{
						context->num *= 10;
					}
				}
				context->numdouble *= pow(10.0, k * d);
			}

			if (flagint)
			{
				context->ansttype = LINT;
                return NUMBER;
			}
			else
			{
				if (flagtoolong)
				{
					warning(context, too_long_int);
				}
				context->ansttype = LFLOAT;
			}
			memcpy(&context->numr, &context->numdouble, sizeof(double));
			return NUMBER;
		}

		default:
			if (letter(context) || context->curchar == '#')
			{
				int oldrepr = REPRTAB_LEN;
				int r;
				compiler_table_expand(&context->reprtab, 2);
				REPRTAB_LEN += 2;
				context->hash = 0;

				do
				{
					context->hash += context->curchar;
					compiler_table_expand(&context->reprtab, 1);
					REPRTAB[REPRTAB_LEN++] = context->curchar;
					nextch(context);
				} while (letter(context) || digit(context));

				context->hash &= 255;
				compiler_table_expand(&context->reprtab, 1);
				REPRTAB[REPRTAB_LEN++] = 0;
				r = context->hashtab[context->hash];
				if (r)
				{
					do
					{
						if (equal(context, r, oldrepr))
						{
							REPRTAB_LEN = oldrepr;
							compiler_table_ensure_allocated(&context->reprtab, r + 1);
							return (REPRTAB[r + 1] < 0) ? REPRTAB[r + 1] : (REPRTAB_POS = r, IDENT);
						}
						else
						{
							compiler_table_ensure_allocated(&context->reprtab, r);
							r = REPRTAB[r];
						}
					} while (r);
				}

				compiler_table_ensure_allocated(&context->reprtab, oldrepr);
				REPRTAB[oldrepr] = context->hashtab[context->hash];
				REPRTAB_POS = context->hashtab[context->hash] = oldrepr;
				compiler_table_expand(&context->reprtab, 1);
				REPRTAB[REPRTAB_POS + 1] =
					(context->keywordsnum) ? -((++context->keywordsnum - 2) / 4) : 1; // 0 - только MAIN, < 0 - ключевые
																					  // слова, 1 - обычные иденты
				return IDENT;
			}
			else
			{
				printer_printf(&context->err_options, "плохой символ %c %i\n", context->curchar, context->curchar);
				nextch(context);
				exit(10);
			}
	}
}

int scaner(compiler_context *context)
{
	context->cur = context->next;
	context->next = scan(context);
	//	if(kw)
	//		printf("scaner context->cur %i context->next %i repr %i\n",
	//		context->cur, context->next, repr);
	return context->cur;
}
