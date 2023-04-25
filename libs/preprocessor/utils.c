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

#include "utils.h"
#include "constants.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "utf8.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int equal_reprtab(int i, int j, environment *const env)
{
    i += 2;
    j += 2;

    while (env->reprtab[i] == env->reprtab[j])
    {
        i++;
        j++;

        if (env->reprtab[i] == 0 && env->reprtab[j] == 0)
        {
            return 1;
        }
    }

    return 0;
}

void output_keywords(environment *const env)
{
    for (size_t j = 0; j < (size_t)env->reprtab[env->rp]; j++)
    {
        m_fprintf(env, env->reprtab[env->rp + 2 + j]);
    }
}

int macro_keywords(environment *const env)
{
    int oldrepr = env->rp;
    int r = 0;
    int n = 0;

    env->rp += 2;
    int hash = 0;
    do
    {
        hash += env->curchar;
        env->reprtab[env->rp++] = env->curchar;
        n++;
        m_nextch(env);
    } while (utf8_is_letter(env->curchar) || utf8_is_digit(env->curchar));

    /*if (env->curchar != '\n' && env->curchar != ' ' && env->curchar != '\t' && env->curchar != '(' &&
        env->curchar != '\"')
    {

        env_error(env, after_ident_must_be_space);
    }*/

    hash &= 255;
    env->reprtab[env->rp++] = 0;
    r = env->hashtab[hash];
    if (r)
    {
        do
        {
            if (equal_reprtab(r, oldrepr, env))
            {
                env->rp = oldrepr;
                env->reprtab[env->rp] = n;
                return (env->reprtab[r + 1] < 0) ? env->reprtab[r + 1] : 0;
            }
            else
            {
                r = env->reprtab[r];
            }
        } while (r);
    }

    env->rp = oldrepr;
    env->reprtab[env->rp] = n;
    return 0;
}

int mf_equal(int i, environment *const env)
{
    int j = 0;
    i += 2;

    while (env->reprtab[i] == env->mstring[j])
    {
        i++;
        j++;
        if (env->reprtab[i] == 0 && env->mstring[j] == MACROEND)
        {
            return 1;
        }
    }

    return 0;
}

int collect_mident(environment *const env)
{
    int r;
    int hash = 0;
    env->msp = 0;

    while (utf8_is_letter(env->curchar) || utf8_is_digit(env->curchar))
    {
        env->mstring[env->msp++] = env->curchar;
        hash += env->curchar;
        m_nextch(env);
    }

    env->mstring[env->msp] = MACROEND;
    hash &= 255;
    r = env->hashtab[hash];

    while (r)
    {
        if (r >= env->mfirstrp && mf_equal(r, env))
        {
            return (env->macro_tab[env->reprtab[r + 1]] != MACROUNDEF) ? r : 0;
        }

        r = env->reprtab[r];
    }

    return 0;
}

int skip_line(environment *const env)
{
    while (env->curchar != '\n')
    {
        if (env->curchar == ' ' || env->curchar == '\t' || env->curchar == '\r')
        {
            m_nextch(env);
        }
        else
        {
            env_error(env, after_preproces_words_must_be_space);
            return -1;
        }
    }

    return 0;
}

void skip_separators(environment *const env)
{
    while (env->curchar == ' ' || env->curchar == '\t')
    {
        m_nextch(env);
    }
}

int skip_string(environment *const env)
{
    int c = env->curchar;
    m_fprintf(env, env->curchar);
    m_nextch(env);

    while (env->curchar != c && env->curchar != EOF && env->curchar != '\n')
    {
        if (env->curchar == '\\')
        {
            m_fprintf(env, env->curchar);
            m_nextch(env);
        }

        m_fprintf(env, env->curchar);
        m_nextch(env);
    }

    if (env->curchar == EOF || env->curchar == '\n')
    {
        env_error(env, no_string_ending);
        return -1;
    }

    m_fprintf(env, env->curchar);
    m_nextch(env);
    return 0;
}

void end_of_file(environment *const env)
{
    while (env->nextch_type != FILETYPE)
    {
        m_old_nextch_type(env);
    }
    env->curchar = EOF;
}
