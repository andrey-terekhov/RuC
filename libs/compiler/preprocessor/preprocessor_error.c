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
#include "logger.h"
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

void errors_set(int ernum, const char *const tag)
{
	switch (ernum)
	{
		case after_preproces_words_must_be_space:
			log_system_error(tag, "после команды препроцессора должен идти перенос строки");
			break;
		case after_ident_must_be_space1:
			log_system_error(tag, "после идентификатора должен идти ' ' ");
			break;
		case ident_begins_with_letters1:
			log_system_error(tag, "идентификатор должен начинаться с буквы ");
			break;
		case must_be_endif:
			log_system_error(tag, "условный оператор препроцессора должен заканчиваться '#ENDIF' ");
			break;
		case dont_elif:
			log_system_error(tag, "в этом типе условного оператора не может использоваться '#ELIF' ");
			break;
		case preproces_words_not_exist:
			log_system_error(tag, "в препроцессоре не существует написанной команды");
			break;
		case not_enough_param:
			log_system_error(tag, "у этого идентификатора меньше параметров");
			break;
		case functionid_begins_with_letters:
			log_system_error(tag, "идентификатор с параметрами должен начинаться с буквы");
			break;
		case functions_cannot_be_changed:
			log_system_error(tag, "идентификатор с параметрами нельзя переопределять");
			break;
		case after_functionid_must_be_comma:
			log_system_error(tag, "после идентификатора в функции должны быть ')' или ',' потом ' ' ");
			break;
		case stalpe:
			log_system_error(tag, "в функции аргументы должны быть описаны через запятую, в скобках");
			break;
		case not_relis_if:
			log_system_error(tag, "if ещё не реализован");
			break;
		case before_endif:
			log_system_error(tag, "перед '#ENDIF' должен стоять условный оператор препроцессора");
			break;
		case repeat_ident:
			log_system_error(tag, "этот идентификатор препроцессора уже используется");
			break;
		case ident_not_exist:
			log_system_error(tag, "данный идентификатор препроцессора не существует");
			break;
		case comm_not_ended:
			log_system_error(tag, "комментарий, начавшийся с /* , не закрыт");
			break;
		case not_enough_param2:
			log_system_error(tag, "у этой функции больше параметров");
			break;
		case not_end_fail_define:
			log_system_error(tag, "файл не может закончится до окончания команды '#DEFINE' поставьте перенос строки");
			break;
		case scob_not_clous:
			log_system_error(tag, "количество открывающих скобок не соответствует числу закрывающих");
			break;
		case after_eval_must_be_ckob:
			log_system_error(tag, "сразу после команды '#EVAL' должен быть символ '('");
			break;
		case too_many_nuber:
			log_system_error(tag, "слишком большое число");
			break;
		case must_be_digit_after_exp1:
			log_system_error(tag, "после экспоненты должно быть число");
			break;
		case not_arithmetic_operations:
			log_system_error(tag, "все арифметические операции должны быть внутри команды '#EVAL()'");
			break;
		case not_logical_operations:
			log_system_error(tag, "внутри команды '#EVAL()' не должно быть логических операций");
			break;
		default:
		{
			char msg[128];
			sprintf(msg, "не реализованная ошибка №%d", ernum);
			log_system_error(tag, msg);
		}
	}
}

void m_error(int ernum, preprocess_context *context)
{
	/*if ()
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

#if MACRODEBUG
		printf("\n\n");
#endif
		errors_set(ernum, name);

#if MACRODEBUG
		// int j = 2;
		// printf("line 1) ");
		printf(" ");


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
		errors_set(ernum, "macro");
	}

	exit(2);*/
}
