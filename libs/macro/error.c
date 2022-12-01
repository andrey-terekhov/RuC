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
#include "uniscanner.h"
#include "utf8.h"


#define TAG_MACRO		"macro"
#define QUOTE			"\""

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
		case LINKER_NO_INPUT:
			sprintf(msg, "нет входных файлов");
			break;
		case LINKER_WRONG_IO:
			sprintf(msg, "некорректные параметры ввода/вывода");
			break;
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

			size_t index = sprintf(msg, "макрос " QUOTE);
			index += utf8_to_buffer(name, &msg[index]);
			sprintf(&msg[index], QUOTE " уже существует");
		}
		break;
		case MACRO_NAME_REDEFINE:
		{
			const char *const name = va_arg(args, char *);
			sprintf(msg, "макрос " QUOTE "%s" QUOTE " уже существует", name);
		}
		break;

		case CALL_DEPTH:
			sprintf(msg, "превышена максимальная глубина вызова");
			break;
		case ITERATION_MAX:
			sprintf(msg, "превышено максимальное количество итераций");
			break;
		case STRING_UNTERMINATED:
		{
			const char32_t quote = va_arg(args, char32_t);

			size_t index = sprintf(msg, "завершающий символ ");
			index += utf8_to_string(&msg[index], quote);
			sprintf(&msg[index], " пропущен");
		}
		break;
		case COMMENT_UNTERMINATED:
			sprintf(msg, "незавершённый комментарий");
			break;

		case DIRECTIVE_NAME_NON:
		{
			const char *const directive = va_arg(args, char *);
			sprintf(msg, "в директиве %s пропущено имя макроса", directive);
		}
		break;
		case DIRECTIVE_INVALID:
		{
			const char *const directive = va_arg(args, char *);
			sprintf(msg, "неизвестная директива препроцессора %s", directive);
		}
		break;

		case HASH_STRAY:
			sprintf(msg, "потерянный '#' в программе");
			break;
		case HASH_ON_EDGE:
			sprintf(msg, "'##' не может стоять по краям макроса");
			break;
		case HASH_NOT_FOLLOWED:
		{
			const char *const hash = va_arg(args, char *);
			sprintf(msg, "'%s' не сопровождается параметром макроса", hash);
		}
		break;

		case INCLUDE_EXPECTS_FILENAME:
		{
			const char *const directive = va_arg(args, char *);
			sprintf(msg, "%s ожидает \"ИМЯФАЙЛА\" или <ИМЯФАЙЛА>", directive);
		}
		break;
		case INCLUDE_NO_SUCH_FILE:
			sprintf(msg, "нет такого файла или каталога");
			break;
		case INCLUDE_DEPTH:
			sprintf(msg, "превышена максимальная глубина подключения");
			break;

		case ARGS_DUPLICATE:
		{
			const char *const name = va_arg(args, char *);
			sprintf(msg, "дублирующий макро параметр " QUOTE "%s" QUOTE, name);
		}
		break;
		case ARGS_EXPECTED_NAME:
		{
			const char32_t character = va_arg(args, char32_t);
			universal_io *io = va_arg(args, universal_io *);

			size_t index = sprintf(msg, "ожидалось имя параметра, найдено " QUOTE);

			const size_t position = in_get_position(io);
			index += utf8_is_digit(character) ? uni_scan_number(io, &msg[index])
					: utf8_to_string(&msg[index], character);
			in_set_position(io, position);

			sprintf(&msg[index], QUOTE);
		}
		break;
		case ARGS_EXPECTED_COMMA:
		{
			const char32_t character = va_arg(args, char32_t);
			universal_io *io = va_arg(args, universal_io *);

			size_t index = sprintf(msg, "ожидалась ',' или ')', найдено " QUOTE);

			const size_t position = in_get_position(io);
			index += utf8_is_letter(character) ? uni_scan_identifier(io, &msg[index])
					: utf8_is_digit(character) ? uni_scan_number(io, &msg[index])
						: utf8_to_string(&msg[index], character);
			in_set_position(io, position);

			sprintf(&msg[index], QUOTE);
		}
		break;
		case ARGS_EXPECTED_BRACKET:
			sprintf(msg, "ожидалась ')' до завершения строки");
			break;
		case ARGS_UNTERMINATED:
		{
			const char *const name = va_arg(args, char *);
			sprintf(msg, "незавершённый список аргументов, вызываемый макрос " QUOTE "%s" QUOTE, name);
		}
		break;
		case ARGS_REQUIRES:
		{
			const char *const name = va_arg(args, char *);
			const size_t expected = va_arg(args, size_t);
			const size_t actual = va_arg(args, size_t);

			const bool one = expected % 10 == 1 && expected % 100 != 11;
			const bool many = expected % 10 == 0 || expected % 10 >= 5
				|| (expected % 100 >= 11 && expected % 100 <= 14);

			sprintf(msg, "макросу " QUOTE "%s" QUOTE " требуется %zu аргумент%s, но передан%s только %zu"
					, name, expected, one ? "" : many ? "ов" : "а"
					, actual % 10 == 1 && actual % 100 != 11 ? "" : "о", actual);
		}
		break;
		case ARGS_PASSED:
		{
			const char *const name = va_arg(args, char *);
			const size_t expected = va_arg(args, size_t);
			const size_t actual = va_arg(args, size_t);

			const bool one = actual % 10 == 1 && actual % 100 != 11;
			const bool many = actual % 10 == 0 || actual % 10 >= 5
				|| (actual % 100 >= 11 && actual % 100 <= 14);

			sprintf(msg, "макросу " QUOTE "%s" QUOTE " передан%s %zu аргумент%s, но принимает он только %zu"
					, name, one ? "" : "о", actual, one ? "" : many ? "ов" : "а", expected);
		}
		break;
		case ARGS_NON:
		{
			const char *const name = va_arg(args, char *);
			sprintf(msg, "макросу " QUOTE "%s" QUOTE " не передано аргументов", name);
		}
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
		case MACRO_NAME_UNDEFINED:
		{
			const char *const name = va_arg(args, char *);
			sprintf(msg, "макрос " QUOTE "%s" QUOTE " не определён", name);
		}
		break;

		case DIRECTIVE_EXTRA_TOKENS:
		{
			const char *const directive = va_arg(args, char *);
			sprintf(msg, "дополнительные токены в конце директивы %s", directive);
		}
		break;
		case DIRECTIVE_LINE_SKIPED:
			sprintf(msg, "директива позиционирования будет пропущена");
			break;

		default:
			sprintf(msg, "неизвестное предупреждение");
			break;
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


void macro_error(location *const loc, error_t num, ...)
{
	va_list args;
	va_start(args, num);

	macro_verror(loc, num, args);

	va_end(args);
}

void macro_warning(location *const loc, warning_t num, ...)
{
	va_list args;
	va_start(args, num);

	macro_vwarning(loc, num, args);

	va_end(args);
}


void macro_verror(location *const loc, const error_t num, va_list args)
{
	char msg[MAX_MSG_SIZE];
	get_error(num, msg, args);
	log_auto_error(loc, msg);
}

void macro_vwarning(location *const loc, const warning_t num, va_list args)
{
	char msg[MAX_MSG_SIZE];
	get_warning(num, msg, args);
	log_auto_warning(loc, msg);
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
