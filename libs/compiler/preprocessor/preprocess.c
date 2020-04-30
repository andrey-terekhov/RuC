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
#include <wchar.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "while.h"
#include "context_var.h"
#include "constants.h"
#include "preprocessor_error.h"
#include "file.h"
#include "errors.h"
#include "define.h"
#include "if.h"
#include "calculator.h"
#include "file.h"
#include "preprocessor_utils.h"
#include "include.h"
#include "context.h"

#define MACRODEBAG 0


void to_reprtab(char str[], int num, preprocess_context *context)
{
	int i;
	int oldrepr = context->rp;
	int hash = 0;
	context->rp += 2;

	for (i = 0; str[i] != 0; i++)
	{
		hash += str[i];
		context->reprtab[context->rp++] = str[i];
	}

	hash &= 255;
	context->reprtab[context->rp++] = 0;
	context->reprtab[oldrepr] = context->hashtab[hash];
	context->reprtab[oldrepr + 1] = num;
	context->hashtab[hash] = oldrepr;
}

void to_reprtab_full(char str1[], char str2[], char str3[], char str4[], int num, preprocess_context *context)
{
	to_reprtab(str1, num, context);
	to_reprtab(str2, num, context);
	to_reprtab(str3, num, context);
	to_reprtab(str4, num, context);
}

void add_keywods(preprocess_context *context)
{
	to_reprtab_full("#DEFINE", "#define", "#ОПРЕД", "#опред", SH_DEFINE, context);
	to_reprtab_full("#IFDEF", "#ifdef", "#ЕСЛИБЫЛ", "#еслибыл", SH_IFDEF, context);
	to_reprtab_full("#IFNDEF", "#ifndef", "#ЕСЛИНЕБЫЛ", "#еслинебыл", SH_IFNDEF, context);
	to_reprtab_full("#IF", "#if", "#ЕСЛИ", "#если", SH_IF, context);
	to_reprtab_full("#ELIF", "#elif", "#ИНЕСЛИ", "#инесли", SH_ELIF, context);
	to_reprtab_full("#ENDIF", "#endif", "#КОНЕЦЕСЛИ", "#конецесли", SH_ENDIF, context);
	to_reprtab_full("#ELSE", "#else", "#ИНАЧЕ", "#иначе", SH_ELSE, context);
	to_reprtab_full("#MACRO", "#macro", "#МАКРО", "#макро", SH_MACRO, context);
	to_reprtab_full("#ENDM", "#endm", "#КОНЕЦМ", "#конецм", SH_ENDM, context);
	to_reprtab_full("#WHILE", "#while", "#ПОКА", "#пока", SH_WHILE, context);
	to_reprtab_full("#ENDW", "#endw", "#КОНЕЦП", "#конецп", SH_ENDW, context);
	to_reprtab_full("#SET", "#set", "#ПЕРЕОПРЕД", "#переопред", SH_SET, context);
	to_reprtab_full("#UNDEF", "#undef", "#ОТМЕНАОПРЕД", "#отменаопред", SH_UNDEF, context);
	to_reprtab_full("#FOR", "#for", "#ДЛЯ", "#для", SH_FOR, context);
	to_reprtab_full("#ENDF", "#endf", "#КОНЕЦД", "#конецд", SH_ENDF, context);
	to_reprtab_full("#EVAL", "#eval", "#ВЫЧИСЛЕНИЕ", "#вычисление", SH_EVAL, context);
	to_reprtab_full("#INCLUDE", "#include", "#ДОБАВИТЬ", "#ДОБАВИТЬ", SH_INCLUDE, context);
}


