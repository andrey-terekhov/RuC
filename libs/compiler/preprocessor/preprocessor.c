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

		if((firstchar  & /*0b11100000*/ 0xE0) == /*0b11000000*/ 0xC0) 
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
	if (context->curchar != '(' && context->curchar != '\"')
	{
		m_nextch(context);
	}

	space_skip(context);
	switch (context->cur)
	{
		case SH_INCLUDE:
		{
			if (!context->h_flag)
			{
				include_relis(context, &context->c_files);
			}
			else
			{
				include_relis(context, &context->h_files);
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

			context->nextp = 0;
			while_relis(context);

			return;
		}
		default:
		{
			m_nextch(context);
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
			context->prep_flag = 1;
			preprocess_words(context);

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
	macro_long_string temp_s = f1->befor_sorse;
	f1->befor_sorse = f2->befor_sorse;
	f2->befor_sorse = temp_s;

	int *temp_include_sorse = f1->include_sorse;
	f1->include_sorse = f2->include_sorse;
	f2->include_sorse = temp_include_sorse;

	int temp_pred = f1->pred;
	f1->pred = f2->pred;
	f2->pred = temp_pred;

	char *temp_way = f1->way;
	f1->way = f2->way;
	f2->way = temp_way;

	char *temp_name = f1->name;
	f1->name = f2->name;
	f2->name = temp_name;
}

void add_c_file_siple(preprocess_context *context)
{
	include_sorse_set(&context->c_files, context->befor_temp_p, (&context->befor_temp)->p);

	(&context->befor_temp)->str = (&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->str;
	(&context->befor_temp)->p = (&(&(&context->c_files)->files[(&context->c_files)->cur])->befor_sorse)->p;
	context->temp_output = 0;

	while (context->curchar != EOF && context->main_file == -1)
	{
		if (context->curchar == 'm' || context->curchar == 'M' || context->curchar == 0x413 || context->curchar == 0x433)
		{
			context->cur = macro_keywords(context);
			if (context->cur == SH_MAIN)
			{
				context->main_file = (&context->c_files)->cur;
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
	get_next_char(context);
	m_nextch(context);


	while (context->curchar != EOF)
	{
		switch (context->curchar)
		{
			case EOF:
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
					context->befor_temp_p = get_long_string_p(&context->befor_temp);
					context->cur = macro_keywords(context);
					if ((context->cur == SH_INCLUDE))
					{
						include_relis(context, &context->c_files);
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

/*void open_files_config(const char *code, preprocess_context *context, int start)
{
	FILE* cofig = fopen(context->way, "r");
	context->curent_file = cofig;
	while (code[start] != '/')
	{
		start--;
	}
	start++;
	context->way[start] = '\0';

	char* way_temp = context->way;
	int j = start;
	get_next_char(context);
	m_nextch(context);
	while(context->curchar != EOF)
	{
		while(way_temp[j-1] != 'c'|| way_temp[j-2] != '.')
		{
			way_temp[j++] = context->curchar;
			m_nextch(context);
		}
		way_temp[j++] = '\0';

		if (find_file(context, way_temp))
		{
			int old_cur = open_p_faile(context, way_temp);
			context->line = 1;

			cur_failes_next(&context->c_files, old_cur, context);
			(&context->befor_temp)->str = (&(&context->c_files)->files[(&context->c_files)->cur])->include_sorse;
			(&context->befor_temp)->p = 0;

			add_c_file(context);

			include_fclose(context);
			set_old_cur(&context->c_files, old_cur, context);
			(&context->befor_temp)->str = NULL;


			context->curent_file = cofig;
			get_next_char(context);
			m_nextch(context);
		}
		space_skip(context);
		j = start;
	}
}*/

void open_files_parametr(compiler_workspace_file *codes, preprocess_context *context)
{
	int j = 0;
	int i = 0;
	char *code;
	int k = 1;
	while (codes != NULL)
	{
		code = codes->path;
		k++;
		if (code[0] == '-' && code[1] == 'I')
		{
			i = 2;
			int l = strlen(code);
			context->include_ways[context->iwp] = malloc(l * sizeof(char));
			memset(context->include_ways[context->iwp], 0, l * sizeof(char));

			while (code[i] != '\0')
			{
				context->include_ways[context->iwp][i - 2] = code[i];
				i++;
			}
			if (context->include_ways[context->iwp][i - 3] != '/')
			{
				context->include_ways[context->iwp][i - 2] = '/';
				i++;
			}
			context->include_ways[context->iwp][i - 2] = '\0';
			context->iwp++;
			codes = codes->next;
			continue;
		}

		i = 0;
		while (code[i] != '\0')
		{
			context->way[i] = code[i];
			i++;
			j++;
		}
		context->way[i] = code[i];
		i++;
		context->way[i] = code[i];
		i++;
		context->way[i] = '\0';
		i++;

		context->way[j] = '\0';

		if (find_file(context, context->way))
		{
			int old_cur = open_p_faile(context, context->way);

			cur_failes_next(&context->c_files, old_cur, context);
			(&context->befor_temp)->str = (&(&context->c_files)->files[(&context->c_files)->cur])->include_sorse;
			(&context->befor_temp)->p = 0;

			add_c_file(context);
	
			include_fclose(context);
			set_old_cur(&context->c_files, old_cur, context);
			(&context->befor_temp)->str = NULL;
		}

		codes = codes->next;
	}
}

void open_files(compiler_workspace_file *codes, preprocess_context *context)
{
	context->FILE_flag = 1;
	open_files_parametr(codes, context);
}

void preprocess_h_file(preprocess_context *context, data_files *fs)
{
	context->h_flag = 1;
	context->include_type = 1;
	fs->cur++;
	while (fs->cur < fs->p)
	{
		context->curent_file = get_input(&fs->files[fs->cur]);

		(&context->befor_temp)->str = (&(&fs->files[fs->cur])->befor_sorse)->str;
		(&context->befor_temp)->p = 0;
		context->temp_output = 0;

		file_read(context);
		fs->cur++;
	}

	context->h_flag = 0;
	context->FILE_flag = 0;
}

void preprocess_c_file(preprocess_context *context, data_files *fs)
{
	fs->cur = 0;
	if(context->main_file != fs->p - 1)
	{
		swap(&fs->files[context->main_file], &fs->files[fs->p - 1]);
	}
	
	while (fs->cur < fs->p)
	{
		context->curent_string = (&(&fs->files[fs->cur])->befor_sorse)->str;
		context->curent_p = (&(&fs->files[fs->cur])->befor_sorse)->p;

		file_read(context);
		fs->cur++;
	}
}

void save_data(compiler_context *c_context, preprocess_context *context)
{
	c_context->cfs = context->c_files;
	c_context->hfs = context->h_files;
	c_context->c_flag = -1;
}

const char *preprocess_file(compiler_context *c_context, compiler_workspace_file *code)
{
	#if MACRODEBAG
		printf("\nИсходный текст:\n \n");
	#endif
	preprocess_context *context = malloc(sizeof(preprocess_context));
	preprocess_context_init(context);
	printer_attach_buffer(&context->output_options, 1024);

	for (int i = 0; i < HASH; i++)
	{
		context->hashtab[i] = 0;
	}
	add_keywods(context);

	context->include_type = 0;
	context->FILE_flag = 1;
	context->mfirstrp = context->rp;


	open_files(code, context);

	preprocess_h_file(context, &context->h_files);

	preprocess_c_file(context, &context->c_files);

	save_data(c_context, context);

	const char *macro_processed = strdup(context->output_options.ptr);
	#if MACRODEBAG
		printf("\n>\n%s<\n", macro_processed);
	#endif

	return macro_processed;
}

/*
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
