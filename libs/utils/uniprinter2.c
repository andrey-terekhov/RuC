#include "uniprinter2.h"
#include <stdarg.h>
#include "utf8.h"


int uni_printf(universal_io *const io, const char *const format, ...)
{
	if (!out_is_correct(io))
	{
		return -1;
	}

	va_list args;
	va_start(args, format);

	io_func func = out_get_func(io);
	int ret = func(io, format, args);

	va_end(args);
	return ret;
}

int uni_print_char(universal_io *const io, const char32_t wchar)
{
	char buffer[8];

	utf8_to_string(buffer, wchar);

	return uni_printf(io, "%s", buffer);
}
