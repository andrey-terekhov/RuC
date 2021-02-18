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
#include "get_macro.h"
#include "linker.h"
#include "utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int flagint = 1;

double get_digit(environment *const env, int* error)
{
	double k;
	int d = 1;

	flagint = 1;
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
			size_t position = env_skip_str(env); 
			macro_error(too_many_nuber, env_get_current_file(env), env->error_string, env->line, position);
			*error = -1;
			return 0.0;
		}
		num = num * 10 + (env->curchar - '0');
		m_nextch(env);
	}

	if (env->curchar == '.')
	{
		flagint = 0;
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
			flagint = 0;
			m_nextch(env);
			sign = -1;
		}
		else if (env->curchar == '+')
		{
			m_nextch(env);
		}


		if (!utf8_is_digit(env->curchar))
		{
			size_t position = env_skip_str(env); 
			macro_error(must_be_digit_after_exp1, env_get_current_file(env), env->error_string, env->line, position);
			*error = -1;
			return 0.0;
		}


		while (utf8_is_digit(env->curchar))
		{
			power = power * 10 + env->curchar - '0';
			m_nextch(env);
		}

		if (flagint)
		{
			for (i = 1; i <= power; i++)
			{
				num *= 10;
			}
		}

		numdouble *= pow(10.0, sign * power);
	}

	if (flagint)
	{
		return num * d;
	}
	else
	{
		return numdouble * d;
	}
}

int check_opiration(environment *const env)
{
	int c = env->curchar;

	if (c == '|' || c == '&' || c == '=' || c == '!')
	{
		if ((env->nextchar == c && c != '!') || (c == '!' && env->nextchar == '='))
		{
			m_nextch(env);
			m_nextch(env);
			return c;
		}
		else
		{
			return 0;
		}
	}
	else if (c == '>' && env->nextchar == '=')
	{
		m_nextch(env);
		m_nextch(env);
		return 'b';
	}
	else if (c == '>' && env->nextchar == '=')
	{
		m_nextch(env);
		m_nextch(env);
		return 's';
	}
	else if (c == '>' || c == '<' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '(')
	{
		m_nextch(env);
		return c;
	}
	else
	{
		return 0;
	}
}

int get_prior(int r)
{
	switch (r)
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

double implementation_opiration(double x, double y, int r, int int_flag)
{
	switch (r)
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
				return 0;
			}
			
		default:
			return 0;
	}
}

void double_to_string(double x, int int_flag, environment *const env)
{
	char s[30] = "\0";

	if (int_flag)
	{
		sprintf(s, "%f", x);
		for (env->csp = 0; env->csp < 20; env->csp++)
		{
			env->cstring[env->csp] = s[env->csp];

			if (s[env->csp] == '.')
			{
				return;
			}
		}
	}
	else
	{
		int l = 0;

		sprintf(s, "%.14lf", x);
		for (env->csp = 0; env->csp < 20; env->csp++)
		{
			env->cstring[env->csp] = s[env->csp];

			if (s[env->csp] != '0' && utf8_is_digit(s[env->csp]))
			{
				l = env->csp;
			}
		}
		env->csp = l + 1;
	}
}

int calculate(const int logic_flag, environment *const env)
{
	int i = 0;
	int op = 0;
	int c = 0;
	double stack[10];
	int int_flag[10];
	int operation[10];
	int opration_flag = 0;

	if (!logic_flag)
	{
		operation[op++] = '(';
		m_nextch(env);
	}

	while (env->curchar != '\n')
	{
		skip_space(env);

		if ((utf8_is_digit(env->curchar) || (env->curchar == '-' && utf8_is_digit(env->nextchar))) && !opration_flag)
		{
			int error = 0;
			opration_flag = 1;
			stack[i] = get_digit(env, &error);
			if (error)
			{
				return -1;
			}
			int_flag[i++] = flagint;
		}
		else if (utf8_is_letter(env->curchar))
		{
			int r = collect_mident(env);

			if (r)
			{
				if (get_macro(r, env))
				{
					return -1;
				}
			}
			else
			{
				size_t position = env_skip_str(env); 
				macro_error(not_macro, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
		}
		else if (env->curchar == '#' && logic_flag)
		{
			env->cur = macro_keywords(env);

			if (env->cur == SH_EVAL && env->curchar == '(')
			{
				if (calculate(0, env))
				{
					return -1;
				}	
			}
			else
			{
				size_t position = env_skip_str(env); 
				macro_error(after_eval_must_be_ckob, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
			m_change_nextch_type(CTYPE, 0, env);
			m_nextch(env);
		}
		else if (env->curchar == ')')
		{
			while (operation[op - 1] != '(')
			{
				if (i < 2 || op == 0)
				{
					size_t position = env_skip_str(env); 
					macro_error(incorrect_arithmetic_expression, env_get_current_file(env), env->error_string, env->line, position);
					return -1;
				}

				int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
				stack[i - 2] = implementation_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
				op--;
				i--;
			}
			op--;
			m_nextch(env);

			if (op == 0 && !logic_flag)
			{
				double_to_string(stack[0], int_flag[0], env);
				return 0;
			}
		}
		else if (opration_flag || env->curchar == '(')
		{
			c = check_opiration(env);
			if (c)
			{
				int n = get_prior(c);
				opration_flag = 0;

				if (n != 0 && logic_flag && n > 3)
				{
					size_t position = env_skip_str(env); 
					macro_error(not_arithmetic_operations, env_get_current_file(env), env->error_string, env->line, position);
					return -1;
				}
				if (n != 0 && !logic_flag && n <= 3)
				{
					size_t position = env_skip_str(env); 
					macro_error(not_logical_operations, env_get_current_file(env), env->error_string, env->line, position);
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
				size_t position = env_skip_str(env); 
				macro_error(third_party_symbol, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}
		}
		else if (env->curchar != '\n')
		{
			size_t position = env_skip_str(env); 
			macro_error(third_party_symbol, env_get_current_file(env), env->error_string, env->line, position);
			return -1;
		}
	}

	if (logic_flag)
	{
		env->csp = 0;
		while (op > 0)
		{
			if (i < 2)
			{
				size_t position = env_skip_str(env); 
				macro_error(incorrect_arithmetic_expression, env_get_current_file(env), env->error_string, env->line, position);
				return -1;
			}

			int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
			stack[i - 2] = implementation_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
			op--;
			i--;
		}

		if (stack[0] == 0)
		{
			env->cstring[0] = 0;
		}
		else
		{
			env->cstring[0] = 1;
		}
	}
	else
	{
		size_t position = env_skip_str(env); 
		macro_error(in_eval_must_end_parenthesis, env_get_current_file(env), env->error_string, env->line, position);
		return -1;
	}
	return 0;
}
