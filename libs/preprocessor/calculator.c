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
#include "macro_load.h"
#include "linker.h"
#include "utils.h"
#include <math.h>


double get_digit(environment *const env, int* error)// Временная замена
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

double implementation_opiration(const double x, const double y, const char opiration, const int int_flag)
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
			if (int_flag)
			{
				return (int)x / (int)y;
			}
			else
			{
				return x / y;
			}
		case '%':
			if (int_flag)
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

void double_to_string(environment *const env, const double x, const int int_flag)
{
	char s[30] = "\0";

	if (int_flag)
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
				l = env->calc_string_size;
			}
		}
		env->calc_string_size = l + 1;
	}
}

int calculate(environment *const env, const int type)
{
	int op = 0;
	char operation[10];

	if (!type)
	{
		operation[op++] = '(';
		m_nextch(env);
	}

	int i = 0;
	double stack[10];
	int int_flag[10];
	int opration_flag = 0;
	while (env->curchar != '\n')
	{
		skip_to_significant_character(env);

		if ((utf8_is_digit(env->curchar) || (env->curchar == '-' && utf8_is_digit(env->nextchar))) && !opration_flag)
		{
			int error = 0;
			opration_flag = 1;
			stack[i] = get_digit(env, &error);
			if (error)
			{
				return -1;
			}
			int_flag[i++] = env->flagint;
		}
		else if (utf8_is_letter(env->curchar))
		{
			const int macros_ptr = collect_mident(env);
			if (!macros_ptr)
			{
				env_error(env, not_macro);
				return -1;
			}

			if(macros_get(env, macros_ptr))
			{
				return -1;
			}

		}
		else if (env->curchar == '#' && type)
		{
			env->cur = macro_keywords(env);

			if (env->cur == SH_EVAL && env->curchar == '(')
			{
				if (calculate(env, 0))
				{
					return -1;
				}
			}
			else
			{
				env_error(env, after_eval_must_be_ckob);
				return -1;
			}
			m_change_nextch_type(env, CTYPE, 0);
			m_nextch(env);
		}
		else if (env->curchar == ')')
		{
			while (operation[op - 1] != '(')
			{
				if (i < 2 || op == 0)
				{
					env_error(env, incorrect_arithmetic_expression);
					return -1;
				}

				int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
				stack[i - 2] = implementation_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
				op--;
				i--;
			}
			op--;
			m_nextch(env);

			if (op == 0 && !type)
			{
				double_to_string(env, stack[0], int_flag[0]);
				return 0;
			}
		}
		else if (opration_flag || env->curchar == '(')
		{
			const char c = get_opiration((char)env->curchar, (char)env->nextchar);
			if (c)
			{
				m_nextch(env);
				if (c == 'b'|| c == 's' || c == '=' || c == '&'|| c == '|' || c == '!')
				{
					m_nextch(env);
				}
				int n = get_prior(c);
				opration_flag = 0;

				if (n != 0 && type && n > 3)
				{
					env_error(env, not_arithmetic_operations);
					return -1;
				}
				if (n != 0 && !type && n <= 3)
				{
					env_error(env, not_logical_operations);
					return -1;
				}

				while (op != 0 && n != 0 && get_prior(operation[op - 1]) >= n)
				{
					int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
					stack[i - 2] = implementation_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
					op--;
					i--;
				}
				operation[op++] = c;
			}
			else if (env->curchar != '\n')
			{
				env_error(env, third_party_symbol);
				return -1;
			}
		}
		else if (env->curchar != '\n')
		{
			env_error(env, third_party_symbol);
			return -1;
		}
	}

	if (type)
	{
		env->calc_string_size = 0;
		while (op > 0)
		{
			if (i < 2)
			{
				env_error(env, incorrect_arithmetic_expression);
				return -1;
			}

			int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
			stack[i - 2] = implementation_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
			op--;
			i--;
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
