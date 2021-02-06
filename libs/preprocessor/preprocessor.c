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
#include "define.h"
#include "environment.h"
#include "error.h"
#include "file.h"
#include "if.h"
#include "uniio.h"
#include "uniprinter.h"
#include "utils.h"
#include "linker.h"
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


void to_reprtab(const char str[], int num, environment *const env)
{
	int i = 0;
	int oldrepr = env->rp;
	int hash = 0;
	//unsigned char firstchar;
	//unsigned char secondchar;
	//int p;
	int c = 0;
	env->rp += 2;

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
		env->reprtab[env->rp++] = c;
	}

	hash &= 255;
	env->reprtab[env->rp++] = 0;
	env->reprtab[oldrepr] = env->hashtab[hash];
	env->reprtab[oldrepr + 1] = num;
	env->hashtab[hash] = oldrepr;
}

void to_reprtab_full(const char str1[], const char str2[], const char str3[], const char str4[], int num, environment *const env)
{
	to_reprtab(str1, num, env);
	to_reprtab(str2, num, env);
	to_reprtab(str3, num, env);
	to_reprtab(str4, num, env);
}

void add_keywods(environment *const env)
{
	to_reprtab_full("MAIN", "main", "ГЛАВНАЯ", "главная", SH_MAIN, env);
	to_reprtab_full("#DEFINE", "#define", "#ОПРЕД", "#опред", SH_DEFINE, env);
	to_reprtab_full("#IFDEF", "#ifdef", "#ЕСЛИБЫЛ", "#еслибыл", SH_IFDEF, env);
	to_reprtab_full("#IFNDEF", "#ifndef", "#ЕСЛИНЕБЫЛ", "#еслинебыл", SH_IFNDEF, env);
	to_reprtab_full("#IF", "#if", "#ЕСЛИ", "#если", SH_IF, env);
	to_reprtab_full("#ELIF", "#elif", "#ИНЕСЛИ", "#инесли", SH_ELIF, env);
	to_reprtab_full("#ENDIF", "#endif", "#КОНЕЦЕСЛИ", "#конецесли", SH_ENDIF, env);
	to_reprtab_full("#ELSE", "#else", "#ИНАЧЕ", "#иначе", SH_ELSE, env);
	to_reprtab_full("#MACRO", "#macro", "#МАКРО", "#макро", SH_MACRO, env);
	to_reprtab_full("#ENDM", "#endm", "#КОНЕЦМ", "#конецм", SH_ENDM, env);
	to_reprtab_full("#WHILE", "#while", "#ПОКА", "#пока", SH_WHILE, env);
	to_reprtab_full("#ENDW", "#endw", "#КОНЕЦП", "#конецп", SH_ENDW, env);
	to_reprtab_full("#SET", "#set", "#ПЕРЕОПРЕД", "#переопред", SH_SET, env);
	to_reprtab_full("#UNDEF", "#undef", "#ОТМЕНАОПРЕД", "#отменаопред", SH_UNDEF, env);
	to_reprtab_full("#FOR", "#for", "#ДЛЯ", "#для", SH_FOR, env);
	to_reprtab_full("#ENDF", "#endf", "#КОНЕЦД", "#конецд", SH_ENDF, env);
	to_reprtab_full("#EVAL", "#eval", "#ВЫЧИСЛЕНИЕ", "#вычисление", SH_EVAL, env);
	to_reprtab_full("#INCLUDE", "#include", "#ДОБАВИТЬ", "#добавить", SH_INCLUDE, env);
}

int preprocess_words(environment *const env)
{

	skip_space(env);
	switch (env->cur)
	{
		case SH_INCLUDE:
		{
			return lk_include(env);
		}
		case SH_DEFINE:
		case SH_MACRO:
		{
			env->prep_flag = 1;
			return define_realiz(env);
		}
		case SH_UNDEF:
		{
			int k = collect_mident(env);
			if(k)
			{
				env->macrotext[env->reprtab[k + 1]] = MACROUNDEF;
				return space_end_line(env);
			}
			else
			{
				size_t position = skip_str(env); 
				macro_error(macro_does_not_exist
				, lk_get_current(&env->lk)
				, env->error_string, env->line, position);
				return -1;
			}
		}
		case SH_IF:
		case SH_IFDEF:
		case SH_IFNDEF:
		{
			return if_realiz(env);
		}
		case SH_SET:
		{
			return set_realiz(env);
		}
		case SH_ELSE:
		case SH_ELIF:
		case SH_ENDIF:
			return 0;
		case SH_EVAL:
		{
			if (env->curchar == '(')
			{
				if(calculator(0, env))
				{
					return -1;
				}

			}
			else
			{
				size_t position = skip_str(env); 
				macro_error(after_eval_must_be_ckob, lk_get_current(&env->lk), env->error_string, env->line, position);
				return -1;
			}

			m_change_nextch_type(CTYPE, 0, env);
			return 0;
		}
		case SH_WHILE:
		{
			env->wsp = 0;
			env->ifsp = 0;
			if(while_collect(env))
			{
				return -1;
			}
			
			m_change_nextch_type(WHILETYPE, 0, env);
			m_nextch(env);
			m_nextch(env);

			env->nextp = 0;
			int res = while_realiz(env);
			if(env->nextch_type != FILETYPE)
			{
				m_old_nextch_type(env);
			}

			return res;
		}
		default:
		{
			//output_keywods(env);
			size_t position = skip_str(env); 
			macro_error(preproces_words_not_exist, lk_get_current(&env->lk), env->error_string, env->line, position);
			return 0;
		}
	}
}

int preprocess_scan(environment *const env)
{
	int i;
	printf("!!!!!!!1\n");
	switch (env->curchar)
	{
		case EOF:
			return 0;

		case '#':
		{
			env->cur = macro_keywords(env);

			if (env->cur != 0)
			{
				int res = preprocess_words(env);
				if(env->nextchar != '#' && env->nextch_type != WHILETYPE && 
					env->nextch_type != TEXTTYPE)//curflag
				{
					lk_add_comment(env);
				}
				if(env->cur != SH_INCLUDE && env->cur != SH_ELSE && env->cur != SH_ELIF && env->cur != SH_ENDIF)
				{
					m_nextch(env);
				}
				return res;
			}
			else
			{
				// m_nextch(env);
				output_keywods(env);
			}

			return 0;
		}
		case '\'':
		case '\"':
		{
			skip_space_str(env);
			return 0;
		}
		case '@':
		{
			m_nextch(env);
			return 0;
		}
		default:
		{
			if (utf8_is_letter(env->curchar) && env->prep_flag == 1)
			{
				int r = collect_mident(env);

				if (r)
				{
					return define_get_from_macrotext(r, env);
				}
				else
				{
					for (i = 0; i < env->msp; i++)
					{
						m_fprintf(env->mstring[i], env);
					}
				}
			}
			else
			{
				m_fprintf(env->curchar, env);
				m_nextch(env);
			}

			return 0;
		}
	}
}


int macro_form_io(workspace *const ws, universal_io *const output)
{
	environment env;
	env_init(&env, ws, output);

	add_keywods(&env);
	env.mfirstrp = env.rp;
	
	return lk_preprocess_all(&env);
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
	if (!ws_is_correct(ws) || ws_get_files_num(ws) == 0)
	{
		return NULL;
	}

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
	if (!ws_is_correct(ws) || ws_get_files_num(ws) == 0)
	{
		return -1;
	}

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
	return macro(&ws);
}

int auto_macro_to_file(const int argc, const char *const *const argv, const char *const path)
{
	workspace ws = ws_parse_args(argc, argv);
	return macro_to_file(&ws, path);
}
