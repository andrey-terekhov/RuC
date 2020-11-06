#include "logger.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

void default_error_log(const char *const tag, const char *const msg)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(COLOR_ERROR);
	fprintf(stderr, "ошибка: ");

	set_color(COLOR_DEFAULT);
	fprintf(stderr, "%s\n", msg);
}

void default_warning_log(const char *const tag, const char *const msg)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(COLOR_WARNING);
	fprintf(stderr, "предупреждение: ");

	set_color(COLOR_DEFAULT);
	fprintf(stderr, "%s\n", msg);
}

void default_note_log(const char *const tag, const char *const msg)
{
	set_color(COLOR_TAG);
	fprintf(stderr, "%s: ", tag);

	set_color(COLOR_NOTE);
	fprintf(stderr, "примечание: ");

	set_color(COLOR_DEFAULT);
	fprintf(stderr, "%s\n", msg);
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


void log_system_error(const char *const tag, const char *const msg)
{
	if (tag == NULL || msg == NULL)
	{
		return;
	}
	
	current_error_log(tag, msg);
}

void log_system_warning(const char *const tag, const char *const msg)
{
	if (tag == NULL || msg == NULL)
	{
		return;
	}
	
	current_warning_log(tag, msg);
}

void log_system_note(const char *const tag, const char *const msg)
{
	if (tag == NULL || msg == NULL)
	{
		return;
	}
	
	current_note_log(tag, msg);
}


size_t length(const char *const line, const size_t symbol)
{
	const size_t size = strlen(line);
	size_t i = symbol;

	while (i < size && (line[i] == '_' || line[i] == 'Ё' || line[i] == 'ё'
		|| (line[i] >= '0' && line[i] <= '9')
		|| (line[i] >= 'A' && line[i] <= 'Z')
		|| (line[i] >= 'a' && line[i] <= 'z')
		|| (line[i] >= 'А' && line[i] <= 'Я')
		|| (line[i] >= 'а' && line[i] <= 'п')
		|| (line[i] >= 'р' && line[i] <= 'я')))
	{
		i++;
	}

	return symbol >= size ? 0 : i == symbol ? 1 : i - symbol;
}

void splice(char *const buffer, const char *const msg, const char *const line, const size_t symbol)
{
	strcpy(buffer, msg);
	strcat(buffer, "\n");
	strcat(buffer, line);

	const size_t len = length(line, symbol);
	if (len == 0)
	{
		return;
	}

	size_t cur = strlen(buffer);
	buffer[cur++] = '\n';
	for (size_t i = 0; i < symbol; i++)
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


void log_error(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (tag == NULL || msg == NULL || line == NULL)
	{
		return;
	}
	
	char buffer[BUFFER_SIZE];
	splice(buffer, msg, line, symbol);

	log_system_error(tag, buffer);
}

void log_warning(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (tag == NULL || msg == NULL || line == NULL)
	{
		return;
	}
	
	char buffer[BUFFER_SIZE];
	splice(buffer, msg, line, symbol);

	log_system_warning(tag, buffer);
}

void log_note(const char *const tag, const char *const msg, const char *const line, const size_t symbol)
{
	if (tag == NULL || msg == NULL || line == NULL)
	{
		return;
	}
	
	char buffer[BUFFER_SIZE];
	splice(buffer, msg, line, symbol);

	log_system_note(tag, buffer);
}
