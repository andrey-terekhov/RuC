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
#include "constants.h"
#include "context_var.h"
#include "define.h"
#include "file.h"
#include "if.h"
#include "include.h"
#include "uniio.h"
#include "uniprinter.h"
#include "error.h"
#include "utils.h"
#include "while.h"
#include "workspace.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


#define H_FILE 1
#define C_FILE 0


const size_t SIZE_OUT_BUFFER = 1024;


void to_reprtab(const char str[], int num, preprocess_context *context)
{
	int i = 0;
	int oldrepr = context->rp;
	int hash = 0;
	//unsigned char firstchar;
	//unsigned char secondchar;
	//int p;
	int c = 0;
	context->rp += 2;

	while(str[i] != '\0')
	{
		/*sscanf(&str[i], "%c%n", &firstchar, &p);

		if ((firstchar & 0xE0) == 0xC0)
		{
			++i;
			sscanf(&str[i], "%c%n", &secondchar, &p);
			c = ((int)(firstchar & 0x1F)) << 6 | (secondchar & 0x3F);
		}
		else
		{
			c = firstchar;
		}*/
		c = utf8_convert(&str[i]);
		i += (int)utf8_symbol_size(str[i]);

		hash += c;
		context->reprtab[context->rp++] = c;
	}

	hash &= 255;
	context->reprtab[context->rp++] = 0;
	context->reprtab[oldrepr] = context->hashtab[hash];
	context->reprtab[oldrepr + 1] = num;
	context->hashtab[hash] = oldrepr;
}

void to_reprtab_full(const char str1[], const char str2[], const char str3[], const char str4[], int num, preprocess_context *context)
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

