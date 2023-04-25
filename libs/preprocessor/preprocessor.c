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
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "parser.h"
#include "uniio.h"
#include "uniprinter.h"
#include "utils.h"


const size_t SIZE_OUT_BUFFER = 1024;


void to_reprtab(environment *const env, const char str[], int num)
{
    int i = 0;
    int oldrepr = env->rp;
    int hash = 0;
    // unsigned char firstchar;
    // unsigned char secondchar;
    // int p;
    int c = 0;
    env->rp += 2;

    while (str[i] != '\0')
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

void to_reprtab_full(environment *const env, const char str1[], const char str2[], const char str3[], const char str4[],
                     int num)
{
    to_reprtab(env, str1, num);
    to_reprtab(env, str2, num);
    to_reprtab(env, str3, num);
    to_reprtab(env, str4, num);
}

void add_keywods(environment *const env)
{
    to_reprtab_full(env, "MAIN", "main", "ГЛАВНАЯ", "главная", SH_MAIN);
    to_reprtab_full(env, "#DEFINE", "#define", "#ОПРЕД", "#опред", SH_DEFINE);
    to_reprtab_full(env, "#IFDEF", "#ifdef", "#ЕСЛИБЫЛ", "#еслибыл", SH_IFDEF);
    to_reprtab_full(env, "#IFNDEF", "#ifndef", "#ЕСЛИНЕБЫЛ", "#еслинебыл", SH_IFNDEF);
    to_reprtab_full(env, "#IF", "#if", "#ЕСЛИ", "#если", SH_IF);
    to_reprtab_full(env, "#ELIF", "#elif", "#ИНЕСЛИ", "#инесли", SH_ELIF);
    to_reprtab_full(env, "#ENDIF", "#endif", "#КОНЕЦЕСЛИ", "#конецесли", SH_ENDIF);
    to_reprtab_full(env, "#ELSE", "#else", "#ИНАЧЕ", "#иначе", SH_ELSE);
    to_reprtab_full(env, "#MACRO", "#macro", "#МАКРО", "#макро", SH_MACRO);
    to_reprtab_full(env, "#ENDM", "#endm", "#КОНЕЦМ", "#конецм", SH_ENDM);
    to_reprtab_full(env, "#WHILE", "#while", "#ПОКА", "#пока", SH_WHILE);
    to_reprtab_full(env, "#ENDW", "#endw", "#КОНЕЦП", "#конецп", SH_ENDW);
    to_reprtab_full(env, "#SET", "#set", "#ПЕРЕОПРЕД", "#переопред", SH_SET);
    to_reprtab_full(env, "#UNDEF", "#undef", "#ОТМЕНАОПРЕД", "#отменаопред", SH_UNDEF);
    to_reprtab_full(env, "#FOR", "#for", "#ДЛЯ", "#для", SH_FOR);
    to_reprtab_full(env, "#ENDF", "#endf", "#КОНЕЦД", "#конецд", SH_ENDF);
    to_reprtab_full(env, "#EVAL", "#eval", "#ВЫЧИСЛЕНИЕ", "#вычисление", SH_EVAL);
    to_reprtab_full(env, "#INCLUDE", "#include", "#ДОБАВИТЬ", "#добавить", SH_INCLUDE);
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
    char *ret = macro(&ws);
    ws_clear(&ws);
    return ret;
}

int auto_macro_to_file(const int argc, const char *const *const argv, const char *const path)
{
    workspace ws = ws_parse_args(argc, argv);
    const int ret = macro_to_file(&ws, path);
    ws_clear(&ws);
    return ret;
}
