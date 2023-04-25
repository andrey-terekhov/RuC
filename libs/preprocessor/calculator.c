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

#include "calculator.h"
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "macro_load.h"
#include "utils.h"
#include <math.h>


#define STK_SIZE 32
#define OPN_SIZE STK_SIZE


double get_digit(environment *const env, int *error) // Временная замена
{
    double k;
    int d = 1;

    env->flagint = 1;
    int num = 0;
    double numdouble = 0.0;
    if (env->curchar == '-')
    {
        d = -1;
        m_nextch(env);
    }

    while (utf8_is_digit(env->curchar))
    {
        numdouble = numdouble * 10 + (env->curchar - '0');
        if (numdouble > (double)INT_MAX)
        {
            env_error(env, too_many_nuber);
            *error = -1;
            return 0.0;
        }
        num = num * 10 + (env->curchar - '0');
        m_nextch(env);
    }

    if (env->curchar == '.')
    {
        env->flagint = 0;
        m_nextch(env);
        k = 0.1;

        while (utf8_is_digit(env->curchar))
        {
            numdouble += (env->curchar - '0') * k;
            k *= 0.1;
            m_nextch(env);
        }
    }

    if (utf8_is_power(env->curchar))
    {
        int power = 0;
        int sign = 1;
        int i;

        m_nextch(env);
        if (env->curchar == '-')
        {
            env->flagint = 0;
            m_nextch(env);
            sign = -1;
        }
        else if (env->curchar == '+')
        {
            m_nextch(env);
        }


        if (!utf8_is_digit(env->curchar))
        {
            env_error(env, must_be_digit_after_exp1);
            *error = -1;
            return 0.0;
        }


        while (utf8_is_digit(env->curchar))
        {
            power = power * 10 + env->curchar - '0';
            m_nextch(env);
        }

        if (env->flagint)
        {
            for (i = 1; i <= power; i++)
            {
                num *= 10;
            }
        }

        numdouble *= pow(10.0, sign * power);
    }

    if (env->flagint)
    {
        return num * d;
    }
    else
    {
        return numdouble * d;
    }
}

char get_opiration(const char cur, const char next)
{
    switch (cur)
    {
        case '|':
        case '&':
        case '=':
        case '!':
            if ((next == cur && cur != '!') || (cur == '!' && next == '='))
            {
                return cur;
            }
            else
            {
                return '\0';
            }
        case '>':
        case '<':
            if (cur == '>' && next == '=')
            {
                return 'b';
            }
            else if (cur == '>' && next == '=')
            {
                return 's';
            }
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '(':
            return cur;
        default:
            return '\0';
    }
}

int get_prior(const char opiration)
{
    switch (opiration)
    {
        case '(':
            return 0;
        case '|':
            return 1;
        case '&':
            return 2;
        case '<':
        case '>':
        case 's':
        case 'b':
        case '=':
        case '!':
            return 3;
        case '+':
        case '-':
            return 4;
        case '*':
        case '/':
        case '%':
            return 5;
        default:
            return 0;
    }
}

double calc_count(const double x, const double y, const char opiration, const int is_int)
{
    switch (opiration)
    {
        case '<':
            return x < y;
        case '>':
            return x > y;
        case 's':
            return x <= y;
        case 'b':
            return x >= y;
        case '=':
            return x == y;
        case '!':
            return x != y;
        case '&':
            return x && y;
        case '|':
            return x || y;
        case '+':
            return x + y;
        case '-':
            return x - y;
        case '*':
            return x * y;
        case '/':
            if (is_int)
            {
                return (int)x / (int)y;
            }
            else
            {
                return x / y;
            }
        case '%':
            if (is_int)
            {
                return (int)x % (int)y;
            }
            else
            {
                return x / y;
            }

        default:
            return 0;
    }
}

void double_to_string(environment *const env, const double x, const int is_int)
{
    char s[30] = "\0";

    if (is_int)
    {
        sprintf(s, "%f", x);
        for (env->calc_string_size = 0; env->calc_string_size < 20; env->calc_string_size++)
        {
            env->calc_string[env->calc_string_size] = s[env->calc_string_size];

            if (s[env->calc_string_size] == '.')
            {
                return;
            }
        }
    }
    else
    {
        int l = 0;

        sprintf(s, "%.14lf", x);
        for (env->calc_string_size = 0; env->calc_string_size < 20; env->calc_string_size++)
        {
            env->calc_string[env->calc_string_size] = s[env->calc_string_size];

            if (s[env->calc_string_size] != '0' && utf8_is_digit(s[env->calc_string_size]))
            {
                l = (int)env->calc_string_size;
            }
        }
        env->calc_string_size = l + 1;
    }
}

int calc_macro(environment *const env)
{
    const int macro_ptr = collect_mident(env);
    if (!macro_ptr)
    {
        env_error(env, not_macro);
        return -1;
    }

    if (macro_get(env, macro_ptr))
    {
        return -1;
    }

    return 0;
}

int calc_digit(environment *const env, double *stack, int *is_int, int *stk_size)
{
    int error = 0;
    stack[*stk_size] = get_digit(env, &error);
    if (error)
    {
        return -1;
    }

    is_int[*stk_size] = env->flagint;
    *stk_size = *stk_size + 1;

    return 0;
}

