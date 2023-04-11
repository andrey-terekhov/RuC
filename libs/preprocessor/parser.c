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

#include "parser.h"
#include "calculator.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "macro_load.h"
#include "macro_save.h"
#include "utils.h"


int if_check(environment *const env, int type_if)
{
    int flag = 0;

    if (type_if == SH_IF)
    {
        if (calculate(env, LOGIC))
        {
            return -1;
        }
        return env->calc_string[0];
    }
    else
    {
        if (collect_mident(env))
        {
            flag = 1;
        }

        if (skip_line(env))
        {
            return -1;
        }

        if (type_if == SH_IFDEF)
        {
            return flag;
        }
        else
        {
            return 1 - flag;
        }
    }
}

int if_end(environment *const env)
{
    int fl_cur;

    while (env->curchar != EOF)
    {
        if (env->curchar == '#')
        {
            fl_cur = macro_keywords(env);
            if (fl_cur == SH_ENDIF)
            {
                env->nested_if--;
                if (env->nested_if < 0)
                {
                    env_error(env, before_endif);
                    return -1;
                }
                return 0;
            }

            if (fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF)
            {
                env->nested_if++;
                if (if_end(env))
                {
                    return -1;
                }
            }
        }
        else
        {
            m_nextch(env);
        }
    }

    env_error(env, must_be_endif);
    return -1;
}

int if_false(environment *const env)
{
    int fl_cur = env->cur;

    while (env->curchar != EOF)
    {
        if (env->curchar == '#')
        {
            fl_cur = macro_keywords(env);
            m_nextch(env);

            if (fl_cur == SH_ELSE || fl_cur == SH_ELIF || fl_cur == SH_ENDIF)
            {
                return fl_cur;
            }

            if ((fl_cur == SH_IF || fl_cur == SH_IFDEF || fl_cur == SH_IFNDEF) && if_end(env))
            {
                return 0;
            }
        }
        else
        {
            m_nextch(env);
        }
    }


    env_error(env, must_be_endif);
    return 0;
}

int if_true(environment *const env, const int type_if)
{
    int error = 0;
    while (env->curchar != EOF)
    {
        error = preprocess_token(env);
        if (error)
        {
            return error;
        }

        if (env->cur == SH_ELSE || env->cur == SH_ELIF)
        {
            break;
        }

        if (env->cur == SH_ENDIF)
        {
            env->nested_if--;
            if (env->nested_if < 0)
            {
                env_error(env, before_endif);
                return -1;
            }

            return 0;
        }
    }

    if (type_if != SH_IF && env->cur == SH_ELIF)
    {
        env_error(env, dont_elif);
        env->nested_if--;
        return -1;
    }

    return if_end(env);
}

int if_implementation(environment *const env)
{
    const int type_if = env->cur;
    const int truth_flag = if_check(env, type_if); // начало (if)
    if (truth_flag == -1)
    {
        env->nested_if--;
        return -1;
    }

    env->nested_if++;
    if (truth_flag)
    {
        return if_true(env, type_if);
    }
    else
    {
        const int res = if_false(env);
        if (!res)
        {
            env->nested_if--;
            return -1;
        }
        env->cur = res;
    }


    while (env->cur == SH_ELIF)
    {
        const int el_truth_flag = if_check(env, type_if);
        if (el_truth_flag == -1 || skip_line(env))
        {
            env->nested_if--;
            return -1;
        }

        if (el_truth_flag)
        {
            return if_true(env, type_if);
        }
        else
        {
            const int res = if_false(env);
            if (!res)
            {
                env->nested_if--;
                return -1;
            }
            env->cur = res;
        }
    }


    if (env->cur == SH_ELSE)
    {
        env->cur = 0;
        return if_true(env, type_if);
    }

    if (env->cur == SH_ENDIF)
    {
        env->nested_if--;
        if (env->nested_if < 0)
        {
            env_error(env, before_endif);
            return -1;
        }
    }
    return 0;
}


int while_collect(environment *const env)
{
    const int oldwsp = (int)env->while_string_size;

    env->while_string[env->while_string_size++] = WHILEBEGIN;
    env->while_string[env->while_string_size++] = (int)env->if_string_size;
    env->while_string_size++;

    while (env->curchar != '\n')
    {
        env->if_string[env->if_string_size++] = env->curchar;
        m_nextch(env);
    }
    env->if_string[env->if_string_size++] = '\n';
    m_nextch(env);

    while (env->curchar != EOF)
    {
        if (env->curchar == '#')
        {
            env->cur = macro_keywords(env);

            if (env->cur == SH_WHILE)
            {
                while_collect(env);
            }
            else if (env->cur == SH_ENDW)
            {
                env->while_string[env->while_string_size++] = ' ';
                env->while_string[oldwsp + 2] = (int)env->while_string_size;
                env->cur = 0;

                return 0;
            }
            else
            {
                for (size_t i = 0; i < (size_t)env->reprtab[env->rp]; i++)
                {
                    env->while_string[env->while_string_size++] = env->reprtab[env->rp + 2 + i];
                }
            }
        }
        env->while_string[env->while_string_size++] = env->curchar;
        m_nextch(env);
    }

    env_error(env, must_end_endw);
    return -1;
}