int preprocess_words(preprocess_context *context)
{

	space_skip(context);
	switch (context->cur)
	{
		case SH_INCLUDE:
		{
			return include_relis(context);
		}
		case SH_DEFINE:
		case SH_MACRO:
		{
			context->prep_flag = 1;
			return define_relis(context);
		}
		case SH_UNDEF:
		{
			int k = collect_mident(context);
			if(k)
			{
				context->macrotext[context->reprtab[k + 1]] = MACROUNDEF;
				return space_end_line(context);
			}
			else
			{
				size_t position = skip_str(context); 
				macro_error(macro_does_not_exist, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
		}
		case SH_IF:
		case SH_IFDEF:
		case SH_IFNDEF:
		{
			return if_relis(context);
		}
		case SH_SET:
		{
			return set_relis(context);
		}
		case SH_ELSE:
		case SH_ELIF:
		case SH_ENDIF:
			return 0;
		case SH_EVAL:
		{
			if (context->curchar == '(')
			{
				if(calculator(0, context))
				{
					return -1;
				}

			}
			else
			{
				size_t position = skip_str(context); 
				macro_error(after_eval_must_be_ckob, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}

			m_change_nextch_type(CTYPE, 0, context);
			return 0;
		}
		case SH_WHILE:
		{
			context->wsp = 0;
			context->ifsp = 0;
			if(while_collect(context))
			{
				return -1;
			}
			
			m_change_nextch_type(WHILETYPE, 0, context);
			m_nextch(context);
			m_nextch(context);

			context->nextp = 0;
			int res = while_relis(context);
			if(context->nextch_type != FILETYPE)
			{
				m_old_nextch_type(context);
			}

			return res;
		}
		default:
		{
			//output_keywods(context);
			size_t position = skip_str(context); 
			macro_error(preproces_words_not_exist, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			return 0;
		}
	}
}

int preprocess_scan(preprocess_context *context)
{
	int i;

	switch (context->curchar)
	{
		case EOF:
			return 0;

		case '#':
		{
			context->cur = macro_keywords(context);

			if (context->cur != 0)
			{
				int res = preprocess_words(context);
				if(context->nextchar != '#' && context->nextch_type != WHILETYPE && 
					context->nextch_type != TEXTTYPE)//curflag
				{
					con_file_print_coment(&context->fs, context);
				}
				if(context->cur != SH_ELSE && context->cur != SH_ELIF && context->cur != SH_ENDIF)
				{
					m_nextch(context);
				}
				return res;
			}
			else
			{
				// m_nextch(context);
				output_keywods(context);
			}

			return 0;
		}
		case '\'':
		case '\"':
		{
			space_skip_str(context);
			return 0;
		}
		case '@':
		{
			m_nextch(context);
			return 0;
		}
		default:
		{
			if (is_letter(context) && context->prep_flag == 1)
			{
				int r = collect_mident(context);

				if (r)
				{
					return define_get_from_macrotext(r, context);
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

			return 0;
		}
	}
}

void add_c_file_siple(preprocess_context *context)
{
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
				}
				break;
			}
			default:
			{
				add_c_file_siple(context);
				return;
			}
		}
	}
}

void open_files(preprocess_context *context)
{
	int i = 0;
	size_t num = ws_get_files_num(context->fs.ws);
	const char *temp = ws_get_file(context->fs.ws, i++);

	for(size_t j = 0; j < num; j++)
	{
		if (find_file(context, temp))
		{
			con_file_open_next(&context->fs, context, C_FILE);

			get_next_char(context);

			if (context->nextchar != EOF)
			{
				add_c_file(context);
				context->position = 0;
				context->error_string[context->position] = '\0';
			}
			con_file_close_cur(context);
		}
		temp = ws_get_file(context->fs.ws, i++);
	}

	con_file_it_is_end_h(&context->fs, i-1);
}

int preprocess_h_file(preprocess_context *context)
{
	context->h_flag = 1;
	context->include_type = 1;
	int res = con_file_open_hedrs(&context->fs, context);
	if(res == 1)
	{
		res = file_read(context);
		
		if(file_read(context))
		{
			return -1;
		}
			
		res = con_file_open_next(&context->fs, context, C_FILE);

		while (res == 1)
		{
			if(file_read(context))
			{
				return -1;
			}
			
			res = con_file_open_next(&context->fs, context, H_FILE);
		}
	}
	return res;
}

int preprocess_c_file(preprocess_context *context)
{
	context->include_type = 2;
	context->h_flag = 0;
	int res = con_file_open_sorse(&context->fs, context);
	if(res == 1)
	{
		if(file_read(context))
		{
			return -1;
		}

		res = con_file_open_next(&context->fs, context, C_FILE);

		while (res == 1)
		{
			if(file_read(context))
			{
				return -1;
			}
			
			res = con_file_open_next(&context->fs, context, C_FILE);
		}
	}
	return res;
}


int macro_form_io(workspace *const ws, universal_io *const io)
{
	universal_io io_input = io_create();
	preprocess_context context;
	preprocess_context_init(&context, ws, io, &io_input);

	add_keywods(&context);
	context.mfirstrp = context.rp;

	open_files(&context);

	if (preprocess_h_file(&context))
	{
		return -1;
	}

	if (preprocess_c_file(&context))
	{
		return -1;
	}

	in_clear(&io_input);
	
	return 0;
}

/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


char *macro(workspace *const ws)
{
	universal_io io = io_create();
	if (out_set_buffer(&io, SIZE_OUT_BUFFER))
	{
		return NULL;
	}

	int ret = macro_form_io(ws, &io);
	if (ret)
	{
		io_erase(&io);
		return NULL;
	}

	in_clear(&io);
	return out_extract_buffer(&io);
}

int macro_to_file(workspace *const ws, const char *const path)
{
	universal_io io = io_create();
	if (out_set_file(&io, path))
	{
		return -1;
	}

	int ret = macro_form_io(ws, &io);

	io_erase(&io);
	return ret;
}


char *auto_macro(const int argc, const char *const *const argv)
{
	workspace ws = ws_parse_args(argc, argv);
	if (!ws_is_correct(&ws))
	{
		return NULL;
	}

	return macro(&ws);
}

int auto_macro_to_file(const int argc, const char *const *const argv, const char *const path)
{
	workspace ws = ws_parse_args(argc, argv);
	if (!ws_is_correct(&ws))
	{
		return -1;
	}

	return macro_to_file(&ws, path);
}
