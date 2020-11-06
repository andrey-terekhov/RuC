#include "logger.h"
#include <stdio.h>
#include <stdint.h>

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


void log_system_error(const char *const tag, const char *const msg)
{
	current_error_log(tag, msg);
}

void log_system_warning(const char *const tag, const char *const msg)
{
	current_warning_log(tag, msg);
}

void log_system_note(const char *const tag, const char *const msg)
{
	current_note_log(tag, msg);
}
