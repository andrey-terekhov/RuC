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
#include "context.h"
#include "define.h"
#include "defs.h"
#include "file.h"
#include "global.h"
#include "preprocessor_error.h"
#include "preprocessor_utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int flagint = 1;


int is_power(preprocess_context *context)
{
	return context->curchar == 'e' ||
		   context->curchar ==
			   'E'; // || context->curchar == (int)'е' || context->curchar == (int)'Е';	// это русские е и Е
}

double get_digit(preprocess_context *context)
{
	double k;
	int d = 1;

	flagint = 1;
	int num = 0;
	double numdouble = 0.0;
	if (context->curchar == '-')
	{
		d = -1;
		m_nextch(context);
	}

	while (is_digit(context->curchar))
	{
		numdouble = numdouble * 10 + (context->curchar - '0');
		if (numdouble > (double)INT_MAX)
		{
			m_error(too_many_nuber, context);
		}
		num = num * 10 + (context->curchar - '0');
		m_nextch(context);
	}

	if (context->curchar == '.')
	{
		flagint = 0;
		m_nextch(context);
		k = 0.1;

		while (is_digit(context->curchar))
		{
			numdouble += (context->curchar - '0') * k;
			k *= 0.1;
			m_nextch(context);
		}
	}

	if (is_power(context))
	{
		int d = 0;
		int k = 1;
		int i;

		m_nextch(context);
		if (context->curchar == '-')
		{
			flagint = 0;
			m_nextch(context);
			k = -1;
		}
		else if (context->curchar == '+')
		{
			m_nextch(context);
		}


		if (!is_digit(context->curchar))
		{
			m_error(must_be_digit_after_exp1, context);
		}


		while (is_digit(context->curchar))
		{
			d = d * 10 + context->curchar - '0';
			m_nextch(context);
		}

		if (flagint)
		{
			for (i = 1; i <= d; i++)
			{
				num *= 10;
			}
		}

		numdouble *= pow(10.0, k * d);
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

int check_opiration(preprocess_context *context)
{
	int c = context->curchar;

	if (c == '|' || c == '&' || c == '=' || c == '!')
	{
		if ((context->nextchar == c && c != '!') || (c == '!' && context->nextchar == '='))
		{
			m_nextch(context);
			m_nextch(context);
			return c;
		}
		else
		{
			return 0;
		}
	}
	else if (c == '>' && context->nextchar == '=')
	{
		m_nextch(context);
		m_nextch(context);
		return 'b';
	}
	else if (c == '>' && context->nextchar == '=')
	{
		m_nextch(context);
		m_nextch(context);
		return 's';
	}
	else if (c == '>' || c == '<' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '(')
	{
		m_nextch(context);
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

double relis_opiration(double x, double y, int r, int int_flag)
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
		default:
			return 0;
	}
}

void double_to_string(double x, int int_flag, preprocess_context *context)
{
	char s[30] = "\0";

	if (int_flag)
	{
		sprintf(s, "%f", x);
		for (context->csp = 0; context->csp < 20; context->csp++)
		{
			context->cstring[context->csp] = s[context->csp];

			if (s[context->csp] == '.')
			{
				return;
			}
		}
	}
	else
	{
		int l;

		sprintf(s, "%.14lf", x);
		for (context->csp = 0; context->csp < 20; context->csp++)
		{
			context->cstring[context->csp] = s[context->csp];

			if (s[context->csp] != '0' && is_digit(s[context->csp]))
			{
				l = context->csp;
			}
		}
		context->csp = l + 1;
	}
}

void calculator(int if_flag, preprocess_context *context)
{
	int i = 0;
	int op = 0;
	int c;
	double stack[10];
	int int_flag[10];
	int operation[10];
	int opration_flag = 0;

	if (!if_flag)
	{
		operation[op++] = '(';
		m_nextch(context);
	}

	while (context->curchar != '\n')
	{
		space_skip(context);

		if ((is_digit(context->curchar) || (context->curchar == '-' && is_digit(context->nextchar))) && !opration_flag)
		{
			opration_flag = 1;
			stack[i] = get_digit(context);
			int_flag[i++] = flagint;
		}
		else if (is_letter(context))
		{
			int r = collect_mident(context);

			if (r)
			{
				define_get_from_macrotext(r, context);
			}
			else
			{
				m_error(1, context);
			}
		}
		else if (context->curchar == '#' && if_flag)
		{
			context->cur = macro_keywords(context);

			if (context->cur == SH_EVAL && context->curchar == '(')
			{
				calculator(0, context);
			}
			else
			{
				m_error(after_eval_must_be_ckob, context);
			}
			m_change_nextch_type(CTYPE, 0, context);
			m_nextch(context);
		}
		else if ((opration_flag || context->curchar == '(') && (c = check_opiration(context)))
		{
			int n = get_prior(c);
			opration_flag = 0;

			if (n != 0 && if_flag && n > 3)
			{
				m_error(not_arithmetic_operations, context);
			}
			if (n != 0 && !if_flag && n <= 3)
			{
				m_error(not_logical_operations, context);
			}

			while (op != 0 && n != 0 && get_prior(operation[op - 1]) >= n)
			{
				int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
				stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
				op--;
				i--;
			}
			operation[op++] = c;
		}
		else if (context->curchar == ')')
		{
			while (operation[op - 1] != '(')
			{
				if (i < 2 || op == 0)
				{
					m_error(2, context);
				}

				int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
				stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
				op--;
				i--;
			}
			op--;
			m_nextch(context);

			if (op == 0 && !if_flag)
			{
				double_to_string(stack[0], int_flag[0], context);
				return;
			}
		}
		else if (context->curchar != '\n')
		{
			m_error(3, context);
		}
	}

	if (if_flag)
	{
		context->csp = 0;
		while (op > 0)
		{
			if (i < 2)
			{
				m_error(4, context);
			}

			int_flag[i - 2] = int_flag[i - 2] && int_flag[i - 1];
			stack[i - 2] = relis_opiration(stack[i - 2], stack[i - 1], operation[op - 1], int_flag[i - 2]);
			op--;
			i--;
		}

		if (stack[0] == 0)
		{
			context->cstring[0] = 0;
		}
		else
		{
			context->cstring[0] = 1;
		}
	}
	else
	{
		m_error(5, context);
	}
}
