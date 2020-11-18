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

#define H_FILE 1
#define C_FILE 0

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

			include_relis(context);

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
			// m_nextch(context);
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

			if (context->cur != 0)
			{
				context->prep_flag = 1;
				preprocess_words(context);
				if(context->curchar != '#')
				{
					con_file_print_coment(&context->fs, context);
				}
			}
			else
			{
				// m_nextch(context);
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

void add_c_file_siple(preprocess_context *context)
{
	context->temp_output = 0;

	while (context->curchar != EOF && context->fs.main_faile == -1)
	{
		context->cur = macro_keywords(context);
		if (context->cur == SH_MAIN)
		{
			con_file_it_is_main(&context->fs);
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
				return;
			}
			case ' ':
			case '\n':
			case '\t':
			{
				m_nextch(context);
				break;
			}
			case '#':
			{
				context->cur = macro_keywords(context);
				if (context->cur == SH_INCLUDE)
				{
					include_relis(context);
					break;
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

	for (int i = 0; i < number; i++)
	{
		int l = strlen(codes[i]);
		if ((codes[i][0] == '-' && codes[i][1] == 'I') || codes[i][l - 1] == 'h')
		{
			continue;
		}

		if (find_file(context, codes[i]))
		{
			con_files_add_parametrs(&context->fs, codes[i]);
			con_file_open(&context->fs, context, C_FILE);

			get_next_char(context);

			if (context->nextchar != EOF)
			{
				add_c_file(context);
			}
			con_file_close_cur(context);
		}
	}
	con_file_it_is_end_h(&context->fs);
}

void preprocess_h_file(preprocess_context *context)
{

	context->h_flag = 1;
	context->include_type = 1;
	int stop = 1;
	stop = con_file_open(&context->fs, context, H_FILE);

	while(!stop)
	{
		file_read(context);
		context->temp_output = 0;
		stop = con_file_open(&context->fs, context, H_FILE);
	}

}

void preprocess_c_file(preprocess_context *context)
{
	context->include_type = 2;
	context->h_flag = 0;

	int stop = 1;
	stop = con_file_open(&context->fs, context, C_FILE);

	while (!stop)
	{
		file_read(context);
		stop = con_file_open(&context->fs, context, C_FILE);
	}
	con_file_open_main(&context->fs, context);

	file_read(context);
}

char *preprocess_file(int argc, const char *argv[])
{
	preprocess_context context;
	preprocess_context_init(&context, argc);
	printer_attach_buffer(&context.output_options, 1024);

	add_keywods(&context);

	context.mfirstrp = context.rp;
	open_files(&context, argc, argv);
	preprocess_h_file(&context);
	preprocess_c_file(&context);
	free(context.include_ways);

	char *macro_processed = context.output_options.ptr;

#if MACRODEBUG
	printf("\n\n");
	printf("Текст после препроцессирования:\n>\n%s<\n", macro_processed);
#endif

	con_files_free(&context.fs);
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
