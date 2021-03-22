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
#include "uniio.h"
#include "uniscanner.h"
#include "string.h"
#include <math.h>


#define STK_SIZE 32
#define OPN_SIZE STK_SIZE

#define DOUBLE	0
#define INT		1
#define WARNING	-2
#define ERROR	-1

int get_digit(universal_io *in, double *rez, int cur, int next, int *last)
{
	if (in == NULL || rez == NULL || (cur != '-'  && !utf8_is_digit(cur)))
	{
		return ERROR;
	}

	int flag_int = INT;
	int num_int = 0;
	int sign = 1;
	int flag_too_long = 0;
	double num_double = 0.0;

	if (cur == '-')
	{
		sign = -1;
	}
	else
	{
		num_double = num_double * 10 + (cur - '0');
		num_int = num_int * 10 + (cur - '0');
	}

	char32_t curchar = next;

	while (utf8_is_digit(curchar))
	{
		num_double = num_double * 10 + (curchar - '0');
		num_int = num_int * 10 + (curchar - '0');
		curchar = uni_scan_char(in);
	}

	if (num_double > (double)INT_MAX)
	{
		flag_too_long = 1;
		flag_int = DOUBLE;
	}

	if (curchar == '.')
	{
		double pow = 0.1;
		flag_int = DOUBLE;
		curchar = uni_scan_char(in);
		while (utf8_is_digit(curchar))
		{
			num_double += (curchar - '0') * pow;
			pow *= 0.1;
			curchar = uni_scan_char(in);
		}
	}

	if (utf8_is_power(curchar))
	{
		int power = 0;
		int sign_power = 1;
		curchar = uni_scan_char(in);
		if (curchar == '-')
		{
			flag_int = DOUBLE;
			curchar = uni_scan_char(in);
			sign_power = -1;
		}
		else if (curchar == '+')
		{
			curchar = uni_scan_char(in);
		}

		if (!utf8_is_digit(curchar))
		{
			return ERROR;
		}
		while (utf8_is_digit(curchar))
		{
			power = power * 10 + curchar - '0';
			curchar = uni_scan_char(in);
		}

		if (flag_int == INT)
		{
			for (int i = 1; i <= power; i++)
			{
				num_int *= 10;
			}
		}

		num_double *= pow(10.0, sign_power * power);
	}

	if (flag_int)
	{
		*rez = num_int * sign;
	}
	else
	{
		*rez = num_double * sign;
	}
	
	if (last != NULL)
	{
		*last = curchar;
	}

	if (flag_too_long)
	{
		return WARNING;
	}

	return flag_int;
}

char get_operation(const char cur, const char next)
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

