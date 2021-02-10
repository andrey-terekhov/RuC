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
#include "parser.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "file.h"
#include "uniio.h"
#include "uniprinter.h"
#include "utils.h"
#include "linker.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


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

int macro_form_io(workspace *const ws, universal_io *const output)
{
	linker lk = lk_create(ws);

	environment env;
	env_init(&env, &lk, output);

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
