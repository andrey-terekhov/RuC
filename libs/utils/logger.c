/*
 *	Copyright 2020 Andrey Terekhov, Victor Y. Fadeev, Dmitrii Davladov
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

#include "logger.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "utf8.h"

#ifdef _WIN32
	#include <windows.h>

	static const uint8_t COLOR_TAG = 0x0F;
	static const uint8_t COLOR_ERROR = 0x0C;
	static const uint8_t COLOR_WARNING = 0x0D;
	static const uint8_t COLOR_NOTE = 0x0E;
	static const uint8_t COLOR_DEFAULT = 0x07;
#else
	static const uint8_t COLOR_TAG = 39;
	static const uint8_t COLOR_ERROR = 31;
	static const uint8_t COLOR_WARNING = 35;
	static const uint8_t COLOR_NOTE = 33;
	static const uint8_t COLOR_DEFAULT = 0;
#endif

#define MAX_MSG_SIZE 1024

static const char *const TAG_LOGGER = "logger";

static const char *const TAG_ERROR = "ошибка";
static const char *const TAG_WARNING = "предупреждение";
static const char *const TAG_NOTE = "примечание";

static const char *const ERROR_LOGGER_ARG_NULL = "NULL указатель на строку";
static const char *const ERROR_LOGGER_ARG_MULTILINE  = "многострочный входной параметр";
static const char *const ERROR_LOGGER_NO_LOCATION  = "позиционирование невозможно";


static void default_error_log(const char *const tag, const char *const msg);
static void default_warning_log(const char *const tag, const char *const msg);
static void default_note_log(const char *const tag, const char *const msg);


static logger current_error_log = &default_error_log;
static logger current_warning_log = &default_warning_log;
static logger current_note_log = &default_note_log;


static inline void set_color(const uint8_t color)
{
#if defined(NDEBUG) || !defined(__APPLE__)
	#ifdef _WIN32
		SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), color);
	#else
		fprintf(stderr, "\x1B[1;%im", color);
	#endif
#else
	(void)color;
#endif
}

static inline void print_msg(const uint8_t color, const char *const msg)
{
	set_color(COLOR_DEFAULT);

	size_t i = 0;
	while (msg[i] != '\0' && msg[i] != '\n')
	{
		fprintf(stderr, "%c", msg[i++]);
	}

	if (msg[i] == '\0')
	{
		fprintf(stderr, "\n");
		return;
	}

	size_t j = i + 1;
	while (msg[j] != '\0' && msg[j] != '\n')
	{
		j++;
	}

	if (msg[j] == '\0')
	{
		fprintf(stderr, "%s\n", &msg[i]);
		return;
	}

	while (msg[j] != '^')
	{
#ifdef _WIN32
		fprintf(stderr, "%c", msg[i++]);
#else
		for (size_t k = utf8_symbol_size(msg[i]); k > 0; k--)
		{
			fprintf(stderr, "%c", msg[i++]);
		}
#endif

		j++;
	}

	set_color(color);
	while (msg[j] != '\0')
	{
#ifdef _WIN32
		fprintf(stderr, "%c", msg[i++]);
#else
		for (size_t k = utf8_symbol_size(msg[i]); k > 0; k--)
		{
			fprintf(stderr, "%c", msg[i++]);
		}
#endif

		j++;
	}

	set_color(COLOR_DEFAULT);
	while (msg[i] != '\n')
	{
		fprintf(stderr, "%c", msg[i++]);
	}

	set_color(color);
	fprintf(stderr, "%s\n", &msg[i]);
	set_color(COLOR_DEFAULT);
}


static inline void default_log(const char *const tag, const char *const msg, const uint8_t color, const char *const tag_log)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(color);
#ifdef _WIN32
	char buffer[MAX_MSG_SIZE];
	utf8_to_cp866(tag_log, buffer);
	fprintf(stderr, "%s: ", buffer);
#else
	fprintf(stderr, "%s: ", tag_log);
#endif

#ifdef _WIN32
	utf8_to_cp866(msg, buffer);
	print_msg(color, buffer);
#else
	print_msg(color, msg);
#endif
}

static void default_error_log(const char *const tag, const char *const msg)
{
	default_log(tag, msg, COLOR_ERROR, TAG_ERROR);
}

static void default_warning_log(const char *const tag, const char *const msg)
{
	default_log(tag, msg, COLOR_WARNING, TAG_WARNING);
}

static void default_note_log(const char *const tag, const char *const msg)
{
	default_log(tag, msg, COLOR_NOTE, TAG_NOTE);
}


static int check_arg(const char *const arg)
{
	if (arg == NULL)
	{
		current_error_log(TAG_LOGGER, ERROR_LOGGER_ARG_NULL);
		return -1;
	}

	if (strchr(arg, '\n') != NULL)
	{
		current_error_log(TAG_LOGGER, ERROR_LOGGER_ARG_MULTILINE);
		return -1;
	}

	return 0;
}


static inline size_t literal(const char *const line, const size_t symbol)
{
	size_t i = utf8_to_first_byte(line, symbol);
	size_t j = i;

	char32_t ch = utf8_convert(&line[j]);
	while (utf8_is_letter(ch) || utf8_is_digit(ch))
	{
		i = j;
		if (j == 0)
		{
			break;
		}

		j = utf8_to_first_byte(line, j - 1);
		ch = utf8_convert(&line[j]);
	}

	return i;
}


static inline size_t length(const char *const line, const size_t size, const size_t symbol)
{
	size_t i = symbol;
	size_t j = i;

	while (i < size)
	{
		const char32_t ch = utf8_convert(&line[i]);

		if (utf8_is_letter(ch) || utf8_is_digit(ch))
		{
			i += utf8_symbol_size(line[i]);
			j++;
			continue;
		}

		break;
	}

	return symbol >= size ? 0 : j == symbol ? 1 : j - symbol;
}

static void splice(char *const buffer, const char *const msg, const char *const line, const size_t symbol)
{
	size_t cur = (size_t)sprintf(buffer, "%s\n", msg);

	size_t size = 0;
	while (line[size] != '\0' && line[size] != '\n')
	{
		cur += (size_t)sprintf(&buffer[cur], "%c", line[size]);
		size++;
	}

	const size_t ch = literal(line, symbol);
	const size_t len = length(line, size, ch);
	if (len == 0)
	{
		return;
	}

	buffer[cur++] = '\n';
	for (size_t i = 0; i < ch; i += utf8_symbol_size(line[i]))
	{
		buffer[cur++] = line[i] == '\t' ? '\t' : ' ';
	}

	buffer[cur++] = '^';
	for (size_t i = 0; i < len - 1; i++)
	{
		buffer[cur++] = '~';
	}

	buffer[cur++] = '\0';
}


static inline void log_main(const logger func, const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (check_arg(tag) || check_arg(msg))
	{
		return;
	}

	if (line == NULL)
	{
		current_error_log(TAG_LOGGER, ERROR_LOGGER_ARG_NULL);
		return;
	}

	char buffer[MAX_MSG_SIZE];
	splice(buffer, msg, line, symbol);

	func(tag, buffer);
}

static inline void log_auto(const logger func, location *const loc, const char *const msg)
{
	if (check_arg(msg))
	{
		return;
	}

	char tag[MAX_MSG_SIZE];
	char line[MAX_MSG_SIZE];
	if (!loc_get_tag(loc, tag) || !loc_get_code_line(loc, line))
	{
		current_error_log(TAG_LOGGER, ERROR_LOGGER_NO_LOCATION);
		return;
	}

	char buffer[MAX_MSG_SIZE];
	splice(buffer, msg, line, loc_get_index(loc));

	func(tag, buffer);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


int set_error_log(const logger func)
{
	if (func == NULL)
	{
		return -1;
	}

	current_error_log = func;
	return 0;
}

int set_warning_log(const logger func)
{
	if (func == NULL)
	{
		return -1;
	}

	current_warning_log = func;
	return 0;
}

int set_note_log(const logger func)
{
	if (func == NULL)
	{
		return -1;
	}

	current_note_log = func;
	return 0;
}


void log_error(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	log_main(current_error_log, tag, msg, line, symbol);
}

void log_warning(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	log_main(current_warning_log, tag, msg, line, symbol);
}

void log_note(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	log_main(current_note_log, tag, msg, line, symbol);
}


void log_auto_error(location *const loc, const char *const msg)
{
	log_auto(current_error_log, loc, msg);
}

void log_auto_warning(location *const loc, const char *const msg)
{
	log_auto(current_warning_log, loc, msg);
}

void log_auto_note(location *const loc, const char *const msg)
{
	log_auto(current_note_log, loc, msg);
}


void log_system_error(const char *const tag, const char *const msg)
{
	if (check_arg(tag) || check_arg(msg))
	{
		return;
	}

	current_error_log(tag, msg);
}

void log_system_warning(const char *const tag, const char *const msg)
{
	if (check_arg(tag) || check_arg(msg))
	{
		return;
	}

	current_warning_log(tag, msg);
}

void log_system_note(const char *const tag, const char *const msg)
{
	if (check_arg(tag) || check_arg(msg))
	{
		return;
	}

	current_note_log(tag, msg);
}
