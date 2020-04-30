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

#include "preprocess.h"
#include "errors.h"
#include "global.h"
#include "scanner.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define STRIGSIZE 70


/* Forward declarations */
static int mletter(compiler_context *context, int r);
static int mdigit(compiler_context *context, int r);
static int mequal(compiler_context *context, int str[], int j);

static void mend_line(compiler_context *context);
static void m_nextch(compiler_context *context, int i);
static void m_fprintf(compiler_context *context, int a);

static void to_macrotext(compiler_context *, int oldrepr); //
static void macro_reprtab(compiler_context *, char chang[]);
static void from_macrotext(compiler_context *); // 5
static int macro_keywords(compiler_context *);	// 12
static void relis_define(compiler_context *);	// 2

static void toreprtab_f(compiler_context *);
static void to_functionident(compiler_context *); // 4
static int scob(compiler_context *, int cp);	  // 6
static void from_functionident(compiler_context *, int r);
static void create_change(compiler_context *, int r1); // 11
static void r_macrofunction(compiler_context *);	   // 3

// void m_ident();	//5
static int find_ident(compiler_context *);

static int check_if(compiler_context *, int type_if); // 10
static void end_line(compiler_context *);			  // 9
static void false_if(compiler_context *);			  // 8
static int m_false(compiler_context *);				  // 7
static void m_true(compiler_context *, int type_if);
static void m_if(compiler_context *, int type_if);

static void macroscan(compiler_context *context); // 1,17
void preprocess_file(compiler_context *context);  // 18


void show_macro(compiler_context *context)
{
	int i1 = context->lines[context->line];
	int str1[STRIGSIZE];
	int j = 0;
	int k;
	int flag = 1;

	context->arg = context->mlines[context->m_conect_lines[context->line]];

	context->flag_show_macro = 1;
	while (i1 < context->charnum)
	{
		// printf("\nbe[context->arg= %i] = %i, so[i1] = %i",context->arg,
		// context->before_source[context->arg],context->source[i1] );
		if (context->source[i1] == context->before_source[context->arg])
		{
			str1[j++] = context->before_source[context->arg];
			i1++;
			context->arg++;
		}
		else
		{
			flag = 0;
			context->curchar = context->before_source[context->arg];
			from_macrotext(context);

			i1 += context->msp;
		}
	}

	printer_printf(&context->miscout_options, "line %i) ", context->m_conect_lines[context->line]);

	for (k = 0; k < j; k++)
	{
		printer_printchar(&context->miscout_options, str1[k]);
	}

	if (flag == 0)
	{
		printer_printf(&context->miscout_options,
					   "\n В строке есть макрозамена, строка после "
					   "макрогенерации:\nline %i)",
					   context->m_conect_lines[context->line]);
		for (k = context->lines[context->line - 1]; k < context->charnum; k++)
		{
			printer_printchar(&context->miscout_options, context->source[k]);
		}
	}
}

static int mletter(compiler_context *context, int r)
{
	UNUSED(context);
	return (r >= 'A' && r <= 'Z') || (r >= 'a' && r <= 'z') || r == '_' || (r >= 0x410 /*А */ && r <= 0x44F /*'я'*/);
}

static int mdigit(compiler_context *context, int r)
{
	UNUSED(context);
	return r >= '0' && r <= '9';
}

static int mequal(compiler_context *context, int str[], int j)
{
	int i = 0;
	while (str[i++] == context->functionident[j++])
	{
		if (str[i] == 0 && context->functionident[j] == 0)
		{
			return 1;
		}
	}
	return 0;
}

static void mend_line(compiler_context *context)
{
	int j;

	if (context->flag_show_macro == 0)
	{
		context->mlines[++context->mline] = context->m_charnum;
		context->mlines[context->mline + 1] = context->m_charnum;

		if (context->kw)
		{
			printer_printf(&context->miscout_options, "Line %i) ", context->mline - 1);
			for (j = context->mlines[context->mline - 1]; j < context->mlines[context->mline]; j++)
			{
				if (context->before_source[j] != EOF)
				{
					printer_printchar(&context->miscout_options, context->before_source[j]);
				}
			}
		}
	}

	return;
}

static void monemore(compiler_context *context)
{
	if (context->flag_show_macro == 0)
	{
		context->curchar = context->nextchar;
		context->nextchar = getnext(context);
		context->before_source[context->m_charnum++] = context->curchar;
	}
	else
	{
		context->curchar = context->before_source[context->arg++];
	}

	if (context->curchar == EOF)
	{
		mend_line(context);
		printer_printf(&context->miscout_options, "\n");
		return;
	}
}

