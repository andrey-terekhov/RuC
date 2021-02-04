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
#include "context_var.h"
#include "file.h"
#include "error.h"
#include "utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int define_get_from_macrotext(int r, preprocess_context *context);


int m_equal(preprocess_context *context)
{
	int i = 0;
	int n = 1;
	int j = 0;

	while (j < context->csp)
	{
		while (context->mstring[i] == context->cstring[j])
		{
			i++;
			j++;
			if (context->mstring[i] == MACROEND && context->cstring[j] == 0)
			{
				return n;
			}
		}
		i++;
		j++;

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
int function_scob_collect(int t, int num, preprocess_context *context)
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
				int locfchange[STRING_SIZE];
				int lcp = 0;
				int ldip;

				context->lsp += num;
				if(define_get_from_macrotext(r, context))
				{
					return -1;
				}
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
			
			if(function_scob_collect(0, num, context))
			{
				return -1;
			}
		}
		else if (context->curchar == ')' || (t == 1 && context->curchar == ','))
		{
			if (t == 0)
			{
				context->fchange[context->cp++] = context->curchar;
				m_nextch(context);
			}

			return 0;
		}
		else if (context->curchar == '#')
		{
			if (macro_keywords(context) == SH_EVAL && context->curchar == '(')
			{	
				if(calculator(0, context))
				{
					return -1;
				}
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
	size_t position = skip_str(context); 
	macro_error(scob_not_clous, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
	return -1;
}

int function_stack_create(int n, preprocess_context *context)
{
	int num = 0;

	m_nextch(context);
	context->localstack[num + context->lsp] = context->cp;

	if (context->curchar == ')')
	{
		size_t position = skip_str(context); 
		macro_error(stalpe, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}

	while (context->curchar != ')')
	{
		if(function_scob_collect(1, num, context))
		{
			return -1;
		}
		context->fchange[context->cp++] = CANGEEND;

		if (context->curchar == ',')
		{
			num++;
			context->localstack[num + context->lsp] = context->cp;

			if (num > n)
			{
				size_t position = skip_str(context); 
				macro_error(not_enough_param, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
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
				size_t position = skip_str(context); 
				macro_error(not_enough_param2, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
			m_nextch(context);

			context->cp = context->localstack[context->lsp];
			return 0;
		}
	}

	size_t position = skip_str(context); 
	macro_error(scob_not_clous, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
	return -1;
}

int funktionleter(int flag_macro, preprocess_context *context)
{
	int n = 0;
	int i = 0;

	context->msp = 0;

	int r = collect_mident(context);

	if ((n = m_equal(context)) != 0)
	{
		context->macrotext[context->mp++] = MACROCANGE;
		context->macrotext[context->mp++] = n - 1;
	}
	else if (!flag_macro && r)
	{
		if(define_get_from_macrotext(r, context))
		{
			return -1;
		}
	}
	else
	{
		for (i = 0; i < context->msp; i++)
		{
			context->macrotext[context->mp++] = context->mstring[i];
		}
	}
	return 0;
}

int to_functionident(preprocess_context *context)
{
	int num = 0;
	context->csp = 0;

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
			size_t position = skip_str(context); 
			macro_error(functionid_begins_with_letters, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			return -1;
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
			size_t position = skip_str(context); 
			macro_error(after_functionid_must_be_comma, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			return -1;
		}
	}
	m_nextch(context);
	return num;
}

int function_add_to_macrotext(preprocess_context *context)
{
	int j;
	int flag_macro = 0;
	int empty = 0;

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
		int res = to_functionident(context);
		if(res == -1)
		{
			return -1;
		}
		context->macrotext[context->mp++] = res;
	}
	space_skip(context);

	while ((context->curchar != '\n' || flag_macro) && context->curchar != EOF)
	{
		if (is_letter(context) && !empty)
		{
			if(funktionleter(flag_macro, context))
			{
				return -1;
			}
		}
		else if (context->curchar == '#')
		{
			context->cur = macro_keywords(context);

			if (!flag_macro && context->cur == SH_EVAL && context->curchar == '(')
			{
				if(calculator(0, context))
				{
					return -1;
				}
				for (j = 0; j < context->csp; j++)
				{
					context->macrotext[context->mp++] = context->cstring[j];
				}
			}
			else if (flag_macro && context->cur == SH_ENDM)
			{
				m_nextch(context);
				context->macrotext[context->mp++] = MACROEND;
				return 0;
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
			size_t position = skip_str(context); 
			macro_error(not_end_fail_define, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			return -1;
		}

		if (context->curchar == '\\')
		{
			m_nextch(context);
			space_end_line(context);
			if(space_end_line(context))
			{
				return -1;
			}
			//context->macrotext[context->mp++] = '\n';
			m_nextch(context);
		}
	}

	context->macrotext[context->mp++] = MACROEND;
	return 0;
}
//

// define
int define_get_from_macrotext(int r, preprocess_context *context)
{
	int t = context->reprtab[r + 1];

	if (r)
	{
		context->msp = 0;
		if (context->macrotext[t] == MACROFUNCTION)
		{
			if (context->macrotext[++t] > -1)
			{
				
				if(function_stack_create(context->macrotext[t], context))
				{
					return -1;
				}
			}
		}

		m_change_nextch_type(TEXTTYPE, t + 1, context);
		m_nextch(context);
	}
	else
	{
		size_t position = skip_str(context); 
		macro_error(ident_not_exist, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}

	return 0;
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
				size_t position = skip_str(context); 
				macro_error(repeat_ident, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
		}
		r = context->reprtab[r];
	}

	context->reprtab[oldrepr] = context->hashtab[hash];
	context->reprtab[oldrepr + 1] = context->mp;
	context->hashtab[hash] = oldrepr;
	return 0;
}

int define_add_to_macrotext(int r, preprocess_context *context)
{
	int j;
	int lmp = context->mp;

	context->macrotext[context->mp++] = MACRODEF;
	if (context->curchar != '\n')
	{
		while (context->curchar != '\n')
		{
			if (context->curchar == EOF)
			{
				size_t position = skip_str(context); 
				macro_error(not_end_fail_define, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
			else if (context->curchar == '#')
			{
				context->cur = macro_keywords(context);
				if (context->cur == SH_EVAL)
				{
					if (context->curchar != '(')
					{
						size_t position = skip_str(context); 
						macro_error(after_eval_must_be_ckob, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
						return -1;
					}

					if(calculator(0, context))
					{
						return -1;
					}

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
				if(space_end_line(context))
				{
					return -1;
				}
				//context->macrotext[context->mp++] = '\n';
				m_nextch(context);
			}
			else if (is_letter(context))
			{
				int k = collect_mident(context);
				if (k)
				{
					if(define_get_from_macrotext(k, context))
					{
						return -1;
					}
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
	return 0;
}

int define_relis(preprocess_context *context)
{
	int r;

	if (!is_letter(context))
	{
		size_t position = skip_str(context); 
		macro_error(ident_begins_with_letters, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}

	r = define_add_to_reprtab(context);
	if(r == -1)
	{
		return -1;
	}

	context->msp = 0;

	if (context->curchar == '(' && !r)
	{
		m_nextch(context);
		if(function_add_to_macrotext(context))
		{
			return -1;
		}
	}
	else if (context->curchar != ' ' && context->curchar != '\n' && context->curchar != '\t')
	{
		size_t position = skip_str(context); 
		macro_error(after_ident_must_be_space, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}
	else
	{
		space_skip(context);
		return define_add_to_macrotext(r, context);
	}
	return 0;
}

int set_relis(preprocess_context *context)
{
	int j;

	space_skip(context);

	if (!is_letter(context))
	{
		size_t position = skip_str(context); 
		macro_error(ident_begins_with_letters, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}

	j = collect_mident(context);

	if (context->macrotext[context->reprtab[j + 1]] == MACROFUNCTION)
	{
		size_t position = skip_str(context); 
		macro_error(functions_cannot_be_changed, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}
	else if (context->curchar != ' ' && context->curchar != '\t')
	{	
		size_t position = skip_str(context);
		macro_error(after_ident_must_be_space, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}

	m_nextch(context);
	space_skip(context);

	return define_add_to_macrotext(j, context);
}
//