int get_prior(const char operation)
{
	switch (operation)
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

double calc_count(const double x, const double y, const char operation, const int is_int)
{
	switch (operation)
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
	if (is_int)
	{
		sprintf(env->calc_string, "%f", x);
		char *dot = strchr(env->calc_string, '.');
		if(dot != 0)
		{
			dot[0] = '\0';
		}
	}
	else
	{	
		sprintf(env->calc_string, "%.14lf", x);
		size_t lenght = strlen(env->calc_string) - 1;
		while (env->calc_string[lenght] == '0' || !utf8_is_digit(env->calc_string[lenght]))
		{
			lenght--;
		}
		env->calc_string[lenght + 1] = '\0';
	}

	env->calc_string_size = strlen(env->calc_string);
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
	int rez;
	if (env->nextch_type != FILE_TYPE)
	{
		char buffer[STRING_SIZE];
		buffer[0] = '\0';
		size_t buffer_size = 0;

		int cur = env->curchar;
		m_nextch(env);

		int next = env->curchar;
		if (utf8_is_digit(env->curchar))
		{
			m_nextch(env);
			while (utf8_is_digit(env->curchar) || utf8_is_power(env->curchar))
			{
				if(utf8_is_power(env->curchar) &&  (env->nextchar == '+' || env->curchar == '-'))
				{
					buffer_size += utf8_to_string(&buffer[buffer_size], env->curchar);
					m_nextch(env);
				}
				buffer_size += utf8_to_string(&buffer[buffer_size], env->curchar);
				m_nextch(env);
			}
		}
		universal_io in = io_create();
		in_set_buffer(&in, buffer);
		rez = get_digit(&in, &stack[*stk_size], cur, next, NULL);
		in_clear(&in);
	}
	else
	{
		rez = get_digit(env->input, &stack[*stk_size], env->curchar, env->nextchar, &env->curchar);
		get_next_char(env);
	}
	
	
	if (rez == ERROR)
	{
		env_error(env, must_be_digit_after_exp1);
		return -1;
	}
	else if (rez == WARNING)
	{
		macro_error(too_many_nuber, lk_get_current(env->lk), env->error_string, env->line, env->position);
		is_int[*stk_size] = DOUBLE;
	}
	else
	{
		is_int[*stk_size] = rez;
	}

	*stk_size = *stk_size + 1;
	return 0;
}

int calc_operation(environment *const env, double *const stack, int *const is_int, char *const operation, int *op_size, int *stk_size, const int type)
{
	const char opr = get_operation((char)env->curchar, (char)env->nextchar);
	if (!opr)
	{
		env_error(env, third_party_symbol);
		return -1;
	}

	m_nextch(env);
	if (opr == 'b'|| opr == 's' || opr == '=' || opr == '&'|| opr == '|' || opr == '!')
	{
		m_nextch(env);
	}

	const int prior = get_prior(opr);
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
		stack[*stk_size - 2] = calc_count(stack[*stk_size - 2], stack[*stk_size - 1], operation[*op_size - 1], is_int[*stk_size - 2]);
		*op_size = *op_size - 1;
		*stk_size = *stk_size - 1;
	}

	operation[*op_size] = opr;
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

	while ((scope_flag && operation[*op_size - 1] != '(' && operation[*op_size - 1] != '[') || (!scope_flag && *op_size > 0))
	{
		if (*stk_size < 2 || *op_size == 0)
		{	
			return -1;
		}

		is_int[*stk_size - 2] = is_int[*stk_size - 2] && is_int[*stk_size - 1];
		stack[*stk_size - 2] = calc_count(stack[*stk_size - 2], stack[*stk_size - 1], operation[*op_size - 1], is_int[*stk_size - 2]);
		*op_size = *op_size - 1;
		*stk_size = *stk_size - 1;
	}

	*op_size = *op_size - 1;
	return 0;
}

int additional_elements(environment *const env, double *const stack, int *const is_int
	, char *const operation , int *op_size, int *stk_size, int *type, int operation_flag)
{
	if (env->curchar == '#' && *type == LOGIC && !operation_flag)
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
			env_error(env, after_eval_must_be_ckob);//
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

	if (env->curchar == '(' && !operation_flag)
	{
		operation[*op_size] = '(';
		*op_size = *op_size + 1;
		m_nextch(env);
		return 1;
	}

	if (env->curchar == ')' && operation_flag)
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

	if (type == ARITHMETIC)
	{
		operation[op_size++] = '(';
		m_nextch(env);
	}

	int stk_size = 0;
	double stack[STK_SIZE];
	int is_int[STK_SIZE];
	int operation_flag = 0;
	int local_type = type;
	while (env->curchar != '\n')
	{
		skip_separators(env);
		const int rez = additional_elements(env, stack, is_int, operation, &op_size, &stk_size, &local_type, operation_flag);
		if (rez == -1 || rez == 0)
		{
			return rez;
		} 
		else if (rez == 1)
		{
			continue;
		}

		if (!operation_flag && (utf8_is_digit(env->curchar) || (env->curchar == '-' && utf8_is_digit(env->nextchar))))
		{
			operation_flag = 1;
			if (calc_digit(env, stack, is_int, &stk_size))
			{
				return -1;
			}
		}
		else if (operation_flag && env->curchar != '\n')
		{
			operation_flag = 0;
			if (calc_operation(env, stack, is_int, operation, &op_size, &stk_size, local_type))
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

	if (local_type == LOGIC)
	{
		if (calc_close(stack, is_int, operation, &op_size, &stk_size))
		{
			env_error(env, incorrect_arithmetic_expression);
			return -1;
		}

		if (stack[0] == 0)
		{
			env->calc_string[0] = '0';
		}
		else
		{
			env->calc_string[0] = '1';
		}
	}
	else
	{
		env_error(env, in_eval_must_end_parenthesis);
		return -1;
	}

	return 0;
}
