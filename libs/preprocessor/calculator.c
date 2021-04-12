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
#include <math.h>
#include <string.h>
#include "environment.h"
#include "error.h"
#include "linker.h"
#include "macro_load.h"
#include "uniio.h"
#include "uniscanner.h"
#include "utils.h"


#define STACK_SIZE 	32
#define OPERATION_SIZE 	STACK_SIZE

#define LOGIC 		0
#define ARITHMETIC	1

#define  LESS_EQUAL 'l'
#define  GREATER_EQUAL 'g'


typedef struct calculator_struct
{
	size_t op_size;
	char32_t operation[OPERATION_SIZE];
	size_t stk_size;
	int stack[STACK_SIZE];
} calculator_struct;


char32_t get_operation(const char32_t cur, const char32_t next)
{
	switch (cur)
	{
		case '|':
		case '&':
		case '=':
			if (next == cur)
			{
				return cur;
			}
			else
			{
				return '\0';
			}
		case '!':
			if ( next == '=')
			{
				return cur;
			}
			else
			{
				return '\0';
			}
		case '>':
		case '<':
			if (cur == '<' && next == '=')
			{
				return LESS_EQUAL;
			}
			else if (cur == '>' && next == '=')
			{
				return GREATER_EQUAL;
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

int get_priority(const char32_t operation, const int type)
{
	if(type == LOGIC)
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
			case LESS_EQUAL:
			case GREATER_EQUAL:
			case '=':
			case '!':
				return 3;
			default:
				return -1;
		}
	}
	else
	{
		switch (operation)
		{
			case '(':
				return 0;
			case '+':
			case '-':
				return 4;
			case '*':
			case '/':
			case '%':
				return 5;
			default:
				return -1;
		}
	}
	
}

int calc_count(const int x, const int y, const char32_t operation)
{
	switch (operation)
	{
		case '<':
			return x < y;
		case '>':
			return x > y;
		case LESS_EQUAL:
			return x <= y;
		case GREATER_EQUAL:
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
			return x / y;
		case '%':
			return x % y;
		default:
			return 0;
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

int calc_digit(environment *const env, calculator_struct *const calc)
{
	if (env->curchar == '#')
	{
		const int cur = macro_keywords(env);
		if (cur == SH_EVAL && env->curchar == '(')
		{
			if (calculate_arithmetic(env, &calc->stack[calc->stk_size]))
			{
				return -1;
			}
			calc->stk_size++;
			return 0;
		}
		else
		{
			env_error(env, after_eval_must_be_ckob);
			return -1;
		}
	}

	int num = 0;
	int sign = 1;

	if (env->curchar == '-')
	{
		sign = -1;
	}
	while (utf8_is_digit(env->curchar))
	{
		num = num * 10 + (env->curchar - '0');
		m_nextch(env);
	}

	calc->stack[calc->stk_size++] = num * sign;
	return 0;
}

int calc_operation(environment *const env, calculator_struct *const calc, const int type)
{
	const char32_t opr = get_operation(env->curchar, env->nextchar);
	if (opr == '\0')
	{
		env_error(env, third_party_symbol);
		return -1;
	}

	m_nextch(env);
	if (opr == 'b'|| opr == 's' || opr == '=' || opr == '&'|| opr == '|' || opr == '!')
	{
		m_nextch(env);
	}

	const int priority = get_priority(opr, type);
	if (type == LOGIC && priority == -1)
	{
		env_error(env, not_arithmetic_operations);
		return -1;
	}
	if (type == ARITHMETIC && priority == -1)
	{
		env_error(env, not_logical_operations);
		return -1;
	}

	while (calc->op_size != 0 && get_priority(calc->operation[calc->op_size - 1], type) >= priority)
	{
		calc->stack[calc->stk_size - 2] = calc_count(calc->stack[calc->stk_size - 2], calc->stack[calc->stk_size - 1], calc->operation[calc->op_size - 1]);
		calc->op_size--;
		calc->stk_size--;
	}

	calc->operation[calc->op_size++] = opr;

	return 0;
}

int calc_close(calculator_struct *const calc)
{
	int scope_flag = 0;
	if (calc->operation[calc->op_size - 1] == ')')
	{
		scope_flag++;
		calc->op_size = calc->op_size - 1;
	}

	while ((scope_flag && calc->operation[calc->op_size - 1] != '(') || (!scope_flag && calc->op_size > 0))
	{
		if (calc->stk_size < 2 || calc->op_size == 0)
		{	
			return -1;
		}

		calc->stack[calc->stk_size - 2] = calc_count(calc->stack[calc->stk_size - 2], calc->stack[calc->stk_size - 1], calc->operation[calc->op_size - 1]);
		calc->op_size--;
		calc->stk_size--;
	}

	calc->op_size--;
	return 0;
}

int additional_elements(environment *const env, calculator_struct *const calc, const int type, const int operation_flag)
{
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
		calc->operation[calc->op_size++] = '(';
		m_nextch(env);
		return 1;
	}

	if (env->curchar == ')' && operation_flag)
	{
		calc->operation[calc->op_size++] = ')';
		if (calc_close(calc))
		{
			env_error(env, incorrect_arithmetic_expression);
			return -1;
		}

		m_nextch(env);
		if (calc->op_size == 0 && type == ARITHMETIC)
		{
			return 0;
		}
		return 1;
	}

	return 2;
}

int calculate(environment *const env, int *const result)
{
	
	calculator_struct calc;
	calc.op_size = 0;
	calc.stk_size = 0;

	int expression_type = LOGIC;
	if (result != NULL)
	{
		expression_type = ARITHMETIC;
		calc.operation[calc.op_size++] = '(';
		m_nextch(env);
	}

	
	int operation_flag = 0;
	
	while (env->curchar != '\n')
	{
		skip_separators(env);
		const int res = additional_elements(env, &calc, expression_type, operation_flag);
		if (res == -1)
		{
			return -1;
		} 
		else if(res == 0)
		{
			
			*result = calc.stack[0];
			return 0;
		}
		else if (res == 1)
		{
			continue;
		}

		if (!operation_flag && (utf8_is_digit(env->curchar) || (env->curchar == '-' && utf8_is_digit(env->nextchar))
			|| (env->curchar == '#' && expression_type == LOGIC)))
		{
			operation_flag = 1;
			calc_digit(env, &calc);
		}
		else if (operation_flag && env->curchar != '\n')
		{
			operation_flag = 0;
			if (calc_operation(env, &calc, expression_type))
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

	if (expression_type == LOGIC)
	{
		if (calc_close(&calc))
		{
			env_error(env, incorrect_arithmetic_expression);
			return -1;
		}

		return calc.stack[0] == 0 ? 0 : 1;
	}
	else
	{
		env_error(env, in_eval_must_end_parenthesis);
		return -1;
	}
}

int calculate_arithmetic(environment *const env, int *const result)
{
	return calculate(env, result);
}

int calculate_logic(environment *const env)
{
	return calculate(env, NULL);
}
