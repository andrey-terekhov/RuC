#include "io2.h"


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
	return io;
}


int in_set_file(universal_io *const io, const char *const path)
{
	return 0;
}

int in_set_buffer(universal_io *const io, const char *const buffer)
{
	return 0;
}

int in_set_func(universal_io *const io, const io_user_func func)
{
	return 0;
}


int in_is_file(const universal_io *const io)
{
	return 0;
}

int in_is_buffer(const universal_io *const io)
{
	return 0;
}

int in_is_func(const universal_io *const io)
{
	return 0;
}


io_func in_get_func(const universal_io *const io)
{
	return 0;
}

size_t in_get_path(const universal_io *const io, char *const buffer)
{
	return 0;
}

const char *in_get_buffer(const universal_io *const io)
{
	return 0;
}

size_t in_get_position(const universal_io *const io)
{
	return 0;
}


int in_close_file(universal_io *const io)
{
	return 0;
}

int in_clear(universal_io *const io)
{
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
	return 0;
}


int out_is_file(const universal_io *const io)
{
	return 0;
}

int out_is_buffer(const universal_io *const io)
{
	return 0;
}

int out_is_func(const universal_io *const io)
{
	return 0;
}


io_func out_get_func(const universal_io *const io)
{
	return 0;
}

size_t out_get_path(const universal_io *const io, char *const buffer)
{
	return 0;
}


char *out_extract_buffer(universal_io *const io)
{
	return 0;
}

int out_close_file(universal_io *const io)
{
	return 0;
}

int out_clear(universal_io *const io)
{
	return 0;
}


int io_erase(universal_io *const io)
{
	return 0;
}