int calc_opiration(environment *const env, double *const stack, int *const is_int, char *const operation, int *op_size,
                   int *stk_size, const int type)
{
    const char c = get_opiration((char)env->curchar, (char)env->nextchar);
    if (!c)
    {
        env_error(env, third_party_symbol);
        return -1;
    }

    m_nextch(env);
    if (c == 'b' || c == 's' || c == '=' || c == '&' || c == '|' || c == '!')
    {
        m_nextch(env);
    }

    const int prior = get_prior(c);
    if (type == LOGIC && prior > 3)
    {
        env_error(env, not_arithmetic_operations);
        return -1;
    }
    if (type == ARITHMETIC && prior <= 3)
    {
        env_error(env, not_logical_operations);
        return -1;
    }

    while (*op_size != 0 && get_prior(operation[*op_size - 1]) >= prior)
    {
        is_int[*stk_size - 2] = is_int[*stk_size - 2] && is_int[*stk_size - 1];
        stack[*stk_size - 2] =
            calc_count(stack[*stk_size - 2], stack[*stk_size - 1], operation[*op_size - 1], is_int[*stk_size - 2]);
        *op_size = *op_size - 1;
        *stk_size = *stk_size - 1;
    }

    operation[*op_size] = c;
    *op_size = *op_size + 1;

    return 0;
}

int calc_close(double *const stack, int *const is_int, const char *operation, int *op_size, int *stk_size)
{
    int scope_flag = 0;
    if (operation[*op_size - 1] == ')')
    {
        scope_flag++;
        *op_size = *op_size - 1;
    }

    while ((scope_flag && operation[*op_size - 1] != '(' && operation[*op_size - 1] != '[') ||
           (!scope_flag && *op_size > 0))
    {
        if (*stk_size < 2 || *op_size == 0)
        {
            return -1;
        }

        is_int[*stk_size - 2] = is_int[*stk_size - 2] && is_int[*stk_size - 1];
        stack[*stk_size - 2] =
            calc_count(stack[*stk_size - 2], stack[*stk_size - 1], operation[*op_size - 1], is_int[*stk_size - 2]);
        *op_size = *op_size - 1;
        *stk_size = *stk_size - 1;
    }

    *op_size = *op_size - 1;
    return 0;
}

int additional_elements(environment *const env, double *const stack, int *const is_int, char *const operation,
                        int *op_size, int *stk_size, int *type, int opration_flag)
{
    if (env->curchar == '#' && *type == LOGIC && !opration_flag)
    {
        const int cur = macro_keywords(env);
        if (cur == SH_EVAL && env->curchar == '(')
        {
            *type = ARITHMETIC;
            operation[*op_size] = '[';
            *op_size = *op_size + 1;
            m_nextch(env);
            return 1;
        }
        else
        {
            env_error(env, after_eval_must_be_ckob); //
            return -1;
        }
    }

    if (utf8_is_letter(env->curchar))
    {
        if (calc_macro(env))
        {
            return -1;
        }
        return 1;
    }

    if (env->curchar == '(' && !opration_flag)
    {
        operation[*op_size] = '(';
        *op_size = *op_size + 1;
        m_nextch(env);
        return 1;
    }

    if (env->curchar == ')' && opration_flag)
    {
        operation[*op_size] = ')';
        *op_size = *op_size + 1;
        if (calc_close(stack, is_int, operation, op_size, stk_size))
        {
            env_error(env, incorrect_arithmetic_expression);
            return -1;
        }

        m_nextch(env);
        if (operation[*op_size] == '[')
        {
            *type = LOGIC;
        }
        if (*op_size == 0 && *type == ARITHMETIC)
        {
            double_to_string(env, stack[0], is_int[0]);
            return 0;
        }
        return 1;
    }

    return 2;
}

int calculate(environment *const env, const int type)
{
    int op_size = 0;
    char operation[OPN_SIZE];
    int locl_type = type;

    if (type == ARITHMETIC)
    {
        operation[op_size++] = '(';
        m_nextch(env);
    }

    int stk_size = 0;
    double stack[STK_SIZE];
    int is_int[STK_SIZE];
    int opration_flag = 0;
    while (env->curchar != '\n')
    {
        skip_separators(env);
        const int rez =
            additional_elements(env, stack, is_int, operation, &op_size, &stk_size, &locl_type, opration_flag);
        if (rez == -1 || rez == 0)
        {
            return rez;
        }
        else if (rez == 1)
        {
            continue;
        }

        if (!opration_flag && (utf8_is_digit(env->curchar) || (env->curchar == '-' && utf8_is_digit(env->nextchar))))
        {
            opration_flag = 1;
            if (calc_digit(env, stack, is_int, &stk_size))
            {
                return -1;
            }
        }
        else if (opration_flag && env->curchar != '\n')
        {
            opration_flag = 0;
            if (calc_opiration(env, stack, is_int, operation, &op_size, &stk_size, locl_type))
            {
                return -1;
            }
        }
        else if (env->curchar != '\n')
        {
            env_error(env, third_party_symbol);
            return -1;
        }
    }

    if (locl_type == LOGIC)
    {
        if (calc_close(stack, is_int, operation, &op_size, &stk_size))
        {
            env_error(env, incorrect_arithmetic_expression);
            return -1;
        }

        if (stack[0] == 0)
        {
            env->calc_string[0] = 0;
        }
        else
        {
            env->calc_string[0] = 1;
        }
    }
    else
    {
        env_error(env, in_eval_must_end_parenthesis);
        return -1;
    }

    return 0;
}
