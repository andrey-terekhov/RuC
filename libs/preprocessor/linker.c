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

#include "linker.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "parser.h"
#include "uniio.h"
#include "uniprinter.h"
#include "utils.h"
#include <string.h>


linker lk_create(workspace *const ws)
{
    linker lk;

    lk.ws = ws;
    lk.current = MAX_PATHS;
    lk.count = ws_get_files_num(ws);

    for (size_t i = 0; i < lk.count; i++)
    {
        lk.included[i] = 0;
    }

    return lk;
}

void lk_make_path(char *const output, const char *const source, const char *const header, const int is_slash)
{
    size_t index = 0;

    if (is_slash)
    {
        char *slash = strrchr(source, '/');
        if (slash != NULL)
        {
            index = slash - source + 1;
            strncpy(output, source, index);
        }
    }
    else
    {
        index = sprintf(output, "%s/", source);
    }

    strcpy(&output[index], header);
}

size_t lk_open_include(environment *const env, const char *const path)
{
    char full_path[MAX_ARG_SIZE];
    lk_make_path(full_path, lk_get_current(env->lk), path, 1);

    if (in_set_file(env->input, full_path))
    {
        size_t i = 0;
        const char *dir;

        do
        {
            dir = ws_get_dir(env->lk->ws, i++);
            lk_make_path(full_path, dir, path, 0);
        } while (dir != NULL && in_set_file(env->input, full_path));
    }

    if (!in_is_correct(env->input))
    {
        in_clear(env->input);
        macro_system_error(full_path, include_file_not_found);
        return SIZE_MAX - 1;
    }

    const size_t index = ws_add_file(env->lk->ws, full_path);
    if (index == env->lk->count)
    {
        env->lk->included[env->lk->count++] = 0;
    }
    else if (env->lk->included[index])
    {
        in_clear(env->input);
        return SIZE_MAX;
    }

    return index;
}

int lk_open_source(environment *const env, const size_t index)
{
    if (in_set_file(env->input, ws_get_file(env->lk->ws, index)))
    {
        macro_system_error(lk_get_current(env->lk), source_file_not_found);
        return -1;
    }

    return 0;
}

int lk_preprocess_file(environment *const env, const size_t number)
{
    env_clear_error_string(env);
    env->lk->included[number]++;

    const size_t old_cur = env->lk->current;
    const size_t old_line = env->line;
    env->lk->current = number;
    env->line = 1;

    get_next_char(env);
    m_nextch(env);

    if (env->curchar != '#')
    {
        env_add_comment(env);
    }

    int was_error = 0;
    while (env->curchar != EOF)
    {
        was_error = preprocess_token(env) || was_error;
    }

    m_fprintf(env, '\n');

    env->line = old_line;
    env->lk->current = old_cur;

    in_clear(env->input);
    return was_error ? -1 : 0;
}

int lk_preprocess_include(environment *const env)
{
    char header_path[MAX_ARG_SIZE];
    size_t i = 0;

    while (env->curchar != '\"')
    {
        if (env->curchar == EOF)
        {
            env_error(env, must_end_quote);
            return -1;
        }

        i += utf8_to_string(&header_path[i], env->curchar);
        m_nextch(env);
    }

    universal_io new_in = io_create();
    universal_io *old_in = env->input;
    env->input = &new_in;

    const size_t index = lk_open_include(env, header_path);
    if (index >= SIZE_MAX - 1)
    {
        env->input = old_in;
        return index == SIZE_MAX ? 0 : -1;
    }

    int flag_io_type = 0;
    if (env->nextch_type != FILETYPE)
    {
        m_change_nextch_type(env, FILETYPE, 0);
        flag_io_type++;
    }

    const int res = 2 * lk_preprocess_file(env, index);
    env->input = old_in;

    if (flag_io_type)
    {
        m_old_nextch_type(env);
    }

    return res;
}

int lk_include(environment *const env)
{
    if (env == NULL)
    {
        return -1;
    }

    skip_separators(env);

    if (env->curchar != '\"')
    {
        env_error(env, must_start_quote);
        return -1;
    }

    m_nextch(env);

    int res = lk_preprocess_include(env);
    if (res == -2)
    {
        end_of_file(env);
        return -1;
    }

    get_next_char(env);
    m_nextch(env);

    return res;
}

int lk_preprocess_all(environment *const env)
{
    if (env == NULL)
    {
        return -1;
    }

    for (size_t i = 0; i < env->lk->count; i++)
    {
        if (env->lk->included[i])
        {
            continue;
        }

        universal_io input = io_create();
        env->input = &input;

        if (lk_open_source(env, i) || lk_preprocess_file(env, i))
        {
            return -1;
        }
        in_clear(&input);
    }

    if (env->was_error)
    {
        return -1;
    }

    return 0;
}

const char *lk_get_current(const linker *const lk)
{
    return lk == NULL ? NULL : ws_get_file(lk->ws, lk->current);
}
