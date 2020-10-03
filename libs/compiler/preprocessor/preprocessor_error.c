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

#include "preprocessor_error.h"
#include "constants.h"
#include "context.h"
#include "context_var.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void printf_character(int wchar)
{
	if (wchar < 0)
	{
		return;
	}

	if (wchar < 128)
	{
		printf("%c", wchar);
	}
	else
	{
		unsigned char first = (wchar >> 6) | /*0b11000000*/ 0xC0;
		unsigned char second = (wchar & /*0b111111*/ 0x3F) | /*0b10000000*/ 0x80;

		printf("%c%c", first, second);
	}
}

void errors_set(int ernum)
{
	switch (ernum)
	{
		case after_preproces_words_must_be_space:
			fprintf(stderr, "после команды препроцессора должен идти перенос строки\n");
			break;
		case after_ident_must_be_space1:
			fprintf(stderr, "после идентификатора должен идти ' ' \n");
			break;
		case ident_begins_with_letters1:
			fprintf(stderr, "идентификатор должен начинаться с буквы \n");
			break;
		case must_be_endif:
			fprintf(stderr, "условный оператор препроцессора должен заканчиваться '#ENDIF' \n");
			break;
		case dont_elif:
			fprintf(stderr, "в этом типе условного оператора не может использоваться '#ELIF' \n");
			break;
		case preproces_words_not_exist:
			fprintf(stderr, "в препроцессоре не существует написанной команды\n");
			break;
		case not_enough_param:
			fprintf(stderr, "у этого идентификатора меньше параметров\n");
			break;
		case functionid_begins_with_letters:
			fprintf(stderr, "идентификатор с параметрами должен начинаться с буквы\n");
			break;
		case functions_cannot_be_changed:
			fprintf(stderr, "идентификатор с параметрами нельзя переопределять\n");
			break;
		case after_functionid_must_be_comma:
			fprintf(stderr, "после идентификатора в функции должны быть ')' или ',' потом ' ' \n");
			break;
		case stalpe:
			fprintf(stderr, "в функции аргументы должны быть описаны через запятую, в скобках\n");
			break;
		case not_relis_if:
			fprintf(stderr, "if ещё не реализован");
			break;
		case before_endif:
			fprintf(stderr, "перед '#ENDIF' должен стоять условный оператор препроцессора\n");
			break;
		case repeat_ident:
			fprintf(stderr, "этот идентификатор препроцессора уже используется\n");
			break;
		case ident_not_exist:
			fprintf(stderr, "данный идентификатор препроцессора не существует\n");
			break;
		case comm_not_ended:
			fprintf(stderr, "комментарий, начавшийся с /* , не закрыт\n");
			break;
		case not_enough_param2:
			fprintf(stderr, "у этой функции больше параметров\n");
			break;
		case not_end_fail_define:
			fprintf(stderr, "файл не может закончится до окончания команды '#DEFINE' поставьте перенос строки\n");
			break;
		case scob_not_clous:
			fprintf(stderr, "количество открывающих скобок не соответствует числу закрывающих\n");
			break;
		case after_eval_must_be_ckob:
			fprintf(stderr, "сразу после команды '#EVAL' должен быть символ '('\n");
			break;
		case too_many_nuber:
			fprintf(stderr, "слишком большое число\n");
			break;
		case must_be_digit_after_exp1:
			fprintf(stderr, "после экспоненты должно быть число\n");
			break;
		case not_arithmetic_operations:
			fprintf(stderr, "все арифметические операции должны быть внутри команды '#EVAL()'\n");
			break;
		case not_logical_operations:
			fprintf(stderr, "внутри команды '#EVAL()' не должно быть логических операций\n");
			break;
		default:
			fprintf(stderr, "не реализованная ошибка №%d\n", ernum);
	}
}

void m_error(int ernum, preprocess_context *context)
{
	if (context->before_temp != NULL)
	{
		int i = 0;
		data_file *f;

		if (context->h_flag)
		{
			f = &context->headers->files[context->headers->cur];
		}
		else
		{
			f = &context->sources->files[context->sources->cur];
		}

		const char *name = f->name;
		int *s = context->before_temp->str;
		int p = context->before_temp->p;

#if MACRODEBUG
		printf("\n\n");
#endif
		fprintf(stderr, "\x1B[1;39m%s:\x1B[1;31m Ошибка:\x1B[0m ", name);
		errors_set(ernum);

#if MACRODEBUG
		// int j = 2;
		// printf("line 1) ");
		printf(" ");

		if ((f)->include_source.str[0] != '\0')
		{
			int *s2 = (f)->include_source.str;
			while (s2[i] != '\0')
			{
				printf_character(s[i]);
				if (s2[i] == '\n')
				{
					printf(" ");
					// printf("line %i) ", j);
					// j++;
				}
				i++;
			}
		}

		for (i = 0; i < p; i++)
		{
			printf_character(s[i]);
			if (s[i] == '\n')
			{
				printf(" ");
				// printf("line %i) ", j);
				// j++;
			}
		}
		printf("\n");
#endif
	}
	else
	{
		fprintf(stderr, "\x1B[1;39mruc:\x1B[1;31m ошибка:\x1B[0m ");
		errors_set(ernum);
	}

	exit(2);
}
