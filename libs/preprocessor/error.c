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

#include "error.h"
#include "constants.h"
#include "logger.h"
#include <stddef.h>
#include <stdio.h>


#define TAG_MACRO      "macro"

#define ERROR_TAG_SIZE STRING_SIZE
#define ERROR_MSG_SIZE STRING_SIZE


void get_message_error(const int num, char *const msg)
{
    switch (num)
    {
        case after_preproces_words_must_be_space: // test_exist
            sprintf(msg, "после команды препроцессора должен идти перенос строки");
            break;
        case after_ident_must_be_space: // test_exist
            sprintf(msg, "после идентификатора должен идти ' '");
            break;
        case ident_begins_with_letters: // test_exist
            sprintf(msg, "идентификатор должен начинаться с буквы ");
            break;
        case must_be_endif: // test_exist
            sprintf(msg, "условный оператор препроцессора должен заканчиваться '#ENDIF'");
            break;
        case dont_elif: // test_exist
            sprintf(msg, "в этом типе условного оператора не может использоваться '#ELIF'");
            break;
        case preproces_words_not_exist: // test_exist
            sprintf(msg, "в препроцессоре не существует такой команды");
            break;
        case not_enough_param: // test_exist
            sprintf(msg, "у этого идентификатора меньше параметров");
            break;
        case functionid_begins_with_letters: // test_exist
            sprintf(msg, "идентификатор с параметрами должен начинаться с буквы");
            break;
        case functions_cannot_be_changed: // need_test
            sprintf(msg, "идентификатор с параметрами нельзя переопределять");
            break;
        case after_functionid_must_be_comma: // test_exist
            sprintf(msg, "после идентификатора в функции должны быть ')' или ',' потом ' '");
            break;
        case stalpe: // need_test
            sprintf(msg, "в функции аргументы должны быть описаны через запятую, в скобках");
            break;
        case before_endif: // need_test
            sprintf(msg, "перед '#ENDIF' должен стоять условный оператор препроцессора");
            break;
        case repeat_ident: // test_exist
            sprintf(msg, "этот идентификатор препроцессора уже используется");
            break;
        case ident_not_exist: // need_test
            sprintf(msg, "данный идентификатор препроцессора не существует");
            break;
        case comm_not_ended: // need_test
            sprintf(msg, "комментарий, начавшийся с /* , не закрыт");
            break;
        case not_enough_param2: // need_test
            sprintf(msg, "у этой функции больше параметров");
            break;
        case not_end_fail_define: // need_test
            sprintf(msg, "файл не может закончится до окончания команды '#DEFINE' поставьте перенос строки");
            break;
        case scope_not_close: // need_test
            sprintf(msg, "количество открывающих скобок не соответствует числу закрывающих");
            break;
        case after_eval_must_be_ckob: // test_exist
            sprintf(msg, "сразу после команды '#EVAL' должен быть символ '('");
            break;
        case too_many_nuber: // need_test
            sprintf(msg, "слишком большое число");
            break;
        case must_be_digit_after_exp1: // need_test
            sprintf(msg, "после экспоненты должно быть число");
            break;
        case not_arithmetic_operations: // need_test
            sprintf(msg, "все арифметические операции должны быть внутри команды '#EVAL()'");
            break;
        case not_logical_operations: // need_test
            sprintf(msg, "внутри команды '#EVAL()' не должно быть логических операций");
            break;
        case not_macro: // need_test
            sprintf(msg, "идентификатор не является макросом, это недопустимо для данных вычислений");
            break;
        case incorrect_arithmetic_expression: // need_test
            sprintf(msg, "неправильно составленное арифметическое выражение, возможно неправильно расставлены скобки");
            break;
        case third_party_symbol: // need_test
            sprintf(msg, "в строке с вычислениями не должно быть посторонних символов");
            break;
        case in_eval_must_end_parenthesis: // test_exist
            sprintf(msg, "вычисления внутри директивы #eval должны заканчиваться символом )");
            break;
        case must_end_quote: // need_test
            sprintf(msg, "указание имени include файла должно заканчиваться символом \"");
            break;
        case must_start_quote: // test_exist
            sprintf(msg, "указание имени include файла должно начинаться символа \"");
            break;
        case macro_does_not_exist: // need_test
            sprintf(msg, "такого макроса не существует");
            break;
        case must_end_endw: // need_test
            sprintf(msg, "цикл должен заканчиваться #ENDW");
            break;
        case source_file_not_found: // need_test
            sprintf(msg, "исходный файл не найден");
            break;
        case include_file_not_found: // need_test
            sprintf(msg, "заголовочный файл не найден");
            break;
        case no_string_ending: // need_test
            sprintf(msg, "строка не завершена, пропущен символ \" или \'");
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
