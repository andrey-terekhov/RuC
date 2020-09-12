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

#include "preprocessor.h"
#include "calculator.h"
#include "compiler.h"
#include "constants.h"
#include "context.h"
#include "context_var.h"
#include "define.h"
#include "file.h"
#include "frontend_utils.h"
#include "if.h"
#include "include.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include "while.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void to_reprtab(char str[], int num, preprocess_context *context)
{
	int i;
	int oldrepr = context->rp;
	int hash = 0;
	unsigned char firstchar;
	unsigned char secondchar;
	int p;
	int c = 0;
	context->rp += 2;

	for (i = 0; str[i] != 0; i++)
	{
		sscanf(&str[i], "%c%n", &firstchar, &p);

		if ((firstchar & /*0b11100000*/ 0xE0) == /*0b11000000*/ 0xC0)
		{
			++i;
			sscanf(&str[i], "%c%n", &secondchar, &p);
			c = ((int)(firstchar & /*0b11111*/ 0x1F)) << 6 | (secondchar & /*0b111111*/ 0x3F);
		}
		else
		{
			c = firstchar;
		}

		hash += c;
		context->reprtab[context->rp++] = c;
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
	to_reprtab_full("MAIN", "main", "ГЛАВНАЯ", "главная", SH_MAIN, context);
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
	to_reprtab_full("#INCLUDE", "#include", "#ДОБАВИТЬ", "#добавить", SH_INCLUDE, context);
}

void output_keywods(preprocess_context *context)
{
	for (int j = 0; j < context->reprtab[context->rp]; j++)
	{
		m_fprintf(context->reprtab[context->rp + 2 + j], context);
	}
}

void preprocess_words(preprocess_context *context)
{
	/*if (context->curchar != '(')
	{
		m_nextch(context);
	}*/

	space_skip(context);
	switch (context->cur)
	{
		case SH_INCLUDE:
		{
			
			if (context->curchar != '\"')
			{
				m_nextch(context);
			}

			if (!context->h_flag)
			{
				include_relis(context, context->sources);
			}
			else
			{
				include_relis(context, context->headers);
			}

			return;
		}
		case SH_DEFINE:
		case SH_MACRO:
		{
			define_relis(context);
			return;
		}
		case SH_UNDEF:
		{
			int k;
			context->macrotext[context->reprtab[(k = collect_mident(context)) + 1]] = MACROUNDEF;
			space_end_line(context);
			return;
		}
		case SH_IF:
		case SH_IFDEF:
		case SH_IFNDEF:
		{
			if_relis(context);
			return;
		}
		case SH_SET:
		{
			set_relis(context);
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
				calculator(0, context);
			}
			else
			{
				m_error(after_eval_must_be_ckob, context);
			}

			m_change_nextch_type(CTYPE, 0, context);
			return;
		}
		case SH_WHILE:
		{
			context->wsp = 0;
			context->ifsp = 0;
			while_collect(context);
			m_change_nextch_type(WHILETYPE, 0, context);
			m_nextch(context);
			m_nextch(context);

			context->nextp = 0;
			while_relis(context);

			return;
		}
		default:
		{
			//m_nextch(context);
			output_keywods(context);
			return;
		}
	}
}

void preprocess_scan(preprocess_context *context)
{
	int i;

	switch (context->curchar)
	{
		case EOF:
			return;

		case '#':
		{
			context->cur = macro_keywords(context);
		
			if(context->cur != 0)
			{
				context->prep_flag = 1;
				preprocess_words(context);
			}
			else
			{
				//m_nextch(context);
				output_keywods(context);
			}

			return;
		}
		case '\'':
		case '\"':
		{
			space_skip_str(context);
			return;
		}
		case '@':
		{
			m_nextch(context);
			return;
		}
		default:
		{
			if (is_letter(context) && context->prep_flag == 1)
			{
				int r = collect_mident(context);

				if (r)
				{
					define_get_from_macrotext(r, context);
				}
				else
				{
					for (i = 0; i < context->msp; i++)
					{
						m_fprintf(context->mstring[i], context);
					}
				}
			}
			else
			{
				m_fprintf(context->curchar, context);
				m_nextch(context);
			}
		}
	}
}

void swap(data_file *f1, data_file *f2)
{
	macro_long_string temp_s = f1->before_source;
	f1->before_source = f2->before_source;
	f2->before_source = temp_s;

	macro_long_string temp_include_source = f1->include_source;
	f1->include_source = f2->include_source;
	f2->include_source = temp_include_source;

	int temp_pred = f1->pred;
	f1->pred = f2->pred;
	f2->pred = temp_pred;

	const char *temp_name = f1->name;
	f1->name = f2->name;
	f2->name = temp_name;
}

void add_c_file_siple(preprocess_context *context)
{
	include_source_set(context->sources, context->before_temp_p, context->before_temp->p);
	context->before_temp = &context->sources->files[context->sources->cur].before_source;
	context->temp_output = 0;
	context->sources->files[context->sources->cur].include_line = context->line - 1;
	context->sources->files[context->sources->cur].cs.p = 0;
	context->control_aflag = 0;
	context->control_bflag = 0;

	while (context->curchar != EOF && context->main_file == -1)
	{
		if (context->curchar == 'm' || context->curchar == 'M' || context->curchar == 0x413 ||
			context->curchar == 0x433)
		{
			context->cur = macro_keywords(context);
			if (context->cur == SH_MAIN)
			{
				context->main_file = context->sources->cur;
			}
		}
		m_nextch(context);
	}

	while (context->curchar != EOF)
	{
		m_nextch(context);
	}
}

