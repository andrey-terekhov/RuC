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

#include "define.h"
#include "calculator.h"
#include "constants.h"
#include "context.h"
#include "context_var.h"
#include "file.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void define_get_from_macrotext(int r, preprocess_context *context);


int m_equal(preprocess_context *context)
{
	int i = 0;
	int n = 1;
	int j = 0;

	while (j < context->csp)
	{
		while (context->mstring[i++] == context->cstring[j++])
		{
			if (context->mstring[i] == MACROEND && context->cstring[j] == 0)
			{
				return n;
			}
		}

		n++;
		i = 0;
		if (context->cstring[j++] != 0)
		{
			while (context->cstring[j++] != 0)
			{
				;
			}
		}
	}

	return 0;
}

// define c параметрами (function)
void function_scob_collect(int t, int num, preprocess_context *context)
{
	int i;

	while (context->curchar != EOF)
	{
		if (is_letter(context))
		{
			int r = collect_mident(context);

			if (r)
			{
				int oldcp1 = context->cp;
				int oldlsp = context->lsp;
				int locfchange[STRIGSIZE];
				int lcp = 0;
				int ldip;

				context->lsp += num;
				define_get_from_macrotext(r, context);
				ldip = get_dipp(context);

				if (context->nextch_type == FTYPE)
				{
					ldip--;
				}

				while (get_dipp(context) >= ldip) // 1 переход потому что есть префиксная замена
				{
					locfchange[lcp++] = context->curchar;
					m_nextch(context);
				}

				context->lsp = oldlsp;
				context->cp = oldcp1;

				for (i = 0; i < lcp; i++)
				{
					context->fchange[context->cp++] = locfchange[i];
				}
			}
			else
			{
				for (i = 0; i < context->msp; i++)
				{
					context->fchange[context->cp++] = context->mstring[i];
				}
			}
		}
		else if (context->curchar == '(')
		{
			context->fchange[context->cp++] = context->curchar;
			m_nextch(context);
			function_scob_collect(0, num, context);
		}
		else if (context->curchar == ')' || (t == 1 && context->curchar == ','))
		{
			if (t == 0)
			{
				context->fchange[context->cp++] = context->curchar;
				m_nextch(context);
			}

			return;
		}
		else if (context->curchar == '#')
		{
			if (macro_keywords(context) == SH_EVAL && context->curchar == '(')
			{
				calculator(0, context);
				for (i = 0; i < context->csp; i++)
				{
					context->fchange[context->cp++] = context->cstring[i];
				}
			}
			else
			{
				for (i = 0; i < context->reprtab[context->rp]; i++)
				{
					context->fchange[context->cp++] = context->reprtab[context->rp + 2 + i];
				}
			}
		}
		else
		{
			context->fchange[context->cp++] = context->curchar;
			m_nextch(context);
		}
	}
	m_error(scob_not_clous, context);
}

void function_stack_create(int n, preprocess_context *context)
{
	int num = 0;

	m_nextch(context);
	// printf("function_stack_create n = %d\n", n);
	context->localstack[num + context->lsp] = context->cp;

	if (context->curchar == ')')
	{
		m_error(stalpe, context);
	}

	while (context->curchar != ')')
	{
		function_scob_collect(1, num, context);
		context->fchange[context->cp++] = CANGEEND;

		if (context->curchar == ',')
		{
			num++;
			context->localstack[num + context->lsp] = context->cp;

			if (num > n)
			{
				m_error(not_enough_param, context);
			}
			m_nextch(context);

			if (context->curchar == ' ')
			{
				m_nextch(context);
			}
		}
		else if (context->curchar == ')')
		{
			if (num != n)
			{
				m_error(not_enough_param2, context);
			}
			m_nextch(context);

			context->cp = context->localstack[context->lsp];
			return;
		}
	}

	m_error(scob_not_clous, context);
}

