#include "io2.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define MAX_FORMAT_SIZE 128


int is_specifier(const char ch)
{
	return (ch >= '0' && ch <= '9')
		|| ch == 'h' || ch == 'l'		// Sub-specifiers
		|| ch == 'j' || ch == 'z' 
		|| ch == 't' || ch == 'L'
		|| ch == '*'	
		|| ch == 'i'					// Integer
		|| ch == 'd' || ch == 'u'		// Decimal integer
		|| ch == 'o'					// Octal integer
		|| ch == 'x' || ch == 'X'		// Hexadecimal integer
		|| ch == 'f' || ch == 'F'		// Floating point number
		|| ch == 'e' || ch == 'E'
		|| ch == 'g' || ch == 'G'
		|| ch == 'a' || ch == 'A'
		|| ch == 'c' || ch == 'C'		// Character
		|| ch == 's' || ch == 'S'		// String of characters
		|| ch == 'p'					// Pointer address
		|| ch == '['					// Scanset
		|| ch == 'n';					// Count
}

int scan_arg(universal_io *const io, const char *const format, size_t size, void *arg)
{
	char buffer[MAX_FORMAT_SIZE];
	strncpy(buffer, format, size);
	sprintf(&buffer[size], "%%zn");

	size_t number = 0;
	int ret = sscanf(&io->in_buffer[io->in_position], buffer, arg, &number);
	io->in_position += number;

	if (io->in_position >= io->in_size && number == 0)
	{
		return 0;
	}

	return ret;
}

int in_func_buffer(universal_io *const io, const char *const format, va_list args)
{
	const size_t position = io->in_position;
	int ret = 0;

	size_t last = 0;
	size_t i = 0;
	while (format[i] != '\0')
	{
		if (format[i] == '%')
		{
			int was_count = 0;

			while (is_specifier(format[i + 1]))
			{
				i++;
				was_count = was_count || format[i] == 'n';
				
				if (format[i] == '[')
				{
					while (format[i] != '\0' && (format[i - 1] == '\\' || format[i] != ']'))
					{
						i++;
					}
				}
			}

			if (format[i] != '%')
			{
				void *arg = va_arg(args, void *);
				ret += scan_arg(io, &format[last], i + 1, arg);

				if (io->in_position >= io->in_size)
				{
					return ret;
				}

				if (was_count)
				{
					*(size_t *)arg = io->in_position - position;
				}

				last = i + 1;
			}
		}

		i++;
	}

	return ret;
}

int in_func_user(universal_io *const io, const char *const format, va_list args)
{
	return io->in_user_func(format, args);
}


int out_func_buffer(universal_io *const io, const char *const format, va_list args)
{
	va_list local;
	va_copy(local, args);

	int ret = vsnprintf(&io->out_buffer[io->out_position], io->out_size, format, local);

	if (ret != -1)
	{
		io->out_position += ret;
		return ret;
	}

	io->out_buffer[io->out_position] = '\0';

	char *new_buffer = realloc(io->out_buffer, 2 * io->out_size * sizeof(char));
	if (new_buffer == NULL)
	{
		return -1;
	}

	io->out_size *= 2;
	io->out_buffer = new_buffer;
	return out_func_buffer(io, format, args);
}