static void m_nextch(compiler_context *context, int i)
{
	// printf(" i = %d curcar = %c curcar = %i\n", i, context->curchar,
	// context->curchar);
	monemore(context);

	if (context->curchar == '/' && context->nextchar == '/')
	{
		if (i > 13)
		{
			m_fprintf(context, context->curchar);
		}
		do
		{
			monemore(context);
			if (i > 13)
			{
				m_fprintf(context, context->curchar);
			}
		} while (context->curchar != '\n');

		mend_line(context);
		return;
	}

	if (context->curchar == '/' && context->nextchar == '*')
	{
		if (i > 13)
		{
			m_fprintf(context, context->curchar);
		}

		monemore(context);
		if (i > 13)
		{
			m_fprintf(context, context->curchar);
		}
		do
		{
			monemore(context);
			if (i > 13)
			{
				m_fprintf(context, context->curchar);
			}

			if (context->curchar == EOF)
			{
				mend_line(context);
				printf("\n");
				m_error(context, comm_not_ended);
			}
			if (context->curchar == '\n')
			{
				mend_line(context);
			}
		} while (context->curchar != '*' || context->nextchar != '/');

		monemore(context);
		if (i > 13)
		{
			m_fprintf(context, context->curchar);
		}
		context->curchar = ' ';
		return;
	}

	if (context->curchar == '\n')
	{
		mend_line(context);
	}
	return;
}

static void m_fprintf(compiler_context *context, int a)
{
	if (a == '\n')
	{
		context->m_conect_lines[context->mcl++] = context->mline - 1;
	}
	printer_printchar(&context->output_options, a);
	// _obsolete_fprintf_char(context->output_options.output, a);

	return;
}

static void end_line_space(compiler_context *context)
{
	while (context->curchar != '\n')
	{
		if (context->curchar == ' ' || context->curchar == '\t')
		{
			m_nextch(context, 9);
		}
		else
		{
			m_error(context, after_preproces_words_must_be_space);
		}
	}
	m_nextch(context, 9);
}

static int find_ident(compiler_context *context)
{
	int fpr = REPRTAB_LEN;
	int i;
	int r;

	context->hash = 0;
	fpr += 2;
	for (i = 0; i < context->msp; i++)
	{
		context->hash += context->mstring[i];
		compiler_table_ensure_allocated(&context->reprtab, fpr + 1);
		REPRTAB[fpr++] = context->mstring[i];
	}
	compiler_table_ensure_allocated(&context->reprtab, fpr + 1);
	REPRTAB[fpr++] = 0;
	context->hash &= 255;
	r = context->hashtab[context->hash];
	while (r)
	{
		compiler_table_ensure_allocated(&context->reprtab, r);
		if (r >= context->mfirstrp && r <= context->mlastrp && equal(context, r, REPRTAB_LEN))
		{
			return r;
		}

		r = REPRTAB[r];
	}
	return 0;
}

static int macro_keywords(compiler_context *context)
{
	int oldrepr = REPRTAB_LEN;
	int r = 0;

	compiler_table_expand(&context->reprtab, 2);
	REPRTAB_LEN += 2;
	context->hash = 0;

	do
	{
		compiler_table_expand(&context->reprtab, 1);
		context->hash += context->curchar;
		REPRTAB[REPRTAB_LEN++] = context->curchar;
		m_nextch(context, 12);
	} while (letter(context) || digit(context));

	if (context->curchar != ' ' && context->curchar != '\n' && context->curchar != '\t')
	{
		m_error(context, after_ident_must_be_space);
	}
	else
	{
		m_nextch(context, 12);
	}

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
	return 0;
}

static int to_reprtab(compiler_context *context)
{
	int i;
	int r;
	int oldrepr = REPRTAB_LEN;

	context->mlastrp = oldrepr;
	context->hash = 0;
	compiler_table_expand(&context->reprtab, 2);
	REPRTAB_LEN += 2;
	do
	{
		context->hash += context->curchar;
		compiler_table_expand(&context->reprtab, 1);
		REPRTAB[REPRTAB_LEN++] = context->curchar;
		m_nextch(context, 2);
	} while (letter(context) || digit(context));

	context->hash &= 255;
	compiler_table_expand(&context->reprtab, 1);
	REPRTAB[REPRTAB_LEN++] = 0;

	r = context->hashtab[context->hash];
	while (r)
	{
		if (equal(context, r, oldrepr))
		{
			m_error(context, repeat_ident);
		}

		r = REPRTAB[r];
	}

	REPRTAB[oldrepr] = context->hashtab[context->hash];
	context->hashtab[context->hash] = oldrepr;

	return oldrepr;
}