void funktionleter(int flag_macro, preprocess_context *context)
{
	int n = 0;
	int i = 0;

	context->msp = 0;

	int r = collect_mident(context);

	// printf("funktionleter\n");

	if ((n = m_equal(context)) != 0)
	{
		context->macrotext[context->mp++] = MACROCANGE;
		context->macrotext[context->mp++] = n - 1;
	}
	else if (!flag_macro && r)
	{
		define_get_from_macrotext(r, context);
	}
	else
	{
		for (i = 0; i < context->msp; i++)
		{
			context->macrotext[context->mp++] = context->mstring[i];
		}
	}
}

int to_functionident(preprocess_context *context)
{
	int num = 0;
	context->csp = 0;

	// printf("to_functionident\n");

	while (context->curchar != ')')
	{
		context->msp = 0;

		if (is_letter(context))
		{
			while (is_letter(context) || is_digit(context->curchar))
			{
				context->cstring[context->csp++] = context->curchar;
				m_nextch(context);
			}
			context->cstring[context->csp++] = 0;
		}
		else
		{
			m_error(functionid_begins_with_letters, context);
		}

		context->msp = 0;
		if (context->curchar == ',')
		{
			m_nextch(context);
			space_skip(context);
			num++;
		}
		else if (context->curchar != ')')
		{
			m_error(after_functionid_must_be_comma, context);
		}
	}

	// printf("-to_functionident = %d\n", num);
	m_nextch(context);
	return num;
}

void function_add_to_macrotext(preprocess_context *context)
{
	int j;
	int flag_macro = 0;
	int empty = 0;

	// printf("function_add_to_macrotext\n");

	if (context->cur == SH_MACRO)
	{
		flag_macro = 1;
	}

	context->macrotext[context->mp++] = MACROFUNCTION;

	if (context->curchar == ')')
	{
		context->macrotext[context->mp++] = -1;
		empty = 1;
		m_nextch(context);
	}
	else
	{
		context->macrotext[context->mp++] = to_functionident(context);
	}
	space_skip(context);

	while (context->curchar != '\n' || flag_macro && context->curchar != EOF)
	{
		if (is_letter(context) && !empty)
		{
			funktionleter(flag_macro, context);
		}
		else if (context->curchar == '#')
		{
			context->cur = macro_keywords(context);

			if (!flag_macro && context->cur == SH_EVAL && context->curchar == '(')
			{
				calculator(0, context);
				for (j = 0; j < context->csp; j++)
				{
					context->macrotext[context->mp++] = context->cstring[j];
				}
			}
			else if (flag_macro && context->cur == SH_ENDM)
			{
				m_nextch(context);
				context->macrotext[context->mp++] = MACROEND;
				return;
			}
			else
			{
				context->cur = 0;
				for (j = 0; j < context->reprtab[context->rp]; j++)
				{
					context->macrotext[context->mp++] = context->reprtab[context->rp + 2 + j];
				}
			}
		}
		else
		{
			context->macrotext[context->mp++] = context->curchar;
			m_nextch(context);
		}

		if (context->curchar == EOF)
		{
			m_error(not_end_fail_define, context);
		}

		if (context->curchar == '\\')
		{
			m_nextch(context);
			space_end_line(context);
		}
	}

	context->macrotext[context->mp++] = MACROEND;
}
//

// define
void define_get_from_macrotext(int r, preprocess_context *context)
{
	int t = context->reprtab[r + 1];

	if (r)
	{
		context->msp = 0;
		if (context->macrotext[t] == MACROFUNCTION)
		{
			if (context->macrotext[++t] > -1)
			{
				function_stack_create(context->macrotext[t], context);
			}
		}

		// printf("--from_macrotext r = %d\n", t + 1);
		m_change_nextch_type(TEXTTYPE, t + 1, context);
		m_nextch(context);
	}
	else
	{
		m_error(ident_not_exist, context);
	}
}

