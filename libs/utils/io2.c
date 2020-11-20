#include "io2.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


size_t percent(universal_io *const io, const char *const format, va_list *args)
{
	/*switch (format[i])
		{
			case '%':
			{
				i += percent(io, &format[i + 1], &local);
			}
			break;

			case ' ':
			case '\t':
			case '\r':
			case '\n':
			{
				while (io->in_buffer[io->in_position] == ' '
					|| io->in_buffer[io->in_position] == '\t'
					|| io->in_buffer[io->in_position] == '\r'
					|| io->in_buffer[io->in_position] == '\n')
				{
					io->in_position++;
				}
			}
			break;
		}	
		*/
	return 0;
}

int in_func_buffer(universal_io *const io, const char *const format, va_list args)
{
	va_list local;
	va_copy(local, args);

	int ret = vsscanf(io->in_buffer, format, args);
	
	for (size_t i = 0; format[i] != '\0'; i++)
	{
		switch (format[i])
		{
			case '%':
			{
				i += percent(io, &format[i + 1], &local);
			}
			break;

			case ' ':
			case '\t':
			case '\r':
			case '\n':
			{
				while (io->in_buffer[io->in_position] == ' '
					|| io->in_buffer[io->in_position] == '\t'
					|| io->in_buffer[io->in_position] == '\r'
					|| io->in_buffer[io->in_position] == '\n')
				{
					io->in_position++;
				}
			}
			break;
			
			default:
			{
				if (io->in_buffer[io->in_position] != format[i])
				{
					return ret;
				}

				io->in_position++;
			}
		}
	}
		
	return ret;
}

int in_func_user(universal_io *const io, const char *const format, va_list args)
{
	return io->in_user_func(format, args);
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


int in_set_file(universal_io *const io, const char *const path)
{
	if (io == NULL || path == NULL)
	{
		return -1;
	}

	io->in_file = path;

	return 0;
}

int in_set_buffer(universal_io *const io, const char *const buffer)
{
	if (in_clear(io))
	{
		return -1;
	}

	io->in_buffer = buffer;

	io->in_size = strlen(buffer);
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

size_t in_get_path(const universal_io *const io, char *const buffer)
{
	if (!in_is_file(io))
	{
		return 0;
	}

	//sprintf(buffer, "%s", io->in_file);
	return strlen(buffer);
}

const char *in_get_buffer(const universal_io *const io)
{
	return in_is_buffer(io) ? io->in_buffer : NULL;
}

size_t in_get_position(const universal_io *const io)
{
	return in_is_buffer(io) ? io->in_position : 0;
}


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


int out_set_file(universal_io *const io, const char *const path)
{
	return 0;
}

int out_set_buffer(universal_io *const io, const size_t size)
{
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