static void to_macrotext(compiler_context *context, int oldrepr)
{
	m_nextch(context, 2);

	context->macrotext[context->mp++] = oldrepr;

	while (context->curchar != '\n')
	{
		context->macrotext[context->mp++] = context->curchar;
		m_nextch(context, 2);

		if (context->curchar == EOF)
		{
			m_error(context, not_end_fail_preprocess);
		}

		if (context->curchar == '\\')
		{
			m_nextch(context, 2);
			end_line_space(context);
		}
	}

	context->macrotext[context->mp++] = 0;
}

static void from_macrotext(compiler_context *context)
{
	int r;

	context->msp = 0;

	while (letter(context) || digit(context))
	{
		context->mstring[context->msp++] = context->curchar;
		m_nextch(context, 5);
	}

	r = find_ident(context);
	// printf("r = %d\n", r);

	if (r)
	{
		context->msp = 0;
		compiler_table_ensure_allocated(&context->reprtab, r + 1);
		if (REPRTAB[r + 1] == 2)
		{
			from_functionident(context, r);
			return;
		}

		r = REPRTAB[r + 1] + 1;

		for (; context->macrotext[r] != 0; r++)
		{
			context->mstring[context->msp++] = context->macrotext[r];
		}
	}

	return;
}

static void relis_define(compiler_context *context)
{
	if (letter(context))
	{
		int oldrepr = to_reprtab(context);

		context->msp = 0;

		if (context->curchar == '(')
		{
			REPRTAB[oldrepr + 1] = 2;
			compiler_table_ensure_allocated(&context->reprtab, REPRTAB_LEN + 2);
			REPRTAB[REPRTAB_LEN++] = context->fip;
			REPRTAB[REPRTAB_LEN++] = 0;

			m_nextch(context, 2);
			r_macrofunction(context);
			return;
		}
		else if (context->curchar != ' ')
		{
			m_error(context, after_ident_must_be_space);
		}
		else
		{
			REPRTAB[oldrepr + 1] = context->mp;
			to_macrotext(context, oldrepr);

			return;
		}
	}
	else
	{
		m_error(context, ident_begins_with_letters);
	}
}

static void to_functionident(compiler_context *context) // define c параметрами
{
	while (context->curchar != ')')
	{
		//     reportab
		//        \/
		// funcid 5[] -> конец = 13
		//        6[] -> macrofunc
		//        7[] -> fcang
		//        8[a]
		//        9[0]
		//       10[] -> fcang
		//       11[b]
		//       12[0]

		context->msp = 0;
		context->fip++;
		if (letter(context))
		{
			while (letter(context) || digit(context))
			{
				context->functionident[context->fip++] = context->curchar; // 11[b]
				context->mstring[context->msp++] = context->curchar;	   // 12[0]
				m_nextch(context, 4);
			}
			if (find_ident(context) != 0)
			{
				m_error(context, repeat_ident);
			}
			context->functionident[context->fip++] = 0;
		}
		else
		{
			m_error(context, functionid_begins_with_letters);
		}
		context->msp = 0;
		if (context->curchar == ',' && context->nextchar == ' ')
		{
			m_nextch(context, 4);
			m_nextch(context, 4);
		}
		else if (context->curchar != ')')
		{
			m_error(context, after_functionid_must_be_comma);
		}
	}
	m_nextch(context, 4);
	return;
}

static void from_functionident(compiler_context *context, int r)
{
	int i;
	int kp;
	int cp;
	int r1 = r + 2;
	int str[STRIGSIZE];
	int finish;
	int newfi;
	int flag;

	compiler_table_ensure_allocated(&context->reprtab, r1);
	/* Assuming it is allocated */
	for (; REPRTAB[r1] != 0; r1++)
	{
		compiler_table_ensure_allocated(&context->reprtab, r1 + 1);
	}

	r1++;
	r1 = REPRTAB[r1];
	compiler_table_ensure_allocated(&context->reprtab, r1);
	create_change(context, r1);

	finish = context->functionident[r1];
	newfi = context->functionident[r1 + 1];
	flag = 1;

	context->msp = 0;
	while (context->macrofunction[newfi] != '\n')
	{
		if (mletter(context, context->macrofunction[newfi]))
		{
			flag = 1;
			for (i = 0; i < STRIGSIZE; i++)
			{
				str[i] = 0;
			}
			i = 0;
			while (mletter(context, context->macrofunction[newfi]) || mdigit(context, context->macrofunction[newfi]))
			{
				str[i++] = context->macrofunction[newfi++];
			}
			for (kp = r1 + 2; kp < finish;)
			{
				if (mequal(context, str, kp + 1))
				{
					for (cp = context->functionident[kp]; context->fchange[cp] != '\n'; cp++)
					{
						context->mstring[context->msp++] = context->fchange[cp];
					}
					flag = 0;
					break;
				}
				while (context->functionident[kp++] != 0)
				{
					;
				}
			}
			if (flag == 1)
			{
				for (i = 0; str[i] != 0; i++)
				{
					context->mstring[context->msp++] = str[i];
				}
			}
		}
		else
		{
			context->mstring[context->msp++] = context->macrofunction[newfi];
			newfi++;
		}
	}
}