void add_c_file(preprocess_context *context)
{
	m_nextch(context);
	while (context->curchar != EOF)
	{
		switch (context->curchar)
		{
			case EOF:
			{
				add_c_file_siple(context);
				return;
			}
				return;
			case ' ':
			case '\n':
			case '\t':
			{
				m_nextch(context);
				break;
			}
			case '#':
			{
				if (context->nextchar == 'i' || context->nextchar == 'I' || context->nextchar == 0x414 ||
					context->nextchar == 0x434)
				{
					context->before_temp_p = (context->before_temp)->p;
					context->cur = macro_keywords(context);
					if (context->cur == SH_INCLUDE)
					{
						include_relis(context, context->sources);
						if (context->h_flag)
						{
							context->h_flag = 0;
							add_c_file_siple(context);
							return;
						}
						break;
					}
					else
					{
						add_c_file_siple(context);
						return;
					}
				}
				else
				{
					add_c_file_siple(context);
					return;
				}
			}
			default:
			{
				add_c_file_siple(context);
				return;
			}
		}
	}
}

void open_files(preprocess_context *context, int number, const char *codes[])
{
	context->include_ways = malloc(number * sizeof(char *));

	const char **ways = context->include_ways;
	int *iwp = &context->iwp;

	for (int i = 0; i < number; i++)
	{
		if (codes[i][0] == '-' && codes[i][1] == 'I')
		{
			ways[*iwp] = &codes[i][2];

			/*int length = strlen(ways[*iwp]);
			if (ways[*iwp][length - 1] == '/')
			{
				ways[*iwp][length - 1] = '\0';
			}*/

			// printf("\n include_ways[i] = %s\n", ways[*iwp]);
			// printf("\n include_ways[i] = %s\n", context->include_ways[*iwp]);
			context->iwp++;
		}
	}


	data_files_init(context->sources, number);
	data_files_init(context->headers, number);

	for (int i = 0; i < number; i++)
	{
		if (codes[i][0] == '-' && codes[i][1] == 'I')
		{
			continue;
		}

		if (find_file(context, codes[i]))
		{
			int old_cur = open_p_faile(context, codes[i]);
			cur_failes_next(context->sources, old_cur, context);
			context->before_temp = &context->sources->files[context->sources->cur].include_source;
			context->before_temp->p = 0;

			get_next_char(context);

			if (context->nextchar == EOF)
			{
				context->sources->files[context->sources->cur].before_source.str[
					context->sources->files[context->sources->cur].before_source.p++] = EOF;
			}
			else
			{
				add_c_file(context);
			}

			include_fclose(context);
			set_old_cur(context->sources, old_cur, context);
		}
	}
}

void preprocess_h_file(preprocess_context *context)
{
	data_files *fs = context->headers;
	fs->cur++;

	context->h_flag = 1;
	context->include_type = 1;

	while (fs->cur < fs->p - fs->i)
	{
		context->current_file = fs->files[fs->cur].input;
		context->before_temp = &fs->files[fs->cur].before_source;
		context->before_temp->p = 0;
		context->temp_output = 0;
		file_read(context);
		fs->cur++;
	}

	context->h_flag = 0;
	context->FILE_flag = 0;
}

void preprocess_c_file(preprocess_context *context)
{
	data_files *fs = context->sources;
	fs->cur = 0;

	if (context->main_file != fs->p - 1 && context->main_file != -1)
	{
		swap(&fs->files[context->main_file], &fs->files[fs->p - 1]);
	}

	while (fs->cur < fs->p - fs->i)
	{
		context->current_string = fs->files[fs->cur].before_source.str;
		context->current_p = fs->files[fs->cur].before_source.p;

		file_read(context);
		fs->cur++;
	}
}

char *preprocess_file(int argc, const char *argv[], data_files *sources, data_files *headers)
{
	preprocess_context context;
	preprocess_context_init(&context, sources, headers);
	printer_attach_buffer(&context.output_options, 1024);

	add_keywods(&context);

	context.mfirstrp = context.rp;

	open_files(&context, argc, argv);

	preprocess_h_file(&context);

	preprocess_c_file(&context);

	context.before_temp = NULL;
	free(context.include_ways);

	char *macro_processed = context.output_options.ptr;

#if MACRODEBUG
	printf("\n\n");
	printf("Текст после препроцессирования:\n>\n%s<\n", macro_processed);
#endif
	return macro_processed;
}

/*
	printf("cur = %d, %c; next = %d, %c;\n",context->curchar, context->curchar, context->nextchar, context->nextchar);

	for (int k = 0; k < fsp; k++)
	{
		printf("str[%d] = %d,%c.\n", k, fstring[k], fstring[k]);
	}

	printf("!!!!!!!!!!!!!!1\n");

	for (int k = 0; k < context->mp; k++)
	{
		printf("context->macrotext[%d] = %d,%c.\n", k, context->macrotext[k], context->macrotext[k]);
	}
	for (int k = context->mfirstrp; k < context->mfirstrp + 20; k++)
	{
		printf("str[%d] = %d,%c.\n", k, context->reprtab[k], context->reprtab[k]);
	}
	for (int k = 0; k < cp; k++)
	{
		printf(" fchange[%d] = %d,%c.\n", k, fchange[k], fchange[k]);
	}

	/Egor/test_eval1.c
	/Egor/calculator/test1.c
	/Fadeev/import.c
	/Egor/Macro/includ/cofig.txt
*/
