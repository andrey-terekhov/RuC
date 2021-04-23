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

#include "constants.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stddef.h>


#define TAG_MACRO		"macro"

#define ERROR_TAG_SIZE	STRING_SIZE
#define ERROR_MSG_SIZE	STRING_SIZE


void get_message_error(const int num, char *const msg)
{
	switch (num)
	{
		case preprocess_word_not_exists: // test_exist preproces_words_not_exist
			sprintf(msg, "в препроцессоре не существует такой команды");
			break;
		case after_preproces_words_must_be_space: // test_exist
			sprintf(msg, "после команды препроцессора должен идти перенос строки");
			break;
		case ident_must_begins_with_letter: // test_exist ident_begins_with_letters
			sprintf(msg, "идентификатор должен начинаться с буквы ");
			break;
		case ident_is_repeated: // test_exist repeat_ident
			sprintf(msg, "этот идентификатор препроцессора уже используется");
			break;
		case after_ident_must_be_space: // test_exist
			sprintf(msg, "после идентификатора должен идти ' '");
			break;
		case stalpe: // test_exist
			sprintf(msg, "в функции аргументы должны быть описаны через запятую, в скобках");
			break;
		case functionid_must_begins_with_letter: // test_exist functionid_begins_with_letters
			sprintf(msg, "аргументы идентификатора с параметрами должены начинаться с буквы");
			break;
		case functions_cannot_be_changed: // test_exist
			sprintf(msg, "идентификатор с параметрами нельзя переопределять");
			break;
		case after_functionid_must_be_comma: // test_exist
			sprintf(msg, "после аргумента в функции должны быть ')' или ',' потом ' '");
			break;
		case ident_not_exists: // need_test
			sprintf(msg, "данный идентификатор препроцессора не существует");
			break;
		case more_than_enough_arguments: // test_exist not_enough_param
			sprintf(msg, "у этого идентификатора меньше параметров");
			break;
		case not_enough_arguments: // test_exist
			sprintf(msg, "у этой функции больше параметров");
			break;
		case must_be_string_ending: // test_exist
			sprintf(msg, "строка не завершена, пропущен символ \" или \'");
			break;
		case large_value: // test_exist
			sprintf(msg, "слишком большое число");
			break;
		case after_exp_must_be_digit: // test_exist
			sprintf(msg, "после экспоненты должно быть число");
			break;
		case invalid_parenthesis_entry: // test_exist
			sprintf(msg, "количество открывающих скобок не соответствует числу закрывающих");
			break;
		case comm_not_ended: // test_exist
			sprintf(msg, "комментарий, начавшийся с /* , не закрыт");
			break;
		case define_at_the_end: // test_exist
			sprintf(msg, "файл не может закончится до окончания команды '#DEFINE' поставьте перенос строки");
			break;
		case must_be_endif: // test_exist
			sprintf(msg, "условный оператор препроцессора должен заканчиваться '#ENDIF'");
			break;
		case needless_elif: // test_exist dont_elif
			sprintf(msg, "в этом типе условного оператора не может использоваться '#ELIF'");
			break;
		case extra_endif: // test_exist
			sprintf(msg, "перед '#ENDIF' должен стоять условный оператор препроцессора");
			break;
		case after_eval_must_be_parenthesis: // test_exist after_eval_must_be_ckob
			sprintf(msg, "сразу после команды '#EVAL' должен быть символ '('");
			break;
		case eval_must_end_with_parenthesis: // test_exist in_eval_must_end_parenthesis
			sprintf(msg, "вычисления внутри директивы #eval должны заканчиваться символом )");
			break;
		case arithmetic_operations_must_be_in_eval: // test_exist
			sprintf(msg, "все арифметические операции должны быть внутри команды '#EVAL()'");
			break;
		case logical_operations_are_prohibited_in_eval: // test_exist
			sprintf(msg, "внутри команды '#EVAL()' не должно быть логических операций");
			break;
		case ident_not_macro: // test_exist
			sprintf(msg, "идентификатор не является макросом, это недопустимо для данных вычислений");
			break;
		case incorrect_arithmetic_expression: // need_test
			sprintf(msg, "неправильно составленное арифметическое выражение, возможно неправильно расставлены скобки");
			break;
		case third_party_symbol: // test_exist
			sprintf(msg, "в строке с вычислениями не должно быть посторонних символов");
			break;
		case file_name_must_start_with_quote: // test_exist must_start_quote
			sprintf(msg, "указание имени include файла должно начинаться символа \"");
			break;
		case file_name_must_end_with_quote: // test_exist
			sprintf(msg, "указание имени include файла должно заканчиваться символом \"");
			break;
		case source_file_not_found: // need_test
			sprintf(msg, "исходный файл не найден");
			break;
		case included_file_not_found: // test_exist
			sprintf(msg, "заголовочный файл не найден");
			break;
		case macro_not_exists: // need_test
			sprintf(msg, "такого макроса не существует");
			break;
		case cicle_must_end_with_endw: // test_exist
			sprintf(msg, "цикл должен заканчиваться #ENDW");
			break;
		default:
			sprintf(msg, "не реализованная ошибка №%d", num);
			break;
	}
}

void macro_error(const int num, const char *const path, const char *const code, const size_t line, size_t position)
{
	char msg[ERROR_MSG_SIZE];
	get_message_error(num, msg);

	if (path == NULL)
	{
		log_system_error(TAG_MACRO, msg);
		return;
	}

	char tag[ERROR_TAG_SIZE];
	size_t index = sprintf(tag, "%s", path);

	if (code == NULL)
	{
		log_system_error(tag, msg);
		return;
	}

	index += sprintf(&tag[index], ":%zi", line);
	while (position > 0 && (code[position] == ' ' || code[position] == '\t'))
	{
		position--;
	}
	sprintf(&tag[index], ":%zi", position);

	log_error(tag, msg, code, position);
}

void macro_system_error(const char *const tag, const int num)
{
	char msg[ERROR_MSG_SIZE];
	get_message_error(num, msg);

	if (tag != NULL)
	{
		log_system_error(tag, msg);
	}
	else
	{
		log_system_error(TAG_MACRO, msg);
	}
}