void preprocess_words(preprocess_context *context, compiler_context *c_context)
{
	if (context->curchar != '(' && context->curchar != '\"')
	{
		m_nextch(context, c_context);
	}

	space_skip(context, c_context);

	switch (context->cur)
	{
		case SH_INCLUDE:
		{
			include_relis(context, c_context);
			return;
		}
		case SH_DEFINE:
		case SH_MACRO:
		{
			define_relis(context, c_context);
			return;
		}
		case SH_UNDEF:
		{
			int k;
			context->macrotext[context->reprtab[(k = collect_mident(context, c_context)) + 1]] = MACROUNDEF;
			space_end_line(context, c_context);
			return;
		}
		case SH_IF:
		case SH_IFDEF:
		case SH_IFNDEF:
		{
			if_relis(context, c_context);
			return;
		}
		case SH_SET:
		{
			set_relis(context, c_context);
			return;
		}
		case SH_ELSE:
		case SH_ELIF:
		case SH_ENDIF:
			return;
		case SH_EVAL:
		{
			if (context->curchar == '(')
			{
				calculator(0, context, c_context);
			}
			else
			{
				m_error(after_eval_must_be_ckob, c_context);
			}

			m_change_nextch_type(CTYPE, 0, context, c_context);
			return;
		}
		case SH_WHILE:
		{
			context->wsp = 0;
			context->ifsp = 0;
			while_collect(context, c_context);
			m_change_nextch_type(WHILETYPE, 0, context, c_context);

			context->nextp = 0;
			while_relis(context, c_context);

			return;
		}
		default:
		{
			m_nextch(context, c_context);
			for (int j = 0; j < context->reprtab[context->rp]; j++)
			{
				m_fprintf(context->reprtab[context->rp + 2 + j], context, c_context);
			}

			return;
		}
	}
}

void preprocess_scan(preprocess_context *context, compiler_context *c_context)
{
	int i;

	switch (context->curchar)
	{
		case EOF:
			return;

		case '#':
		{
			context->cur = macro_keywords(context, c_context);
			c_context->prep_flag = 1;
			preprocess_words(context, c_context);

			return;
		}
		case '\'':
		case '\"':
		{
			space_skip_str(context, c_context);
			return;
		}
		case '@':
		{
			m_nextch(context, c_context);			
			return;
		}
		default:
		{
			if (is_letter(context) && c_context->prep_flag == 1)
			{
				int r = collect_mident(context, c_context);

				if (r)
				{
					define_get_from_macrotext(r, context, c_context);
				}
				else
				{
					for (i = 0; i < context->msp; i++)
					{
						m_fprintf(context->mstring[i], context, c_context);
					}
				}
			}
			else
			{	
				m_fprintf(context->curchar, context, c_context);
				m_nextch(context, c_context);
			}
		}
	}
}

void open_main_file(const char *code, preprocess_context *context)
{
	int i = 0;
	while (code[i] != '.' || code[i + 1] != 'c')
	{
		context->way[i] = code[i];
		i++;
	}
	context->way[i] = code[i];
	i++;
	context->way[i] = code[i];
	i++;
	context->way[i] = '\0';
	i++;
	
	context->input_stak[context->inp_p++] = fopen(context->way, "r");			// исходный текст
	//context->input_stak[context->inp_p] = fopen("../../tests/00test.c", "r");
	if (context->input_stak[context->inp_file] == NULL)
	{
		printf(" не найден файл %s\n", context->way);
		exit(1);
	}
	

	while (code[i] != '/')
	{
		i--;
	}
	context->way[i+1] = '\0';
}

void preprocess_file(compiler_context *c_context, const char *code)
{
	c_context->mlines[c_context->mline = 1] = 1;
	c_context->charnum = 1;
	preprocess_context *context = malloc(sizeof(preprocess_context));
	preprocess_context_init(context);
	open_main_file(code, context);
	for(int i = 0; i < HASH; i++)
	{
		context->hashtab[i] = 0;
	}

	add_keywods(context);
	context->mfirstrp = context->rp;
	get_next_char(context);
	m_nextch(context, c_context);
	while (context->curchar != EOF)
	{
		preprocess_scan(context, c_context);
	}
	c_context->m_conect_lines[context->mclp++] = c_context->mline - 1;

	if(MACRODEBAG)
	{
		char* macro_processed = strdup(c_context->output_options.ptr);
		printf("%s\n", macro_processed);
	}
}

/*
	for (int k = 0; k < fsp; k++)
	{
		printf("str[%d] = %d,%c.\n", k, fstring[k], fstring[k]);
	}

	printf("1\n");

	for (int k = 0; k < mp; k++)
	{
		printf("macrotext[%d] = %d,%c.\n", k, macrotext[k], macrotext[k]);
	}
	for (int k = 0; k < cp; k++)
	{
		printf("localstack[%d] = %d,%c.\n", k, localstack[k], localstack[k]);
	}
	for (int k = 0; k < cp; k++)
	{
		printf(" fchange[%d] = %d,%c.\n", k, fchange[k], fchange[k]);
	}
*/
