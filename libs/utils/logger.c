#include "logger.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <uchar.h>

#ifdef _MSC_VER
	#include <windows.h>

	const uint8_t COLOR_TAG = 0x0F;
	const uint8_t COLOR_ERROR = 0x0C;
	const uint8_t COLOR_WARNING = 0x0D;
	const uint8_t COLOR_NOTE = 0x0E;
	const uint8_t COLOR_DEFAULT = 0x07;
#else
	const uint8_t COLOR_TAG = 39;
	const uint8_t COLOR_ERROR = 31;
	const uint8_t COLOR_WARNING = 35;
	const uint8_t COLOR_NOTE = 33;
	const uint8_t COLOR_DEFAULT = 0;
#endif

#define BUFFER_SIZE 1024


void default_error_log(const char *const tag, const char *const msg);
void default_warning_log(const char *const tag, const char *const msg);
void default_note_log(const char *const tag, const char *const msg);


logger current_error_log = &default_error_log;
logger current_warning_log = &default_warning_log;
logger current_note_log = &default_note_log;


void set_color(const uint8_t color)
{
#ifdef _MSC_VER
	SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), color);
#else
	fprintf(stderr, "\x1B[1;%im", color);
#endif
}

int is_russian(const char *const str)
{
	const char16_t ch = ((char16_t)str[0] << 8) | ((char16_t)str[1] & 0x00FF);

	return  ch == 'Ё' || ch == 'ё'
		|| (ch >= 'А' && ch <= 'Я')
		|| (ch >= 'а' && ch <= 'п')
		|| (ch >= 'р' && ch <= 'я');
}

void print_msg(const uint8_t color, const char *const msg)
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
		fprintf(stderr, "%c", msg[i++]);
		if (is_russian(&msg[i - 1]))
		{
			fprintf(stderr, "%c", msg[i++]);
		}
		
		j++;
	}

	set_color(color);
	while (msg[j] != '\0')
	{
		fprintf(stderr, "%c", msg[i++]);
		if (is_russian(&msg[i - 1]))
		{
			fprintf(stderr, "%c", msg[i++]);
		}

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

void default_error_log(const char *const tag, const char *const msg)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(COLOR_ERROR);
	fprintf(stderr, "ошибка: ");

	print_msg(COLOR_ERROR, msg);
}

void default_warning_log(const char *const tag, const char *const msg)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(COLOR_WARNING);
	fprintf(stderr, "предупреждение: ");

	print_msg(COLOR_WARNING, msg);
}

void default_note_log(const char *const tag, const char *const msg)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(COLOR_NOTE);
	fprintf(stderr, "примечание: ");

	print_msg(COLOR_NOTE, msg);
}


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


int check_tag_msg(const char *const tag, const char *const msg)
{
	return tag == NULL || msg == NULL || strchr(tag, '\n') != NULL || strchr(msg, '\n') != NULL;
}

void log_system_error(const char *const tag, const char *const msg)
{
	if (check_tag_msg(tag, msg))
	{
		return;
	}
	
	current_error_log(tag, msg);
}

void log_system_warning(const char *const tag, const char *const msg)
{
	if (check_tag_msg(tag, msg))
	{
		return;
	}
	
	current_warning_log(tag, msg);
}

void log_system_note(const char *const tag, const char *const msg)
{
	if (check_tag_msg(tag, msg))
	{
		return;
	}
	
	current_note_log(tag, msg);
}


size_t length(const char *const line, const size_t size, const size_t symbol)
{
	size_t i = symbol;
	size_t j = i;

	while (i < size)
	{
		if (line[i] == '_' || (line[i] >= '0' && line[i] <= '9')
			|| (line[i] >= 'A' && line[i] <= 'Z')
			|| (line[i] >= 'a' && line[i] <= 'z'))
		{
			i++;
			j++;
			continue;
		}

		if (i + 1 < size && is_russian(&line[i]))
		{
			i += 2;
			j++;
			continue;
		}

		break;
	}

	return symbol >= size ? 0 : j == symbol ? 1 : j - symbol;
}

void splice(char *const buffer, const char *const msg, const char *const line, const size_t symbol)
{
	size_t cur = sprintf(buffer, "%s\n", msg);

	size_t size = 0;
	while (line[size] != '\0' && line[size] != '\n')
	{
		cur = sprintf(&buffer[cur], "%c", line[size]);
		size++;
	}

	const size_t len = length(line, size, symbol);
	if (len == 0)
	{
		return;
	}

	buffer[cur++] = '\n';
	for (size_t i = 0; i < symbol; i++)
	{
		buffer[cur++] = line[i] == '\t' ? '\t' : ' ';
		if (is_russian(&line[i]))
		{
			i++;
		}
	}

	buffer[cur++] = '^';
	for (size_t i = 0; i < len - 1; i++)
	{
		buffer[cur++] = '~';
	}

	buffer[cur++] = '\0';
}

void log_error(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (check_tag_msg(tag, msg) || line == NULL)
	{
		return;
	}
	
	char buffer[BUFFER_SIZE];
	splice(buffer, msg, line, symbol);

	current_error_log(tag, buffer);
}

void log_warning(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (check_tag_msg(tag, msg) || line == NULL)
	{
		return;
	}
	
	char buffer[BUFFER_SIZE];
	splice(buffer, msg, line, symbol);

	current_warning_log(tag, buffer);
}

void log_note(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (check_tag_msg(tag, msg) || line == NULL)
	{
		return;
	}
	
	char buffer[BUFFER_SIZE];
	splice(buffer, msg, line, symbol);

	current_note_log(tag, buffer);
}