int define_add_to_reprtab(preprocess_context *context)
{
	int r;
	int oldrepr = context->rp;
	int hash = 0;
	context->rp += 2;

	do
	{
		hash += context->curchar;
		context->reprtab[context->rp++] = context->curchar;
		m_nextch(context);
	} while (is_letter(context) || is_digit(context->curchar));

	hash &= 255;
	context->reprtab[context->rp++] = 0;
	r = context->hashtab[hash];

	while (r)
	{
		if (equal_reprtab(r, oldrepr, context))
		{
			if (context->macrotext[context->reprtab[r + 1]] == MACROUNDEF)
			{
				context->rp = oldrepr;
				return r;
			}
			else
			{
				m_error(repeat_ident, context);
			}
		}
		r = context->reprtab[r];
	}

	context->reprtab[oldrepr] = context->hashtab[hash];
	context->reprtab[oldrepr + 1] = context->mp;
	context->hashtab[hash] = oldrepr;
	return 0;
}

void define_add_to_macrotext(int r, preprocess_context *context)
{
	int j;
	int lmp = context->mp;

	context->macrotext[context->mp++] = MACRODEF;
	if(context->curchar != '\n')
	{
		while (context->curchar != '\n')
		{
			if (context->curchar == EOF)
			{
				m_error(not_end_fail_define, context);
			}
			else if (context->curchar == '#')
			{
				context->cur = macro_keywords(context);
				if (context->cur == SH_EVAL)
				{
					if (context->curchar != '(')
					{
						m_error(after_eval_must_be_ckob, context);
					}

					calculator(0, context);

					for (j = 0; j < context->csp; j++)
					{
						context->macrotext[context->mp++] = context->cstring[j];
					}
				}
				else
				{
					for (j = 0; j < context->reprtab[context->rp]; j++)
					{
						context->macrotext[context->mp++] = context->reprtab[context->rp + 2 + j];
					}
				}
			}
			else if (context->curchar == '\\')
			{
				m_nextch(context);
				space_end_line(context);
			}
			else if (is_letter(context))
			{
				int k = collect_mident(context);
				if (k)
				{
					define_get_from_macrotext(k, context);
				}
				else
				{
					for (j = 0; j < context->msp; j++)
					{
						context->macrotext[context->mp++] = context->mstring[j];
					}
				}
			}
			else
			{
				context->macrotext[context->mp++] = context->curchar;
				m_nextch(context);
			}
		}

		while (context->macrotext[context->mp - 1] == ' ' || context->macrotext[context->mp - 1] == '\t')
		{
			context->macrotext[context->mp - 1] = MACROEND;
			context->mp--;
		}
	}
	else
	{
		context->macrotext[context->mp++] = '0';
	}
	
	context->macrotext[context->mp++] = MACROEND;

	if (r)
	{
		context->reprtab[r + 1] = lmp;
	}
}

void define_relis(preprocess_context *context)
{
	int r;

	if (!is_letter(context))
	{
		m_error(ident_begins_with_letters1, context);
	}

	r = define_add_to_reprtab(context);

	context->msp = 0;

	if (context->curchar == '(' && !r)
	{
		m_nextch(context);
		function_add_to_macrotext(context);
	}
	else if (context->curchar != ' ' && context->curchar != '\n' && context->curchar != '\t')
	{
		m_error(after_ident_must_be_space1, context);
	}
	else
	{
		space_skip(context);
		define_add_to_macrotext(r, context);
	}
	m_nextch(context);
}

void set_relis(preprocess_context *context)
{
	int j;

	space_skip(context);

	if (!is_letter(context))
	{
		m_error(ident_begins_with_letters1, context);
	}

	j = collect_mident(context);

	if (context->macrotext[context->reprtab[j + 1]] == MACROFUNCTION)
	{
		m_error(functions_cannot_be_changed, context);
	}
	else if (context->curchar != ' ')
	{
		m_error(after_ident_must_be_space1, context);
	}

	m_nextch(context);
	space_skip(context);

	define_add_to_macrotext(j, context);
}
//