static int scob(compiler_context *context, int cp)
{
	int i;

	context->fchange[cp++] = context->curchar;
	m_nextch(context, 6);

	while (context->curchar != EOF)
	{
		if (letter(context))
		{
			from_macrotext(context);
			for (i = 0; i < context->msp; i++)
			{
				context->fchange[cp++] = context->mstring[i];
			}
		}
		else if (context->curchar == '(')
		{
			cp = scob(context, cp);
		}
		else
		{
			context->fchange[cp++] = context->curchar;
			m_nextch(context, 6);
			if (context->curchar != ')')
			{
				context->fchange[cp++] = context->curchar;
				m_nextch(context, 6);

				return cp;
			}
		}
	}
	m_error(context, scob_not_clous);
	return cp;
}

static void create_change(compiler_context *context, int r1)
{
	int i;
	int r = r1 + 2;
	int cp = 1;

	context->functionident[r] = cp;
	if (context->curchar == '(')
	{
		m_nextch(context, 11);
		while (context->curchar != EOF)
		{
			if (letter(context))
			{
				from_macrotext(context);
				for (i = 0; i < context->msp; i++)
				{
					context->fchange[cp++] = context->mstring[i];
				}
			}
			else if (context->curchar == '(')
			{
				cp = scob(context, cp);
			}
			else if (context->curchar != ')' && context->curchar != ',')
			{
				context->fchange[cp++] = context->curchar;
				m_nextch(context, 11);
			}
			else
			{
				m_error(context, not_enough_param);
			}

			if (context->curchar == ',' || context->curchar == ')')
			{
				for (; context->functionident[r] != 0; r++)
				{
					;
				}

				if (r < context->functionident[r1])
				{
					context->fchange[cp++] = '\n';
					r++;
				}
				else
				{
					m_error(context, not_enough_param);
				}

				if (context->curchar == ',')
				{
					context->functionident[r] = cp;
					m_nextch(context, 11);
				}
				else
				{
					if (r != context->functionident[r1])
					{
						m_error(context, not_enough_param2);
					}
					m_nextch(context, 11);
					return;
				}
			}
		}
		m_error(context, scob_not_clous);
	}
	else
	{
		m_error(context, stalpe);
	}
}

static void r_macrofunction(compiler_context *context)
{
	int j;
	int olderfip = context->fip++;

	context->functionident[context->fip++] = context->mfp;

	to_functionident(context);
	m_nextch(context, 3);
	context->functionident[olderfip] = context->fip;

	while (context->curchar != '\n')
	{
		if (letter(context))
		{
			from_macrotext(context);
			for (j = 0; j < context->msp; j++)
			{
				context->macrofunction[context->mfp++] = context->mstring[j];
			}
			context->msp = 0;
		}
		else
		{
			context->macrofunction[context->mfp++] = context->curchar;
			m_nextch(context, 3);
		}

		if (context->curchar == EOF)
		{
			m_error(context, not_end_fail_preprocess);
		}
	}
	context->macrofunction[context->mfp++] = '\n';
	return;
}

static int check_if(compiler_context *context, int type_if)
{
	int flag = 0;

	if (type_if == SH_IF)
	{
		m_error(context, not_relis_if);
	}
	else if (type_if == SH_IFDEF || type_if == SH_IFNDEF)
	{
		context->msp = 0;
		while (letter(context) || digit(context))
		{
			context->mstring[context->msp++] = context->curchar;
			m_nextch(context, 10);
		}

		if (find_ident(context))
		{
			flag = 1;
		}
		if (type_if == SH_IFDEF)
		{
			return flag;
		}
		else
		{
			return 1 - flag;
		}
	}
	return 0;
}

static void false_if(compiler_context *context)
{
	int fl_cur;

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			fl_cur = macro_keywords(context);
			if (fl_cur == SH_ENDIF)
			{
				context->checkif--;
				if (context->checkif == -1)
				{
					m_error(context, befor_endif);
				}
				return;
			}
			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				context->checkif++;
				false_if(context);
			}
		}
		else
		{
			m_nextch(context, 8);
		}
	}
	m_error(context, must_be_endif);
}

