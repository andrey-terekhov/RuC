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
#include <stdio.h>
#include "logger.h"
#include "utf8.h"


#define TAG_MACRO		"macro"

#define MAX_TAG_SIZE	1024
#define MAX_MSG_SIZE	4096


static size_t utf8_to_buffer(const char32_t *const src, char *const dest)
{
	size_t size = 0;
	for (size_t i = 0; src[i] != '\0'; i++)
	{
		size += utf8_to_string(&dest[size], src[i]);
	}

	return size;
}


static void get_error(const error_t num, char *const msg, va_list args)
{
	switch (num)
	{
		case LINKER_CANNOT_OPEN:
			sprintf(msg, "невозможно открыть исходные тексты");
			break;

		case MACRO_NAME_NON:
			sprintf(msg, "предопределенный макрос должен иметь имя");
			break;
		case MACRO_NAME_FIRST_CHARACTER:
			sprintf(msg, "имя макроса должно начинаться с буквы или '_'");
			break;
		case MACRO_NAME_EXISTS:
		{
			const char32_t *const name = va_arg(args, char32_t *);

			size_t index = sprintf(msg, "макрос '");
			index += utf8_to_buffer(name, &msg[index]);
			sprintf(&msg[index], "' уже существует");
		}
		break;

		case PARSER_COMM_NOT_ENDED:
			sprintf(msg, "незакрытый комментарий в конце файла");
			break;
		case PARSER_STRING_NOT_ENDED:
			sprintf(msg, "перенос строки в константе");
			break;
		case PARSER_UNEXPECTED_EOF:
			sprintf(msg, "непредвиденное обнаружение конца файла");
			break;

		case PARSER_UNIDETIFIED_KEYWORD:
			sprintf(msg, "нераспознанная директива препроцессора");
			break;
		case PARSER_UNEXPECTED_GRID:
			sprintf(msg, "знак \'#\' здесь не предполагается");
			break;
		case PARSER_UNEXPECTED_ENDM:
			sprintf(msg, "отсутствует #macro для этой директивы");
			break;
		case PARSER_UNEXPECTED_ENDIF:
			sprintf(msg, "отсутствует #if для этой директивы");
			break;
		case PARSER_UNEXPECTED_ENDW:
			sprintf(msg, "отсутствует #while для этой директивы");
			break;

		case PARSER_INCLUDE_NEED_FILENAME:
			sprintf(msg, "#include требуется \"FILENAME\"");
			break;
		case PARSER_INCLUDE_INCORRECT_FILENAME:
			sprintf(msg, "не удается открыть источник файл");
			break;

		case PARSER_NEED_IDENT:
			sprintf(msg, "требуется идентификатор");
			break;
		case PARSER_BIG_IDENT_NAME:
			sprintf(msg, "лексема переполнила внутренний буффер");
			break;
		case PARSER_NEED_SEPARATOR:
			sprintf(msg, "требуется разделитель");
			break;
		case PARSER_IDENT_NEED_ARGS:
			sprintf(msg, "требуется '(");
			break;

		case PARSER_SET_NOT_EXIST_IDENT:
			sprintf(msg, "переопределение несуществующего идентификатора");
			break;
		case PARSER_SET_WITH_ARGS:
			sprintf(msg, "для #set переопределение с аргументами запрещено");
			break;

		case PARSER_MACRO_NOT_ENDED:
			sprintf(msg, "отсутствует #endm для этой директивы");
			break;

		default:
			sprintf(msg, "неизвестная ошибка");
			break;
	}
}

static void get_warning(const warning_t num, char *const msg, va_list args)
{
	(void)args;

	switch (num)
	{
		case MACRO_CONSOLE_SEPARATOR:
			sprintf(msg, "следует использовать разделитель '=' после имени макроса");
			break;

		case PARSER_UNEXPECTED_LEXEME:
			sprintf(msg, "непредвиденная лексема за директивой препроцессора, требуется перенос строки");
			break;

		case PARSER_UNDEF_NOT_EXIST_IDENT:
			sprintf(msg, "удаление несуществующего идентификатора");
			break;

		default:
			sprintf(msg, "неизвестное предупреждение");
			break;
	}
}


static void output(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const char *const msg, void (*func)(const char *const, const char *const, const char *const, const size_t))
{
	size_t size = 0;
	for (size_t i = 0; i < symbol && str[i] != '\0'; i += utf8_symbol_size(str[i]))
	{
		size++;
	}

	char tag[MAX_TAG_SIZE];
	sprintf(tag, "%s:%zu:%zu", file, line, size);

	func(tag, msg, str, size);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void macro_error(const char *const file, const char *const str, const size_t line, const size_t symbol
	, error_t num, ...)
{
	va_list args;
	va_start(args, num);

	macro_verror(file, str, line, symbol, num, args);

	va_end(args);
}

void macro_warning(const char *const file, const char *const str, const size_t line, const size_t symbol
	, warning_t num, ...)
{
	va_list args;
	va_start(args, num);

	macro_vwarning(file, str, line, symbol, num, args);

	va_end(args);
}


void macro_verror(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const error_t num, va_list args)
{
	char msg[MAX_MSG_SIZE];
	get_error(num, msg, args);
	output(file, str, line, symbol, msg, &log_error);
}

void macro_vwarning(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const warning_t num, va_list args)
{
	char msg[MAX_MSG_SIZE];
	get_warning(num, msg, args);
	output(file, str, line, symbol, msg, &log_warning);
}


void macro_system_error(const char *const tag, error_t num, ...)
{
	va_list args;
	va_start(args, num);

	char msg[MAX_MSG_SIZE];
	get_error(num, msg, args);

	va_end(args);
	log_system_error(tag != NULL ? tag : TAG_MACRO, msg);
}

void macro_system_warning(const char *const tag, warning_t num, ...)
{
	va_list args;
	va_start(args, num);

	char msg[MAX_MSG_SIZE];
	get_warning(num, msg, args);

	va_end(args);
	log_system_warning(tag != NULL ? tag : TAG_MACRO, msg);
}