int out_func_user(universal_io *const io, const char *const format, va_list args)
{
	return io->out_user_func(format, args);
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


universal_io io_create()
{
	universal_io io;

	io.in_file = NULL;
	io.in_buffer = NULL;

	io.in_size = 0;
	io.in_position = 0;

	io.in_user_func = NULL;
	io.in_func = NULL;

	io.out_file = NULL;
	io.out_buffer = NULL;

	io.out_size = 0;
	io.out_position = 0;

	io.out_user_func = NULL;
	io.out_func = NULL;

	return io;
}

// FIX ME
int in_set_file(universal_io *const io, const char *const path)
{
	if (io == NULL || path == NULL)
	{
		return -1;
	}

	return 0;
}

int in_set_buffer(universal_io *const io, const char *const buffer)
{
	if (in_clear(io))
	{
		return -1;
	}

	io->in_buffer = buffer;

	io->in_size = strlen(io->in_buffer);
	io->in_position = 0;
	
	io->in_func = &in_func_buffer;

	return 0;
}

int in_set_func(universal_io *const io, const io_user_func func)
{
	if (in_clear(io))
	{
		return -1;
	}
	
	io->in_user_func = func;
	io->in_func = &in_func_user;

	return 0;
}


int in_is_correct(const universal_io *const io)
{
	return io != NULL && (in_is_file(io) || in_is_buffer(io) || in_is_func(io));
}

int in_is_file(const universal_io *const io)
{
	return io != NULL && io->in_file != NULL;
}

int in_is_buffer(const universal_io *const io)
{
	return io != NULL && io->in_buffer != NULL;
}

int in_is_func(const universal_io *const io)
{
	return io != NULL && io->in_user_func != NULL;
}


io_func in_get_func(const universal_io *const io)
{
	return io != NULL ? io->in_func : NULL;
}
// FIX ME
size_t in_get_path(const universal_io *const io, char *const buffer)
{
	if (!in_is_file(io))
	{
		return 0;
	}

	return 0;
}

const char *in_get_buffer(const universal_io *const io)
{
	return in_is_buffer(io) ? io->in_buffer : NULL;
}

size_t in_get_position(const universal_io *const io)
{
	return in_is_buffer(io) ? io->in_position : 0;
}

// FIX ME
int in_close_file(universal_io *const io)
{
	return 0;
}

int in_clear(universal_io *const io)
{
	if (io == NULL)
	{
		return -1;
	}

	if (in_is_file(io))
	{
		in_close_file(io);
	}
	else if (in_is_buffer(io))
	{
		io->in_buffer = NULL;

		io->in_size = 0;
		io->in_position = 0;

		io->in_func = NULL;
	}
	else
	{
		io->in_func = NULL;
		io->in_user_func = NULL;
	}
	
	return 0;
}

// FIX ME
int out_set_file(universal_io *const io, const char *const path)
{
	return 0;
}
// FIX ME
int out_set_buffer(universal_io *const io, const size_t size)
{
	if (out_clear(io))
	{
		return -1;
	}

	io->out_size = size + 1;
	io->out_buffer = malloc(io->out_size * sizeof(char));
	
	if (io->out_buffer == NULL)
	{
		io->out_size = 0;
		return -1;
	}

	io->out_buffer[0] = '\0';
	io->out_position = 0;
	
	io->out_func = &out_func_buffer;

	return 0;
}

int out_set_func(universal_io *const io, const io_user_func func)
{
	if (out_clear(io))
	{
		return -1;
	}
	
	io->out_user_func = func;
	io->out_func = &out_func_user;

	return 0;
}


int out_is_correct(const universal_io *const io)
{
	return io != NULL && (out_is_file(io) || out_is_buffer(io) || out_is_func(io));;
}

int out_is_file(const universal_io *const io)
{
	return io != NULL && io->out_file != NULL;
}

int out_is_buffer(const universal_io *const io)
{
	return io != NULL && io->out_buffer != NULL;
}

int out_is_func(const universal_io *const io)
{
	return io != NULL && io->out_user_func != NULL;
}


io_func out_get_func(const universal_io *const io)
{
	return io != NULL ? io->out_func : NULL;
}
// FIX ME
size_t out_get_path(const universal_io *const io, char *const buffer)
{
	return 0;
}


char *out_extract_buffer(universal_io *const io)
{
	if (!out_is_buffer(io))
	{
		return NULL;
	}

	char *buffer = io->out_buffer;
	io->out_buffer = NULL;

	io->out_size = 0;
	io->out_position = 0;

	io->out_func = NULL;
	return buffer;
}
// FIX ME
int out_close_file(universal_io *const io)
{
	return 0;
}

int out_clear(universal_io *const io)
{
	if (io == NULL)
	{
		return -1;
	}

	if (out_is_file(io))
	{
		out_close_file(io);
	}
	else if (out_is_buffer(io))
	{
		free(out_extract_buffer(io));
	}
	else
	{
		io->out_func = NULL;
		io->out_user_func = NULL;
	}
	
	return 0;
}


int io_erase(universal_io *const io)
{
	return in_clear(io) || out_clear(io) ? -1 : 0;
}
