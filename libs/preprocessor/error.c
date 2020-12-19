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

#include "error.h"
#include "logger.h"
#include <limits.h>
#include <stdio.h>
#include <stddef.h>


#define TAG_MACRO "macro"
#define STRIGSIZE	256


void get_message_error(int ernum, char *const msg)
{
	switch (ernum)
	{
		case after_preproces_words_must_be_space:
			sprintf(msg, "после команды препроцессора должен идти перенос строки");
			break;
		case after_ident_must_be_space:
			sprintf(msg, "после идентификатора должен идти ' ' ");
			break;
		case ident_begins_with_letters:
			sprintf(msg, "идентификатор должен начинаться с буквы ");
			break;
		case must_be_endif:
			sprintf(msg, "условный оператор препроцессора должен заканчиваться '#ENDIF' ");
			break;
		case dont_elif:
			sprintf(msg, "в этом типе условного оператора не может использоваться '#ELIF' ");
			break;
		case preproces_words_not_exist:
			sprintf(msg, "в препроцессоре не существует написанной команды");
			break;
		case not_enough_param:
			sprintf(msg, "у этого идентификатора меньше параметров");
			break;
		case functionid_begins_with_letters:
			sprintf(msg, "идентификатор с параметрами должен начинаться с буквы");
			break;
		case functions_cannot_be_changed:
			sprintf(msg, "идентификатор с параметрами нельзя переопределять");
			break;
		case after_functionid_must_be_comma:
			sprintf(msg, "после идентификатора в функции должны быть ')' или ',' потом ' ' ");
			break;
		case stalpe:
			sprintf(msg, "в функции аргументы должны быть описаны через запятую, в скобках");
			break;
		case before_endif:
			sprintf(msg, "перед '#ENDIF' должен стоять условный оператор препроцессора");
			break;
		case repeat_ident:
			sprintf(msg, "этот идентификатор препроцессора уже используется");
			break;
		case ident_not_exist:
			sprintf(msg, "данный идентификатор препроцессора не существует");
			break;
		case comm_not_ended:
			sprintf(msg, "комментарий, начавшийся с /* , не закрыт");
			break;
		case not_enough_param2:
			sprintf(msg, "у этой функции больше параметров");
			break;
		case not_end_fail_define:
			sprintf(msg, "файл не может закончится до окончания команды '#DEFINE' поставьте перенос строки");
			break;
		case scob_not_clous:
			sprintf(msg, "количество открывающих скобок не соответствует числу закрывающих");
			break;
		case after_eval_must_be_ckob:
			sprintf(msg, "сразу после команды '#EVAL' должен быть символ '('");
			break;
		case too_many_nuber:
			sprintf(msg, "слишком большое число");
			break;
		case must_be_digit_after_exp1:
			sprintf(msg, "после экспоненты должно быть число");
			break;
		case not_arithmetic_operations:
			sprintf(msg, "все арифметические операции должны быть внутри команды '#EVAL()'");
			break;
		case not_logical_operations:
			sprintf(msg, "внутри команды '#EVAL()' не должно быть логических операций");
			break;
		default:
			sprintf(msg, "не реализованная ошибка №%d", ernum);
	}
}

void macro_error(const int num, const char *const path, const size_t line, const char *const code, size_t position)
{
	char tag[STRIGSIZE] = TAG_MACRO;

	
	size_t index = sprintf(tag, "%s", path);
	index += sprintf(&tag[index], ":%zi", line);
	
	while (position > 0
		&& (code[position] == ' ' || code[position] == '\t'
		|| code[position] == '\r' || code[position] == '\n'))
	{
		position--;
	}
	sprintf(&tag[index], ":%zi", position);

	char msg[STRIGSIZE];
	get_message_error(num, msg);

	log_error(tag, msg, code, position);
}
