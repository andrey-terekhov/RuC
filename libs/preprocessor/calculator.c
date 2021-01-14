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
#include "define.h"
#include "file.h"
#include "error.h"
#include "utils.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int flagint = 1;

double get_digit(preprocess_context *context, int* error)
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

	while (utf8_is_digit(context->curchar))
	{
		numdouble = numdouble * 10 + (context->curchar - '0');
		if (numdouble > (double)INT_MAX)
		{
			size_t position = skip_str(context); 
			macro_error(too_many_nuber, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			*error = -1;
			return 0.0;
		}
		num = num * 10 + (context->curchar - '0');
		m_nextch(context);
	}

	if (context->curchar == '.')
	{
		flagint = 0;
		m_nextch(context);
		k = 0.1;

		while (utf8_is_digit(context->curchar))
		{
			numdouble += (context->curchar - '0') * k;
			k *= 0.1;
			m_nextch(context);
		}
	}

	if (utf8_is_power(context->curchar))
	{
		int power = 0;
		int sign = 1;
		int i;

		m_nextch(context);
		if (context->curchar == '-')
		{
			flagint = 0;
			m_nextch(context);
			sign = -1;
		}
		else if (context->curchar == '+')
		{
			m_nextch(context);
		}


		if (!utf8_is_digit(context->curchar))
		{
			size_t position = skip_str(context); 
			macro_error(must_be_digit_after_exp1, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			*error = -1;
			return 0.0;
		}


		while (utf8_is_digit(context->curchar))
		{
			power = power * 10 + context->curchar - '0';
			m_nextch(context);
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
			else
			{
				return 0;
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
		int l = 0;

		sprintf(s, "%.14lf", x);
		for (context->csp = 0; context->csp < 20; context->csp++)
		{
			context->cstring[context->csp] = s[context->csp];

			if (s[context->csp] != '0' && utf8_is_digit(s[context->csp]))
			{
				l = context->csp;
			}
		}
		context->csp = l + 1;
	}
}

int calculator(int if_flag, preprocess_context *context)
{
	int i = 0;
	int op = 0;
	int c = 0;
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

		if ((utf8_is_digit(context->curchar) || (context->curchar == '-' && utf8_is_digit(context->nextchar))) && !opration_flag)
		{
			int error = 0;
			opration_flag = 1;
			stack[i] = get_digit(context, &error);
			if(error)
			{
				return -1;
			}
			int_flag[i++] = flagint;
		}
		else if (utf8_is_letter(context->curchar))
		{
			int r = collect_mident(context);

			if (r)
			{
				if(define_get_from_macrotext(r, context))
				{
					return -1;
				}
			}
			else
			{
				size_t position = skip_str(context); 
				macro_error(not_macro, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
		}
		else if (context->curchar == '#' && if_flag)
		{
			context->cur = macro_keywords(context);

			if (context->cur == SH_EVAL && context->curchar == '(')
			{
				
				if(calculator(0, context))
				{
					return -1;
				}	
			}
			else
			{
				size_t position = skip_str(context); 
				macro_error(after_eval_must_be_ckob, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
			m_change_nextch_type(CTYPE, 0, context);
			m_nextch(context);
		}
		else if (context->curchar == ')')
		{
			while (operation[op - 1] != '(')
			{
				if (i < 2 || op == 0)
				{
					size_t position = skip_str(context); 
					macro_error(incorrect_arithmetic_expression, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
					return -1;
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
				return 0;
			}
		}
		else if (opration_flag || context->curchar == '(')
		{
			c = check_opiration(context);
			if (c)
			{
				int n = get_prior(c);
				opration_flag = 0;

				if (n != 0 && if_flag && n > 3)
				{
					size_t position = skip_str(context); 
					macro_error(not_arithmetic_operations, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
					return -1;
				}
				if (n != 0 && !if_flag && n <= 3)
				{
					size_t position = skip_str(context); 
					macro_error(not_logical_operations, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
					return -1;
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
			else if (context->curchar != '\n')
			{
				size_t position = skip_str(context); 
				macro_error(third_party_symbol, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
			}
		}
		else if (context->curchar != '\n')
		{
			size_t position = skip_str(context); 
			macro_error(third_party_symbol, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
			return -1;
		}
	}

	if (if_flag)
	{
		context->csp = 0;
		while (op > 0)
		{
			if (i < 2)
			{
				size_t position = skip_str(context); 
				macro_error(incorrect_arithmetic_expression, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
				return -1;
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
		size_t position = skip_str(context); 
		macro_error(in_eval_must_end_parenthesis, ws_get_file(context->fs.ws, context->fs.cur),  context->error_string, context->line, position);
		return -1;
	}
	return 0;
}