static int m_false(compiler_context *context)
{
	int fl_cur = context->cur;

	while (context->curchar != EOF)
	{
		if (context->curchar == '#')
		{
			fl_cur = macro_keywords(context);
			if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
			{
				return fl_cur;
			}
			if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
			{
				false_if(context);
			}
		}
		else
		{
			m_nextch(context, 7);
		}
	}
	m_error(context, must_be_endif);
	return 1;
}

static void m_true(compiler_context *context, int type_if)
{
	while (context->curchar != EOF)
	{
		macroscan(context);
		if (context->cur == SH_ELSE || context->cur == SH_ELIF)
		{
			break;
		}
		if (context->cur == SH_ENDIF)
		{
			context->checkif--;
			if (context->checkif == -1)
			{
				m_error(context, befor_endif);
			}
			return;
		}
	}

	if (type_if != SH_IF && context->cur == SH_ELIF)
	{
		m_error(context, dont_elif);
	}

	false_if(context);
	return;
}

static void m_if(compiler_context *context, int type_if)
{
	int flag;

	context->checkif++;
	flag = check_if(context, type_if); // начало (if)
	end_line_space(context);
	if (flag)
	{
		m_true(context, type_if);
		return;
	}
	else
	{
		context->cur = m_false(context);
	}

	/*if (type_if == SH_IF) // середина (else if)
	{
		while (context->cur == SH_ELIF)
		{
			flag = check_if(type_if);
			end_line_space(context);
			if(flag)
			{
				m_true(context, type_if);
				return;
			}
			else
			{
				context->cur = m_folse();
			}

		}
	}
	else if (context->cur == SH_ELIF)
	{
		printf("Неправильное макрослово\n");
		exit (10);
	}*/

	if (context->cur == SH_ELSE) // конец (else)
	{
		context->cur = 0;
		m_true(context, type_if);
		return;
	}

	if (context->cur == SH_ENDIF)
	{
		context->checkif--;
		if (context->checkif == -1)
		{
			m_error(context, befor_endif);
		}
	}
}

static void macroscan(compiler_context *context)
{
	int j;

	switch (context->curchar)
	{
		case EOF:
			return;

		case '#':
			context->cur = macro_keywords(context);
			context->prep_flag = 1;

			if (context->cur == SH_DEFINE)
			{
				relis_define(context);
				m_nextch(context, 1);
				return;
			}
			else if (context->cur == SH_IF || context->cur == SH_IFDEF || context->cur == SH_IFNDEF)
			{
				m_if(context, context->cur);
				return;
			}
			else if (context->cur == SH_ELSE || context->cur == SH_ELIF || context->cur == SH_ENDIF)
			{
				return;
			}
			else
			{
				m_fprintf(context, context->curchar);
				m_nextch(context, 17);
				return;
				m_error(context, preproces_words_not_exist);
			}

		case '\'':
			m_fprintf(context, context->curchar);
			m_nextch(context, 17);
			if (context->curchar == '\\')
			{
				m_fprintf(context, context->curchar);
				m_nextch(context, 17);
			}
			m_fprintf(context, context->curchar);
			m_nextch(context, 17);

			m_fprintf(context, context->curchar);
			m_nextch(context, 17);
			return;

		case '\"':
			m_fprintf(context, context->curchar);
			m_nextch(context, 17);
			while (context->curchar != '\"' && context->curchar != EOF)
			{
				if (context->curchar == '\\')
				{
					m_fprintf(context, context->curchar);
					m_nextch(context, 17);
				}
				m_fprintf(context, context->curchar);
				m_nextch(context, 17);
			}
			m_fprintf(context, context->curchar);
			m_nextch(context, 17);
			return;
		default:
			if (letter(context) && context->prep_flag == 1)
			{
				from_macrotext(context);
				for (j = 0; j < context->msp; j++)
				{
					m_fprintf(context, context->mstring[j]);
				}
				return;
			}
			else
			{
				m_fprintf(context, context->curchar);
				m_nextch(context, 17);
				return;
			}
	}
}

void preprocess_file(compiler_context *context)
{
	context->mfirstrp = REPRTAB_LEN;
	context->mlines[context->mline = 1] = 1;
	context->charnum = 1;
	context->mcl = 1;

	getnext(context);
	m_nextch(context, 18);
	while (context->curchar != EOF)
	{
		macroscan(context);
	}
	context->m_conect_lines[context->mcl++] = context->mline - 1;
}