int while_implementation(environment *const env)
{
    const int oldernextp = (int)env->nextp;
    const size_t end = (size_t)env->while_string[oldernextp + 2];
    int error = 0;

    env->cur = 0;
    while (env->while_string[oldernextp] == WHILEBEGIN)
    {
        m_nextch(env);
        m_change_nextch_type(env, IFTYPE, env->while_string[env->nextp]);
        m_nextch(env);
        if (calculate(env, LOGIC))
        {
            return -1;
        }
        m_old_nextch_type(env);


        if (env->calc_string[0] == 0)
        {
            env->nextp = end;
            m_nextch(env);
            return 0;
        }

        m_nextch(env);
        m_nextch(env);
        m_nextch(env);
        skip_separators(env);

        while (env->nextp != end || env->nextch_type != WHILETYPE)
        {
            if (env->curchar == WHILEBEGIN)
            {
                env->nextp--;
                if (while_implementation(env))
                {
                    return -1;
                }
            }
            else if (env->curchar == EOF)
            {
                env_error(env, must_end_endw);
                return -1;
            }
            else
            {
                error = preprocess_token(env);
                if (error)
                {
                    return error;
                }
            }
        }
        env->nextp = oldernextp;
    }
    return 0;
}


int preprocess_words(environment *const env)
{
    skip_separators(env);
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
            return macro_add(env);
        }
        case SH_UNDEF:
        {
            const int macro_ptr = collect_mident(env);
            if (macro_ptr)
            {
                env->macro_tab[env->reprtab[macro_ptr + 1]] = MACROUNDEF;
                return skip_line(env);
            }
            else
            {
                env_error(env, macro_does_not_exist);
                return -1;
            }
        }
        case SH_IF:
        case SH_IFDEF:
        case SH_IFNDEF:
        {
            return if_implementation(env);
        }
        case SH_SET:
        {
            return macro_set(env);
        }
        case SH_ELSE:
        case SH_ELIF:
        case SH_ENDIF:
            return 0;
        case SH_EVAL:
        {
            if (env->curchar != '(')
            {
                env_error(env, after_eval_must_be_ckob);
                return -1;
            }
            else if (calculate(env, ARITHMETIC))
            {
                return -1;
            }

            m_change_nextch_type(env, CTYPE, 0);
            return 0;
        }
        case SH_WHILE:
        {
            env->while_string_size = 0;
            env->if_string_size = 0;
            if (while_collect(env))
            {
                return -1;
            }
            m_change_nextch_type(env, WHILETYPE, 0);
            m_nextch(env);
            m_nextch(env);

            env->nextp = 0;
            int res = while_implementation(env);
            if (env->nextch_type != FILETYPE)
            {
                m_old_nextch_type(env);
            }

            return res;
        }
        default:
        {
            // output_keywords(env);
            env_error(env, preproces_words_not_exist);
            return 0;
        }
    }
}

int preprocess_token(environment *const env)
{
    if (env == NULL)
    {
        return -1;
    }

    switch (env->curchar)
    {
        case EOF:
            return 0;

        case '#':
        {
            env->cur = macro_keywords(env);

            if (env->cur != 0)
            {
                const int res = preprocess_words(env);
                if (env->nextchar != '#' && env->nextch_type != WHILETYPE && env->nextch_type != TEXTTYPE) // curflag
                {
                    env_add_comment(env);
                }
                if (env->cur != SH_INCLUDE && env->cur != SH_ELSE && env->cur != SH_ELIF && env->cur != SH_ENDIF)
                {
                    m_nextch(env);
                }
                return res;
            }
            else
            {
                // m_nextch(env);
                output_keywords(env);
            }

            return 0;
        }
        case '\'':
        case '\"':
        {
            return skip_string(env);
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
                const int macro_ptr = collect_mident(env);
                if (macro_ptr)
                {
                    return macro_get(env, macro_ptr);
                }
                else
                {
                    for (size_t i = 0; i < env->msp; i++)
                    {
                        m_fprintf(env, env->mstring[i]);
                    }
                }
            }
            else
            {
                m_fprintf(env, env->curchar);
                m_nextch(env);
            }

            return 0;
        }
    }
}
