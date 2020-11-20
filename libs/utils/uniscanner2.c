#include "uniscanner2.h"
#include <stdarg.h>
#include "utf8.h"


int uni_scanf(universal_io *const io, const char *const format, ...)
{
	if (!in_is_correct(io))
	{
		return 0;
	}

	va_list args;
	va_start(args, format);

	io_func func = in_get_func(io);
	int ret = func(io, format, args);

	va_end(args);
	return ret;
}

char32_t uni_scan_char(universal_io *const io)
{
	char buffer[8];
	if (!uni_scanf(io, "%c", &buffer[0]))
	{
		return 0x00000000;
	}

	const size_t size = utf8_symbol_size(buffer[0]);
	for (size_t i = 1; i < size; i++)
	{
		if (!uni_scanf(io, "%c", &buffer[i]))
		{
			return 0x00000000;
		}
	}
	
	return utf8_convert(buffer);
}
